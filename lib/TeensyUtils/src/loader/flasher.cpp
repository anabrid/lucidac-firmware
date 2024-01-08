//******************************************************************************
// Flash write/erase functions (TLC/T3x/T4x/TMM), LMEM cache functions for T3.6
//******************************************************************************
// WARNING:  you can destroy your MCU with flash erase or write!
// This code may or may not protect you from that.
//
// Original by Niels A. Moseley, 2015.
// Modifications for OTA updates by Jon Zeeff, Deb Hollenback
// Paul Stoffregen's T4.x flash routines from Teensy4 core added by Jon Zeeff
// Frank Boesing's T3.x flash routines adapted for OTA by Joe Pasquariello
// Largely adapted and rewritten for the Anabrid REDAC infrastructure by SvenK
// This code is released into the public domain.
//******************************************************************************

#include <stdint.h>     // uint32_t, etc.
#include <Arduino.h>	// Serial, etc. (if used)
#include <malloc.h>		// malloc(), free()
#include <string.h>		// memset()

#include "utils/etl_base64.h"
#include "loader/flasher.h"
#include "utils/logging.h"
#include "flasher.h"


// [<------- code ------->][<--------- buffer --------->][<-- FLASH_RESERVE -->]
// [<------------------------------ FLASH_SIZE ------------------------------->]
// ^FLASH_BASE_ADDR

// teensy is __IMXRT1062__
#define FLASH_ID          "fw_teensy41"  // target ID (in code)
#define FLASH_ID_LEN      (11)           // target ID (in code)
#define FLASH_SIZE        (0x800000)     // 8MB
#define FLASH_SECTOR_SIZE (0x1000)       // 4KB sector size
#define FLASH_WRITE_SIZE  (4)            // 4-byte/32-bit writes
// teensy has 256KB reserve top of flash for EEPROM+LED blink restore program
// however, we make another big 256KB reserve because of LittleFS storage below.
#define FLASH_RESERVE     (80*FLASH_SECTOR_SIZE)
#define FLASH_BASE_ADDR   (0x60000000)   // code starts here

#define IN_FLASH(a) ((a) >= FLASH_BASE_ADDR && (a) < FLASH_BASE_ADDR + FLASH_SIZE)

// reboot is the same for all ARM devices
#define CPU_RESTART_ADDR	((uint32_t *)0xE000ED0C)
#define CPU_RESTART_VAL		(0x5FA0004)
#define REBOOT			      (*CPU_RESTART_ADDR = CPU_RESTART_VAL)

void loader::reboot() { REBOOT; }

#define RAMFUNC __attribute__ ((section(".fastrun"), noinline, noclone, optimize("Os") ))

RAMFUNC int flash_sector_not_erased( uint32_t address );

// from cores\Teensy4\eeprom.c  --  use these functions at your own risk!!!
extern "C" {
void eepromemu_flash_write(void *addr, const void *data, uint32_t len);
void eepromemu_flash_erase_sector(void *addr);
void eepromemu_flash_erase_32K_block(void *addr);
void eepromemu_flash_erase_64K_block(void *addr);
}

// functions used to move code from buffer to program flash (must be in RAM)
RAMFUNC void flash_move( uint32_t dst, uint32_t src, uint32_t size );

// functions that can be in flash
void firmware_buffer_free( uint32_t buffer_addr, uint32_t buffer_size );
int  flash_write_block( uint32_t addr, char *data, uint32_t count );
int  flash_erase_block( uint32_t address, uint32_t size );

static int leave_interrupts_disabled = 0;

