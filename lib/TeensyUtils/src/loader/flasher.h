#ifndef FLASHERX_H
#define FLASHERX_H

#include "loader/hashflash.h"

namespace loader {

  // Just a little generic Teensy reboot routine
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
   **/
  struct FirmwareBuffer {
    std::string name; // only used for name association
    size_t imagelen=0;
    utils::sha256_t upstream_hash;

    size_t bytes_transmitted=0; // (4-aligned)
    bool transfer_completed() { return imagelen == bytes_transmitted; }

    uint32_t buffer_addr=0, ///< buffer on flash for new image
             buffer_size=0; ///< identical to imagelen but increaed to next sector boundary

    FirmwareBuffer();
    ~FirmwareBuffer();
  };

  class FirmwareFlasher {
    FirmwareBuffer *upgrade;
    void delete_upgrade() { delete upgrade; upgrade = nullptr; }
  public:
    int init(JsonObjectConst msg_in, JsonObject &msg_out);
    int stream(JsonObjectConst msg_in, JsonObject &msg_out);
    int abort(JsonObjectConst msg_in, JsonObject &msg_out);
    int complete(JsonObjectConst msg_in, JsonObject &msg_out);
    void status(JsonObject& msg_out);

    static FirmwareFlasher& get();
  };

} // ns loader



#endif /* FLASHERX_H */
