//******************************************************************************
// FXUTIL.H -- FlasherX utility functions
//******************************************************************************
#include <Arduino.h>
#include "FXUtil.h"
extern "C" {
  #include "FlashTxx.h"		// TLC/T3x/T4x/TMM flash primitives
}

#define fx_min(a, b) ((a) < (b) ? (a) : (b))

//******************************************************************************
// update_firmware()	read hex file and write new firmware to program flash
//******************************************************************************
void update_firmware( Stream *in, Stream *out, 
				uint32_t buffer_addr, uint32_t buffer_size )
{
  static char line[HEX_LINE_MAX_SIZE];                // buffer for hex lines
  static char data[HEX_DATA_MAX_SIZE] __attribute__((aligned(8))); // buffer for hex data
  hex_info_t hex;
  hex.data = data;
  hex.addr = 0;
  hex.code = 0;
  hex.num = 0;
  hex.base = 0;
  hex.min = 0xFFFFFFFF;
  hex.max = 0;
  hex.eof = 0;
  hex.lines = 0;
  hex.prevDataLen = 0;

  out->printf( "reading hex lines...\n" );

  // read and process intel hex lines until EOF or error
  while (!hex.eof)  {

    read_ascii_line( in, line, sizeof(line) );
    // reliability of transfer via USB is improved by this printf/flush
    if (in == out && out == (Stream*)&Serial) {
      out->printf( "%s\n", line );
      out->flush();
    }

    if (parse_hex_line( (const char*)line, hex.data, &hex.addr, &hex.num, &hex.code ) == 0) {
      out->printf( "abort - bad hex line %s\n", line );
    }
    else if (process_hex_record( &hex ) != 0) { // error on bad hex code
      out->printf( "abort - invalid hex code %d\n", hex.code );
      return;
    }
    else if (hex.code == 0) { // if data record
      uint32_t addr = buffer_addr + hex.base + hex.addr - FLASH_BASE_ADDR;
      if (hex.max > (FLASH_BASE_ADDR + buffer_size)) {
        out->printf( "abort - max address %08lX too large\n", hex.max );
        return;
      }
      else if (!IN_FLASH(buffer_addr)) {
        memcpy( (void*)addr, (void*)hex.data, hex.num );
      }
      else if (IN_FLASH(buffer_addr)) {
        int error = flash_write_block( addr, hex.data, hex.num );
        if (error) {
          out->printf( "abort - error %02X in flash_write_block()\n", error );
          return;
        }
      }
    }
    hex.lines++;
  }
    
  out->printf( "\nhex file: %1d lines %1lu bytes (%08lX - %08lX)\n",
			hex.lines, hex.max-hex.min, hex.min, hex.max );

  // check FSEC value in new code -- abort if incorrect
  #if defined(KINETISK) || defined(KINETISL)
  uint32_t value = *(uint32_t *)(0x40C + buffer_addr);
  if (value == 0xfffff9de) {
    out->printf( "new code contains correct FSEC value %08lX\n", value );
  }
  else {
    out->printf( "abort - FSEC value %08lX should be FFFFF9DE\n", value );
    return;
  } 
  #endif

  // check FLASH_ID in new code - abort if not found
  if (check_flash_id( buffer_addr, hex.max - hex.min )) {
    out->printf( "new code contains correct target ID %s\n", FLASH_ID );
  }
  else {
    out->printf( "abort - new code missing string %s\n", FLASH_ID );
    return;
  }
  
  // get user input to write to flash or abort
  int user_lines = -1;
  while (user_lines != hex.lines && user_lines != 0) {
    out->printf( "enter %d to flash or 0 to abort\n", hex.lines );
    read_ascii_line( out, line, sizeof(line) );
    sscanf( line, "%d", &user_lines );
  }
  
  if (user_lines == 0) {
    out->printf( "abort - user entered 0 lines\n" );
    return;
  }
  else {
    out->printf( "calling flash_move() to load new firmware...\n" );
    out->flush();
  }
  
  // move new program from buffer to flash, free buffer, and reboot
  flash_move( FLASH_BASE_ADDR, buffer_addr, hex.max-hex.min );

  // should not return from flash_move(), but put REBOOT here as reminder
  REBOOT;
}