// compute addr/size for firmware buffer and return NO/RAM/FLASH_BUFFER_TYPE
// buffer will begin at first sector ABOVE code and below FLASH_RESERVE
// start at bottom of FLASH_RESERVE and work down until non-erased flash found
void firmware_buffer_init(uint32_t *buffer_addr, uint32_t *buffer_size )
{
  *buffer_addr = FLASH_BASE_ADDR + FLASH_SIZE - FLASH_RESERVE - 4;
  LOGMEV("Search starting at 0x%0X", *buffer_addr);
  while (*buffer_addr > 0 && *((uint32_t *)*buffer_addr) == 0xFFFFFFFF)
    *buffer_addr -= 4;
  *buffer_addr += 4; // first address above code
  LOGMEV("Found first non-erased flash byte at 0x%0X", *buffer_addr);

  // increase buffer_addr to next sector boundary (if not on a sector boundary)
  if ((*buffer_addr % FLASH_SECTOR_SIZE) > 0)
    *buffer_addr += FLASH_SECTOR_SIZE - (*buffer_addr % FLASH_SECTOR_SIZE);
  *buffer_size = FLASH_BASE_ADDR - *buffer_addr + FLASH_SIZE - FLASH_RESERVE;
}

/*
 * TODO: It is currently very important that any started upload is either completed or aborted.
 *       Abandoned uploads lead to leftovers in the FLASH which hinder the current buffer detection scheme
 *       to identify them as available.
 * 
 * It is quite easy to do this: Just use utils::flashimagelen() and erase everything
 * until the FLASH_RESERVE. That should probably be a measure to take if the buffer init
 * fails.
 *
 */

loader::FirmwareBuffer::FirmwareBuffer() {
  // TODO: This function should look for leftovers of abandoned file transfers, see above.
  firmware_buffer_init(&buffer_addr, &buffer_size);
}


loader::FirmwareBuffer::~FirmwareBuffer() { firmware_buffer_free(buffer_addr, buffer_size); }

void firmware_buffer_free( uint32_t buffer_addr, uint32_t buffer_size )
{
  flash_erase_block( buffer_addr, buffer_size );
}


//******************************************************************************
// flash_sector_not_erased()	returns 0 if erased and !0 (error) if NOT erased
//******************************************************************************
RAMFUNC int flash_sector_not_erased( uint32_t address )
{
  uint32_t *sector = (uint32_t*)(address & ~(FLASH_SECTOR_SIZE - 1));
  for (int i=0; i<FLASH_SECTOR_SIZE/4; i++) {
    if (*sector++ != 0xFFFFFFFF)
      return 1; // NOT erased
  }
  return 0; // erased
}

