#include <cstring> // memcpy

#include "imxrt.h" // framework-arduinoteensy/cores/teensy4
#include "teensy_startup.h"

#include "plugin.h"
#include "hashflash.h"
#include "base64.h"

using namespace loader;
using namespace utils;

#ifdef ANABRID_ENABLE_GLOBAL_PLUGIN_LOADER
constexpr static int the_memsize = 1024; //< number of bytes permanently hold
uint8_t GlobalPluginLoader_Storage[GlobalPluginLoader::the_memsize];
#endif

// Memory alignment in a similar syntax to the linker ". = align(4)"
// example usage: uint8_t* mem = align((uintptr_t)mem, 4)
uintptr_t align(uintptr_t base, uint8_t exp) { return (base + exp - 1) & ~(exp - 1); }
uint8_t*  align(uint8_t*  base, uint8_t exp) { return (uint8_t*)align((uintptr_t)base, exp); }

// Returns the number of bytes "lost" by alignment. It is 0 <= disalignment <= exp.
uintptr_t disalignment(uint8_t* base, uint8_t exp) { return align(base,exp) - base;  }

/**
 * At the time being, we completely disable the Memory Protection Unit in order
 * to enable our plugin system. The MPU can harden the memory security against exploits,
 * so a more fine-grained approach is favorable in the future.
 **/
void prepare_mpu() {
  // Turn off MPU completely
  // Effect: Can execute code in stack and global storage.
  SCB_MPU_CTRL = 0;

  /*  
  SCB_MPU_RBAR = 0x00000000 | REGION(1); // ITCM
  SCB_MPU_RASR = MEM_NOCACHE | READWRITE | SIZE_512K;

  SCB_MPU_RBAR = 0x20000000 | REGION(4); // DTCM
  SCB_MPU_RASR = MEM_NOCACHE | READWRITE | NOEXEC | SIZE_512K;

  SCB_MPU_RBAR = 0x20200000 | REGION(6); // RAM (AXI bus)
  SCB_MPU_RASR = MEM_CACHE_WBWA | READWRITE | NOEXEC | SIZE_1M;
  */

  // Enabling back the MPU
  //SCB_MPU_CTRL = SCB_MPU_CTRL_ENABLE;

  // recommended to do before and after changing MPU registers
  asm("dsb");
  asm("isb");
}

GlobalPluginLoader::GlobalPluginLoader() {
    #ifdef ANABRID_ENABLE_GLOBAL_PLUGIN_LOADER
    prepare_mpu();
    plugin.load_addr = align(GlobalPluginLoader_Storage, 4);
    memsize = the_memsize - disalignment(GlobalPluginLoader_Storage, 4);
    #endif
}

/**
 * ARM32 requires callable addresses to be memory aligned. This should be ensured
 * by both the plugin linker and a properly aligned memory chunk. Returns NULLPTR if not
 * callable.
 * Furthermore, since jumping to addr results in a BLX instruction, it requires
 * bit 1 to be set for the correct thumb instructionset. This either works by +1 or &1.
 **/
uint8_t* assert_callable(uint8_t* addr) {
    if(disalignment(addr, 4) != 0) return 0;
    return addr + 1;
}


void loader::convertFromJson(JsonVariantConst src, Function& f) {
    if(src.is<int>()) {
        f.addr = src;
    } else if(src.is<JsonObjectConst>()) {
        auto o = src.as<JsonObjectConst>();

        f.addr = o["point"];
        if(o["returns"]) {
            #define M(name) if(o["returns"] == #name) f.ret_type = Function::Returns::name
            M(None); M(Bool); M(Int); M(String); M(JsonObject);
        }
    }
}

void dispatch(uint8_t* callee, Function::Returns ret_type, JsonVariant ret) {
    switch(ret_type) {
          case Function::Returns::None: {
            auto entry = (Function::None_f*) callee;
            entry();
            break;
          }
          #define CALL(type, sig) \
          case Function::Returns::type: { \
             auto entry = (Function::sig*) callee; \
             ret.set(entry()); \
             break; \
          }
          CALL(Bool, Bool_f); CALL(Int, Int_f); CALL(String, String_f); CALL(JsonObject, JsonObject_f)
    }
}

#define return_err(msg) { msg_out["error"] = msg; return false; }

bool loader::SinglePluginLoader::load_and_execute(JsonObjectConst msg_in, JsonObject &msg_out) {
    if(!can_load()) return_err("PluginLoader cannot load code. This is currently most likely due to the missing compile time flag ANABRID_ENABLE_GLOBAL_PLUGIN_LOADER");
    if(!msg_in.containsKey("entry")) return_err("Requiring entry point \"entry\"=0x1234 even if it is just 0.");

    // Load address verification
    if(msg_in["load_addr"] != plugin.load_addr) return_err("Require load address for verification. This SinglePluginLoader can only load from: [todo]");
    auto firmeware_hash = sha256_to_string(hash_flash_sha256());
    if(utils::sha256_test_short(msg_in["firmware_sha256"], firmeware_hash)) return_err("ABI mismatch: You built against [your firmware sha256] but we run [our sha256]");

    // Function pointer registration
    plugin.entry = msg_in["entry"];
    plugin.has_exit = msg_in.containsKey("exit");
    if(plugin.has_exit) plugin.exit = msg_in["exit"];

    // Actual code submission
    // For the time being, expect base64 encoded data.
    // TODO: For slightly larger payloads (mind the typical 1024byte JsonObject maximum size in the code),
    //       a multi-step transmission has to be introduced.
    auto payload = msg_in["payload"].as<std::string>();
    int payload_bin_size = payload.size() / 4 * 3;
    if(!load(payload_bin_size)) return_err("Payload too large or some code is already loaded.");
    if(!base64().decode(payload.c_str(), payload.size(), plugin.load_addr)) { unload(); return_err("Could not decode base64-encoded payload."); }

    // Code execution
    uint8_t* entry_point = assert_callable(plugin.load_addr + plugin.entry.addr);
    if(!entry_point) { unload(); return_err("Entry point address not properly aligned"); }
    dispatch(entry_point, plugin.entry.ret_type, msg_out["returns"].as<JsonVariant>());

    return true;
}

bool loader::SinglePluginLoader::unload(JsonObjectConst msg_in, JsonObject &msg_out) {
    if(!plugin.is_loaded()) { return_err("Cannot unload, nothing loaded"); }
    if(plugin.has_exit) {
        // Code execution
        uint8_t* exit_point = assert_callable(plugin.load_addr + plugin.exit.addr);
        if(!exit_point) { unload(); return_err("Exit point address not properly aligned"); }
        dispatch(exit_point, plugin.exit.ret_type, msg_out["returns"].as<JsonVariant>());
    }
    unload();
    return true;
}


bool loader::SinglePluginLoader::load(size_t code_len) { 
    if(plugin.is_loaded()) unload();
    if(code_len > memsize) return false;
    plugin.size = code_len; return true;
}

bool SinglePluginLoader::load(uint8_t* code, size_t code_len) {
    if(!load(code_len)) return false;
    memcpy(plugin.load_addr, code, code_len);
}



PluginLoader _loader; // Singleton here just for the message handlers.

bool msg::handlers::LoadPluginHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
    return _loader.load_and_execute(msg_in, msg_out);
}

bool msg::handlers::UnloadPluginHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
    return _loader.unload(msg_in, msg_out);
}