//******************************************************************************
// read_ascii_line()	read ascii characters until '\n', '\r', or max bytes
//******************************************************************************
void read_ascii_line( Stream *serial, char *line, int maxbytes )
{
  int c=0, nchar=0;
  while (serial->available()) {
    c = serial->read();
    if (c == '\n' || c == '\r')
      continue;
    else {
      line[nchar++] = c;
      break;
    }
  }
  while (nchar < maxbytes && !(c == '\n' || c == '\r')) {
    if (serial->available()) {
      c = serial->read();
      line[nchar++] = c;
    }
  }
  line[nchar-1] = 0;	// null-terminate
}

//******************************************************************************
// process_hex_record()		process record and return okay (0) or error (1)
//******************************************************************************
int process_hex_record( hex_info_t *hex )
{
  if (hex->code==IRT_DATA) { // data -- update min/max address so far
    if (hex->base + hex->addr + hex->num > hex->max)
      hex->max = hex->base + hex->addr + hex->num;
    if (hex->base + hex->addr < hex->min)
      hex->min = hex->base + hex->addr;
  }
  else if (hex->code==IRT_EOF) { // EOF (:flash command not received yet)
    hex->eof = 1;
  }
  else if (hex->code==IRT_ES_ADDR) { // extended segment address (top 16 of 24-bit addr)
    hex->base = ((hex->data[0] << 8) | hex->data[1]) << 4;
  }
  else if (hex->code==IRT_SS_ADDR) { // start segment address (80x86 real mode only)
    return 1;
  }
  else if (hex->code==IRT_EL_ADDR) { // extended linear address (top 16 of 32-bit addr)
    hex->base = ((hex->data[0] << 8) | hex->data[1]) << 16;
  }
  else if (hex->code==IRT_SL_ADDR) { // start linear address (32-bit big endian addr)
    hex->base = (hex->data[0] << 24) | (hex->data[1] << 16)
              | (hex->data[2] <<  8) | (hex->data[3] <<  0);
  }
  else {
    return 1;
  }

  return 0;
}

//******************************************************************************
// Intel Hex record foramt:
//
// Start code:  one character, ASCII colon ':'.
// Byte count:  two hex digits, number of bytes (hex digit pairs) in data field.
// Address:     four hex digits
// Record type: two hex digits, 00 to 05, defining the meaning of the data field.
// Data:        n bytes of data represented by 2n hex digits.
// Checksum:    two hex digits, computed value used to verify record has no errors.
//
// Examples:
//  :10 9D30 00 711F0000AD38000005390000F5460000 35
//  :04 9D40 00 01480000 D6
//  :00 0000 01 FF
//******************************************************************************

/* Intel HEX read/write functions, Paul Stoffregen, paul@ece.orst.edu */
/* This code is in the public domain.  Please retain my name and */
/* email address in distributed copies, and let me know about any bugs */

/* I, Paul Stoffregen, give no warranty, expressed or implied for */
/* this software and/or documentation provided, including, without */
/* limitation, warranty of merchantability and fitness for a */
/* particular purpose. */

// type modifications by Jon Zeeff

/* parses a line of intel hex code, stores the data in bytes[] */
/* and the beginning address in addr, and returns a 1 if the */
/* line was valid, or a 0 if an error occured.  The variable */
/* num gets the number of bytes that were stored into bytes[] */

#include <stdio.h>		// sscanf(), etc.
#include <string.h>		// strlen(), etc.