//******************************************************************************
// move from source to destination (flash), erasing destination sectors as we go
// DANGER: this is critical and cannot be interrupted, else T3.x can be damaged
//******************************************************************************
RAMFUNC void flash_move( uint32_t dst, uint32_t src, uint32_t size )
{
  uint32_t offset=0, error=0, addr;
  
  // set global flag leave_interrupts_disabled = 1 to prevent the T3.x flash
  // write and erase functions from re-enabling interrupts when they complete 
  leave_interrupts_disabled = 1;
  
  // move size bytes containing new program from source to destination
  while (offset < size && error == 0) {

    addr = dst + offset;

    // if new sector, erase, then immediately write FSEC/FOPT if in this sector
    // this is the ONLY place that FSEC values are written, so it's the only
    // place where calls to KINETIS flash write functions have aFSEC = oFSEC = 1
    if ((addr & (FLASH_SECTOR_SIZE - 1)) == 0) {
      if (flash_sector_not_erased( addr )) {
        #if defined(__IMXRT1062__)
          eepromemu_flash_erase_sector( (void *)addr );
        #elif (FLASH_WRITE_SIZE==4)
          error |= flash_erase_sector( addr, 1 );
          if (addr == (0x40C & ~(FLASH_SECTOR_SIZE - 1)))
            error |= flash_word( 0x40C, 0xfffff9de, 1, 1 );
        #elif (FLASH_WRITE_SIZE==8)
          error |= flash_erase_sector( addr, 1 );
          if (addr == (0x408 & ~(FLASH_SECTOR_SIZE - 1)))
            error |= flash_phrase( 0x408, 0xfffff9deffffffff, 1, 1 );
        #endif
      }
    }
    
    // for KINETIS, these writes may be to the sector containing FSEC, but the
    // FSEC location was written by the code above, so use aFSEC=1, oFSEC=0
    #if defined(__IMXRT1062__)
      // for T4.x, data address passed to flash_write() must be in RAM
      uint32_t value = *(uint32_t *)(src + offset);     
      eepromemu_flash_write( (void*)addr, &value, 4 );
    #elif (FLASH_WRITE_SIZE==4)
      error |= flash_word( addr, *(uint32_t *)(src + offset), 1, 0 );
    #elif (FLASH_WRITE_SIZE==8)
      error |= flash_phrase( addr, *(uint64_t *)(src + offset), 1, 0 );
    #endif

    offset += FLASH_WRITE_SIZE;
  }
  
  // move is complete. if the source buffer (src) is in FLASH, erase the buffer
  // by erasing all sectors from top of new program to bottom of FLASH_RESERVE,
  // which leaves FLASH in same state as if code was loaded using TeensyDuino.
  // For KINETIS, this erase cannot include FSEC, so erase uses aFSEC=0.
  if (IN_FLASH(src)) {
    while (offset < (FLASH_SIZE - FLASH_RESERVE) && error == 0) {
      addr = dst + offset;
      if ((addr & (FLASH_SECTOR_SIZE - 1)) == 0) {
        if (flash_sector_not_erased( addr )) {
          #if defined(__IMXRT1062__)
            eepromemu_flash_erase_sector( (void*)addr );
          #else
            error |= flash_erase_sector( addr, 0 );
          #endif
	}
      }
      offset += FLASH_WRITE_SIZE;
    }   
  }

  // for T3.x, at least, must REBOOT here (via macro) because original code has
  // been erased and overwritten, so return address is no longer valid
  REBOOT;
  // wait here until REBOOT actually happens 
  for (;;) {}
}

//******************************************************************************
// flash_erase_block()	erase sectors from (start) to (start + size)
//******************************************************************************
int flash_erase_block( uint32_t start, uint32_t size )
{
  int error = 0;
  uint32_t address = start;
  while (address < (start + size) && error == 0) { 
    if ((address & (FLASH_SECTOR_SIZE - 1)) == 0) {
      if (flash_sector_not_erased( address )) {
	#if defined(__IMXRT1062__)
          eepromemu_flash_erase_sector( (void*)address );
        #elif defined(KINETISK) || defined(KINETISL)
          error = flash_erase_sector( address, 0 );
	#endif
      }
    }
    address += FLASH_SECTOR_SIZE;
  }
  return( error );
}

//******************************************************************************
// take a 32-bit aligned array of 32-bit values and write it to erased flash
//******************************************************************************
int flash_write_block( uint32_t addr, uint8_t *data, uint32_t count )
{
  if(!IN_FLASH(addr)) return 4; // sanity check
  // static (aligned) variables to guarantee 32-bit or 64-bit-aligned writes
  #if (FLASH_WRITE_SIZE == 4)				// #if 4-byte writes
  static uint32_t buf __attribute__ ((aligned (4)));	//   4-byte buffer
  #elif (FLASH_WRITE_SIZE == 8)				// #elif 8-byte writes
  static uint64_t buf __attribute__ ((aligned (8)));	//   8-byte buffer
  #endif						//
  static uint32_t buf_count = 0;			// bytes in buffer
  static uint32_t next_addr = 0;			// expected address
  
  int ret = 0;						// return value
  uint32_t data_i = 0;					// index to data array

  if ((addr % 4) != 0 || (count % 4) != 0) {		// if not 32-bit aligned
    return 1;	// "flash_block align error\n"		//   return error code 1
  }

  if (buf_count > 0 && addr != next_addr) {		// if unexpected address   
    return 2;	// "unexpected address\n"		//   return error code 2   
  }
  next_addr = addr + count;				//   compute next address
  addr -= buf_count;					//   address of data[0]

  while (data_i < count) {				// while more data
    ((uint8_t*)&buf)[buf_count++] = data[data_i++];	//   copy a byte to buf
    if (buf_count < FLASH_WRITE_SIZE) {			//   if buf not complete
      continue;						//     continue while()
    }							//   
    #if defined(__IMXRT1062__)				//   #if T4.x 4-byte
      eepromemu_flash_write((void*)addr,(void*)&buf,4);	//     flash_write()
    #elif (FLASH_WRITE_SIZE==4)				//   #elif T3.x 4-byte 
      ret = flash_word( addr, buf, 0, 0 );		//     flash_word()
    #elif (FLASH_WRITE_SIZE==8)				//   #elif T3.x 8-byte
      ret = flash_phrase( addr, buf, 0, 0 );		//     flash_phrase()
    #endif
    if (ret != 0) {					//   if write error
      return 3;	// "flash write error %d\n"		//     error code
    }
    buf_count = 0;					//   re-init buf count
    addr += FLASH_WRITE_SIZE;				//   advance address
  }  
  return 0;						// return success
}

