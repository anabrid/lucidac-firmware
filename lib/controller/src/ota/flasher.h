#ifndef FLASHERX_H
#define FLASHERX_H

#include "utils/hashflash.h"
#include "utils/singleton.h"

namespace loader {

  /// Just a little generic Teensy reboot routine
  void reboot();

  /**
   * The Flasher-X over-the-air firmware upgrade process is a small code snippet originally dating from
   * https://github.com/joepasquariello/FlasherX. The basic idea is to use the free flash image as a buffer
   * since the RAM is (typically) not big enough to hold the new firmware. The swap is then done in an
   * "atomic" step and the teensy is subsequently rebooted. All this happens without the dedicated teensy
   * flashing chip, which also means that if this method miserably fails, the teensy can just be flashed
   * via ordinary USB and no hardware damage can happen.
   * 
   * This FirmwareBuffer class represents a running upgrade process and the second firmware image stored
   * within the upper flash region during a running upgrade process. Given that only one flash OTA upgrade
   * can run per time, this is a singleton.
   * 
   * The primary interface to the over-the-air firmware upgrade are the messages below, so this class is
   * actually only exposed to make a bit more transparent to the user what kind of data are stored during
   * an update.
   * 
   * \ingroup Singletons
   **/
  struct FirmwareBuffer {
    std::string name; ///< only used for name association
    size_t imagelen=0; ///< Size of image as defined by user, for checking after transmission
    utils::sha256_t upstream_hash; ///< Hash as defined by user, for checking after transmission

    size_t bytes_transmitted=0; // (4-aligned)
    bool transfer_completed() { return imagelen == bytes_transmitted; }

    uint32_t buffer_addr=0, ///< buffer on flash for new image
             buffer_size=0; ///< identical to imagelen but increaed to next sector boundary

    FirmwareBuffer();
    ~FirmwareBuffer();
  };

  /**
   * Actual "frontend" for flashing.
   * 
   * This code is proof-of-concept like: It works but it is horribly slow due to the protocol
   * overhead.
   * 
   * TODO: Transfer firmware image with direct stream access (API is now present here) or
   *       via side channel such as a HTTP (not https) URL where this teensy downloads it.
   * 
   * \ingroup Singletons
   **/
  class FirmwareFlasher : public utils::HeapSingleton<FirmwareFlasher> {
    FirmwareBuffer *upgrade;
    void delete_upgrade() { delete upgrade; upgrade = nullptr; } ///< Abort an running upgrade
  public:
    ///@addtogroup User-Functions
    ///@{
    int init(JsonObjectConst msg_in, JsonObject &msg_out);
    int stream(JsonObjectConst msg_in, JsonObject &msg_out);
    int abort(JsonObjectConst msg_in, JsonObject &msg_out);
    int complete(JsonObjectConst msg_in, JsonObject &msg_out);
    void status(JsonObject& msg_out);
    ///@}
  };

} // ns loader



#endif /* FLASHERX_H */