// New implementation. Takes a pointer to the line to parse, the length of the line
// plus the length of any remaining lines, and a pointer to the HexInfo to update.
// This implementation is aware fragmented data can be passed between calls, so it
// will concatenate incomplete lines.
// Retuns 0 if an error with line. -1 if line is not complete so it is waiting for
// this function to be called with the remaining data. A positive number if the
// line is good. The positive number indicates the number of characters that was
// parsed from "HexData".
int parse_hex_line(const char *HexData, unsigned int HexDataLen, hex_info_t *HexInfo)
{
  unsigned int off, num;
  unsigned sum, len, cksum;
  int temp;
  const char *ptr;
  String str;

  if (HexDataLen == 0) {
    return 0;
  }

  num = 0;
  off = 0;
  if (HexInfo->prevDataLen == 0) {
    ptr = HexData;
  } else {
    // Copy data from the last frame
    memcpy((void *)&HexInfo->prevData[HexInfo->prevDataLen], (void *)HexData,
                    fx_min(HEX_LINE_MAX_SIZE - HexInfo->prevDataLen, HexDataLen));
    ptr = HexInfo->prevData;
  }

  if (HexDataLen < 11) {
    // Hex line must be at least 11 octets long
    goto copy_leftover;
  }
  
  if (ptr[off] != ':') {
    // Each line must start with the start code
    goto error;
  }
  off++;

  if (!sscanf(&ptr[off], "%02x", &len)) {
    goto error;
  }
  off += 2;

  if (HexDataLen < (11 + (len * 2))) {
    // Incomplete line. Data length byte doesn't match expected line size
    goto copy_leftover;
  }

  if (!sscanf(&ptr[off], "%04x", (unsigned int *)&HexInfo->addr)) {
    goto error;
  }
  off += 4;
  if (!sscanf(&ptr[off], "%02x", &HexInfo->code)) {
    goto error;
  }
  off += 2;

  sum = (len & 255) + ((HexInfo->addr >> 8) & 255) + (HexInfo->addr & 255) + (HexInfo->code & 255);
  while (num != len)
  {

    if (!sscanf(&ptr[off], "%02x", &temp)) {
      goto error;
    }

    HexInfo->data[num] = temp;
    off += 2;
    sum += HexInfo->data[num] & 255;
    num++;
    if (num >= 256) {
      goto error;
    }
  }
  if (!sscanf(&ptr[off], "%02x", &cksum)) {
    goto error;
  }
  off += 2;

  if (((sum & 255) + (cksum & 255)) & 255) {
    goto error; /* checksum error */
  }

  off = off - HexInfo->prevDataLen;
  HexInfo->num = num;
  HexInfo->prevDataLen = 0;
  HexInfo->lines++;
  // Return the length of the line that was parsed
  return off;

copy_leftover:
  memcpy((void *)&HexInfo->prevData[HexInfo->prevDataLen], (void *)ptr, HexDataLen);
  HexInfo->prevDataLen += HexDataLen;
  return -1;

error:
  HexInfo->prevDataLen = 0;
  return 0;
}

// Old implementation
int parse_hex_line(const char *theline, char *bytes,
                   unsigned int *addr, unsigned int *num, unsigned int *code)
{
  unsigned sum, len, cksum;
  const char *ptr;
  int temp;

  *num = 0;
  if (theline[0] != ':')
    return 0;
  if (strlen (theline) < 11)
    return 0;
  ptr = theline + 1;
  if (!sscanf (ptr, "%02x", &len))
    return 0;
  ptr += 2;
  if (strlen (theline) < (11 + (len * 2)))
    return 0;
  if (!sscanf (ptr, "%04x", (unsigned int *)addr))
    return 0;
  ptr += 4;
  /* Serial.printf("Line: length=%d Addr=%d\n", len, *addr); */
  if (!sscanf (ptr, "%02x", code))
    return 0;
  ptr += 2;
  sum = (len & 255) + ((*addr >> 8) & 255) + (*addr & 255) + (*code & 255);
  while (*num != len)
  {
    if (!sscanf (ptr, "%02x", &temp))
      return 0;
    bytes[*num] = temp;
    ptr += 2;
    sum += bytes[*num] & 255;
    (*num)++;
    if (*num >= 256)
      return 0;
  }
  if (!sscanf (ptr, "%02x", &cksum))
    return 0;

  if (((sum & 255) + (cksum & 255)) & 255)
    return 0; /* checksum error */
  return 1;
}