//******************************************************************************
// Actual user frontend code
//******************************************************************************

constexpr static int success = 0;
#define return_err(code, msg) { msg_out["error"] = msg; return code; }
#define return_errf(code, msg,...) {\
  char msgbuf[1000]; snprintf(msgbuf, sizeof(msgbuf), msg, __VA_ARGS__);\
  msg_out["error"] = msgbuf; return code; }


loader::FirmwareFlasher _flasher;
loader::FirmwareFlasher &loader::FirmwareFlasher::get() {
  return _flasher;
}


// no of base64 symbols to transfer in a single chunk. Note that this unit is limited by the
// maximum JSON Documentsize anyway, which is currently 4096 bytes.
// Note: number_of_bytes = max_chunk_size * 3/4 and we require number_of_bytes%4 == 0.
// TODO: Communicate this somewhere centrally
constexpr size_t static json_overhead = 0xff; // for {'bla'}
constexpr size_t static base64_chunk_size = 4096 / 2; //- json_overhead;
constexpr size_t static bin_chunk_size = base64_chunk_size * 3/4;

int loader::FirmwareFlasher::init(JsonObjectConst msg_in, JsonObject &msg_out) {
  if(upgrade) return_err(1, "Firmware upgrade already running. Cancel it before doing another one");

  upgrade = new loader::FirmwareBuffer();
  upgrade->name = msg_in["name"].as<std::string>();
  upgrade->imagelen = msg_in["imagelen"];
  upgrade->upstream_hash = utils::parse_sha256(msg_in["upstream_hash"]);
  upgrade->bytes_transmitted = 0;

  if(upgrade->imagelen > upgrade->buffer_size) {
    delete_upgrade();
    return_errf(2, "New firmware image too large for OTA upgrade process. Image size: %zu bytes > Buffer size: %ld bytes",
      upgrade->imagelen, upgrade->buffer_size);
  }

  // Tell the client a suitable chunk size (in base64-encoded).
  // The JsonDocuments in main.cpp have length 4096.
  msg_out["bin_chunk_size"] = bin_chunk_size;
  msg_out["encoding"] = "binary-base64";
  return success;
}

