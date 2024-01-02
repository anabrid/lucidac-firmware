#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

#include "ArduinoJson.h"
#include "message_handlers.h"

namespace loader {
    
    /**
     * A jumpable function, ie something with a signature "ret_type foo();", located at relative or absolute
     * address addr. I introduced some kind of clumsy return-value dispatch which probably will be replaced just
     * by "either JsonObject or void" in the future.
     * 
     * Could also support calling arguments such as ... well the same, basically. And then any combination :D
     **/
    struct Function {
        uint32_t addr = 0x0;
        enum class Returns { None, Bool, Int, String, JsonObject } ret_type = Returns::None;

        typedef void (None_f)(); typedef bool (Bool_f)();  typedef int (Int_f)();
        typedef std::string (String_f)(); typedef JsonObject (JsonObject_f)();
    };
    
    void convertFromJson(JsonVariantConst src, Function& f);

    /**
     * A plugin (a synonym could also be "extension" or "module") is a small piece of user-defined code
     * which he can send to the teensy at runtime where it gets executed. Plugins are written in C++ and
     * have full access to the firmware API/ABI. This allows to write high level plugins which will feel
     * like Arduino sketches or scripts.
     * Technically, plugins are just compiled and statically linked machine code. That also means this
     * code can do anything it wants on the teensy, such as directly communicating with the LUCIDAC blocks,
     * registering message handlers, doing any kind of network or Serial line access, allocating heap
     * memory, reading and writing the flash, etc.
     * The fact that we use C++ as scripting language makes it *very* easy to expose the full API available.
     * Since there is nothing between the loaded plugin and the firmware, the user has to be 100% sure that
     * his ABI version matches the one running on the teensy. This is supported with SHA-2 hashes.
     * 
     * This plugin structure is used by Plugin Loaders in order to remember entry and exit point addresses.
     * Plugins are thought like C++ classes, where the entry point is the constructor and the exit point
     * is the destructor. If Plugins want to have further communication with the user, they have to use
     * message handlers on their own.
     **/
    struct Plugin {
        uint8_t *load_addr=0; //< Absolute address in memory where the plugin is loaded to.
        uint32_t size=0;      //< Actual size of plugin memory image
        Function entry, exit;  //< entry and exit points. their addr are relative to load_addr.
        bool has_exit=false;   //< Whether the exit Function exists. Mimics std::optional<Function> exit.

        bool can_load() { return !load_addr || size; }
        bool is_loaded() { return size; }
      
        // should write a signature function here which returns a checksum like CRC.
    };
    
    /**
     * The SinglePluginLoader can only load a single plugin at a given time. He is typically limited by
     * his memory size and layout, i.e. he does not use some memory mangament data structure (a heap, stack, tree, etc).
     * Currently there is only one implementation, anyway.
     **/
    struct SinglePluginLoader {
        Plugin plugin;  //< The single plugin managed. It's load_addr defines the memory address.
        int memsize;    //< The maximum memory size managed/accessible/available by this loader
        
        bool can_load() { return plugin.can_load(); }
        virtual bool load(uint8_t* code, size_t code_len); // convenience function, put code in place, returns true if success
        virtual bool load(size_t code_len);
        void unload() { plugin.size = 0; } // frees memory, does not call unloader.
        
        virtual bool load_and_execute(JsonObjectConst msg_in, JsonObject &msg_out); /// load from protocol message, returns a reply msg
        //virtual void show(JsonObjectConst msg_in, JsonObject &msg_out); /// show what is currently there
        virtual bool unload(JsonObjectConst msg_in, JsonObject &msg_out); /// unload from protocol message, returns a reply msg
    };
    
    /**
     * Reserves storage in the data segment (address space that constains static variables,
     * i.e. global variables or static local variables) and uses that storage for loading
     * plugin code at runtime.
     * 
     * Pros: (1.) Address is well-known at compile time for a given firmware, this simplifies
     *       linking of plugin code.
     *       (2.) Plugin loader communcation is reduced thanks to fixed address.
     * Cons: (1.) Storage is always reserved regardless of whether used or not
     *       (2.) Cannot grow (cannot hold bigger code then the reserved memory)
     * 
     * Use the compile time constant
     * ANABRID_ENABLE_GLOBAL_PLUGIN_LOADER to enable this mechanism.
     **/
    struct GlobalPluginLoader : public SinglePluginLoader {
        GlobalPluginLoader();
    };

    using PluginLoader = GlobalPluginLoader;

    
    /*
    
    It is not so hard to come up with other loaders, such as ones reserving space on the
      - Heap (can load multiple plugins, each of differing size)
        Difficulties: Have to disable RAM2 cache for allowing data be executed there
      - unreserved RAM1 just between stack and data segment (can run a heap there
        or just use memory only when neccessary)
        Difficulties: Have to be sure not to mess up with the regular stack
    or other things. The above GlobalPluginLoader is just one example.
    
    */
} // end of namespace loader


namespace msg {

namespace handlers {


class LoadPluginHandler : public MessageHandler {
public:
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

class UnloadPluginHandler : public MessageHandler {
public:
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};


} // namespace handlers

} // namespace msg



#endif /* PLUGIN_LOADER_H */

