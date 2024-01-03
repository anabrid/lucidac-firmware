#include <cstring> // memcpy

#include "imxrt.h" // framework-arduinoteensy/cores/teensy4
#include "teensy_startup.h"

#include "etl_base64.h"

#include "plugin.h"
#include "hashflash.h"
#include "logging.h"
#include "align.h"

using namespace loader;
using namespace utils;

#ifdef ANABRID_ENABLE_GLOBAL_PLUGIN_LOADER
constexpr static int the_memsize = 1024; //< number of bytes permanently hold
uint8_t GlobalPluginLoader_Storage[the_memsize];
#endif


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
    load_addr = align(GlobalPluginLoader_Storage, 4);
    memsize = the_memsize - disalignment(GlobalPluginLoader_Storage, 4);
    LOGMEV("Preparing MPU and %d bytes memory", memsize);
    #else
    LOGMEV("Skipping due to missing ANABRID_ENABLE_GLOBAL_PLUGIN_LOADER", "dummy");
    #endif
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

std::string shortened_hexdump(uint8_t* mem, size_t size) {
    constexpr size_t inspect_bytes = 3;
    char ret[2*inspect_bytes + 3]; // AABBCC...DDEEFF
    for(size_t i=0; i<inspect_bytes; i++) sprintf(ret+2*i, "%x", mem[i]);
    for(size_t i=0; i<inspect_bytes; i++) sprintf(ret+sizeof(ret)-2*(i+1), "%x", mem[i]);
    return std::string(ret);
}

void loader::convertToJson(const GlobalPluginLoader &src, JsonVariant dst) {
    dst["type"] = "GlobalPluginLoader";
    #ifndef ANABRID_ENABLE_GLOBAL_PLUGIN_LOADER
    dst["hint"] = "Set ANABRID_ENABLE_GLOBAL_PLUGIN_LOADER debug feature flag to enable this loader";
    #else
    dst["load_addr"] = (uint32_t) src.load_addr;
    dst["memsize"] = src.memsize;

    dst["can_load"] = src.can_load();
    dst["is_loaded"] = src.plugin.has_value();

    if(src.plugin) {
        auto plugin = dst.createNestedObject("plugin");
        plugin["size"] = src.plugin->size;
        utils::sha256 plugin_checksum(src.plugin->load_addr, src.plugin->size);
        plugin["sha256sum"] = plugin_checksum.to_string();

        // can probably skip that
        plugin["hexdump"] = shortened_hexdump(src.plugin->load_addr, src.plugin->size);
    }
    #endif
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
    if(plugin) return_err("Already have plugin loaded, can only load one plugin at a time. Call unload before.");
    if(!can_load()) return_err("PluginLoader cannot load code. This is currently most likely due to the missing compile time flag ANABRID_ENABLE_GLOBAL_PLUGIN_LOADER");
    if(!msg_in.containsKey("entry")) return_err("Requiring entry point \"entry\"=0x1234 even if it is just 0.");

    // Load address verification, to ensure correct linking
    if(msg_in["load_addr"] != load_addr) return_err("Require matching load address for verification. This SinglePluginLoader can only load from: [todo]");
    auto firmeware_hash = sha256_to_string(hash_flash_sha256());
    if(utils::sha256_test_short(msg_in["firmware_sha256"], firmeware_hash)) return_err("ABI mismatch: You built against [your firmware sha256] but we run [our sha256]");

    // Function pointer registration
    Plugin new_plugin;
    new_plugin.load_addr = load_addr;
    new_plugin.entry = msg_in["entry"];
    if(msg_in.containsKey("exit")) {
        new_plugin.exit = msg_in["exit"].as<Function>();
    }

    // Actual code submission
    // For the time being, expect base64 encoded data.
    // TODO: For slightly larger payloads (mind the typical 1024byte JsonObject maximum size in the code),
    //       a multi-step transmission has to be introduced.
    auto base64_payload = msg_in["payload"].as<std::string>();
    new_plugin.size = base64_payload.size() / 4 * 3;
    if(!load(new_plugin)) return_err("Payload too large or some code is already loaded.");
    auto length_decoded = etl::base64::decode(base64_payload.c_str(), base64_payload.length(), new_plugin.load_addr, new_plugin.size);
    if(length_decoded != new_plugin.size) { unload(); return_err("Could not decode base64-encoded payload."); }

    // Code execution
    uint8_t* entry_point = assert_callable(plugin->load_addr + plugin->entry.addr);
    if(!entry_point) { unload(); return_err("Entry point address not properly aligned"); }
    dispatch(entry_point, plugin->entry.ret_type, msg_out["returns"].as<JsonVariant>());

    // Plugins which are actually RPC.
    if(msg_in.containsKey("immediately_unload")) unload();

    return true;
}

bool loader::SinglePluginLoader::unload(JsonObjectConst msg_in, JsonObject &msg_out) {
    if(!plugin) { return_err("Cannot unload, nothing loaded"); }
    if(plugin->exit) {
        // Code execution
        uint8_t* exit_point = assert_callable(plugin->load_addr + plugin->exit->addr);
        if(!exit_point) { unload(); return_err("Exit point address not properly aligned"); }
        dispatch(exit_point, plugin->exit->ret_type, msg_out["returns"].as<JsonVariant>());
    }
    unload();
    return true;
}


bool loader::SinglePluginLoader::load(const Plugin &new_plugin) {
    if(plugin) return false; // call unload() first.
    if(new_plugin.size > memsize) return false;
    plugin = new_plugin; // copy
    return true;
}


PluginLoader _loader; // Singleton here just for the message handlers.

bool msg::handlers::LoadPluginHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
    return _loader.load_and_execute(msg_in, msg_out);
}

bool msg::handlers::UnloadPluginHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
    return _loader.unload(msg_in, msg_out);
}

bool msg::handlers::PluginStatusHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
    auto loader_list = msg_out.createNestedArray("loaders");
    loader_list.add(_loader);

    auto oflash = msg_out.createNestedObject("flashimage");
    oflash["size"] = utils::flashimagelen();
    oflash["sha256sum"] = utils::sha256_to_string(utils::hash_flash_sha256());
    return true;
}