int loader::FirmwareFlasher::stream(JsonObjectConst msg_in, JsonObject &msg_out) {
  if(!upgrade) return_err(1, "No firmware upgrade running.");
  if(upgrade->transfer_completed()) return_err(2, "Transfer already completed");

  uint8_t decoded_buffer[bin_chunk_size] __attribute__ ((aligned (4)));
  auto base64_payload = msg_in["payload"].as<std::string>();
  auto payload_base64_size = base64_payload.size(); // / 4 * 3;
  if(!payload_base64_size) return_err(3, "Missing payload.");
  if((payload_base64_size % 4) != 0) return_err(4, "Payload has not correct length to be base64-encoded.");
    //return_err("Chunk not 32-bit aligned. Expecting buffer size multiples of 4 bytes.");
  auto payload_bin_size = etl::base64::decode(base64_payload.c_str(), base64_payload.length(), decoded_buffer, bin_chunk_size);
  if(!payload_bin_size) return_err(2, "Could not decode base64-encoded payload.");

  LOGV("payload_base64_size=%d, payload_bin_size=%d", payload_base64_size, payload_bin_size);

  if(payload_bin_size > bin_chunk_size)
    return_errf(6, "Chunk size too large. Given %zu bytes > buffer size %zu bytes", payload_bin_size, bin_chunk_size);
  if(upgrade->bytes_transmitted + payload_bin_size > upgrade->imagelen)
    return_errf(7, "Chunk size too large, yields to image size %zu bytes but only %zu announced.", upgrade->bytes_transmitted + payload_bin_size, upgrade->imagelen);
  if((payload_bin_size % 4) != 0)
    return_err(8, "Chunk not 32-bit aligned. Expecting buffer size multiples of 4 bytes.");

  LOGV("Loaded %zu bytes to RAM, ready for flash writing", payload_bin_size);

  // The write bases on the assumption that the buffer is completely erased.
  int write_error = flash_write_block(upgrade->buffer_addr + upgrade->bytes_transmitted, decoded_buffer, payload_bin_size);
  if(write_error) return_errf(50+write_error, "flash_write_block() error %02X", write_error);

  upgrade->bytes_transmitted += payload_bin_size;
  return success;
}

void loader::FirmwareFlasher::status(JsonObject &msg_out) {
  msg_out["is_upgrade_running"] = bool(upgrade);
  if(upgrade) {
    msg_out["name"] = upgrade->name;
    msg_out["size"] = upgrade->imagelen;
    msg_out["upstream_hash"] = utils::sha256_to_string(upgrade->upstream_hash);
    msg_out["bytes_transmitted"] = upgrade->bytes_transmitted;
    msg_out["transfer_completed"] = upgrade->transfer_completed();

    if(upgrade->transfer_completed()) {
      auto actual_hash = utils::hash_sha256((uint8_t*)upgrade->buffer_addr, upgrade->imagelen);
      msg_out["hash_correct"] = actual_hash == upgrade->upstream_hash;
      // the following is mostly for debugging
      msg_out["actual_hash"] = utils::sha256_to_string(actual_hash);
    }

    // just to always have this information, cf below
    msg_out["buffer_addr"] = upgrade->buffer_addr;
    msg_out["buffer_size"] = upgrade->buffer_size;
  } else {
    // inform about upgrade capabilities
    uint32_t potential_buffer_addr, potential_buffer_size;
    firmware_buffer_init(&potential_buffer_addr, &potential_buffer_size); // is non-destructive = w/o side effects
    msg_out["buffer_addr"] = potential_buffer_addr;
    msg_out["buffer_size"] = potential_buffer_size;
  }
}

int loader::FirmwareFlasher::abort(JsonObjectConst msg_in, JsonObject &msg_out) {
  if(!upgrade) return_err(1, "No upgrade running which I could abort");
  delete_upgrade();
  return true;
}

int loader::FirmwareFlasher::complete(JsonObjectConst msg_in, JsonObject &msg_out) {
  if(!upgrade->transfer_completed()) return_err(1,"Require transfer to be completed");

  LOG_ALWAYS("Checking new firmware image hash...");
  auto actual_hash = utils::hash_sha256((uint8_t*)upgrade->buffer_addr, upgrade->imagelen);
  if(actual_hash != upgrade->upstream_hash) return_err(2,"Corrupted upload data, hash mismatch. Please try again (start new init)");

  LOG_ALWAYS("Moving new firmware image into place and rebooting...");

  // move new program from buffer to flash, free buffer, and reboot
  flash_move( FLASH_BASE_ADDR, upgrade->buffer_addr, upgrade->imagelen);

  // will not return from flash_move(). This is actually never reached.
  // return statement only to surpress compiler warnings.
  return success;
}

