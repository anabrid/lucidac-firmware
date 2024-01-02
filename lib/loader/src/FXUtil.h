//******************************************************************************
// FXUTIL.H -- FlasherX utility functions
//******************************************************************************
#ifndef FXUTIL_H_
#define FXUTIL_H_

/* --------------------------------------------------------------------------------------------
 * IntelRecordType type
 *
 * Standard Intel record types https://en.wikipedia.org/wiki/Intel_HEX
 */
typedef unsigned int IntelRecordType;

/* Data */
#define IRT_DATA              0x00

/* End of File */
#define IRT_EOF               0x01

/* Extended Segment Address */
#define IRT_ES_ADDR           0x02

/* Start Segment Address */
#define IRT_SS_ADDR           0x03

/* Extended Linear Address */
#define IRT_EL_ADDR           0x04

/* Start Linear Address */
#define IRT_SL_ADDR           0x05

/* End IntelRecordType type */

/* --------------------------------------------------------------------------------------------
 * HEX_LINE_MAX_SIZE define
 *
 * Defines the maximum buffer size needed to read a line from the firmware hex file. The
 * length includes a 4 byte padding for the newline characters that could be on either side
 * of a line.
 */
#define HEX_LINE_MAX_SIZE 48

/* --------------------------------------------------------------------------------------------
 * HEX_DATA_MAX_SIZE define
 *
 * Defines the maximum buffer size needed to read the data portion of the firmware hex
 * file
 */
#define HEX_DATA_MAX_SIZE 32

//******************************************************************************
// hex_info_t	struct for hex record and hex file info
//******************************************************************************
typedef struct {
  char           *data; // pointer to array allocated elsewhere of size HEX_DATA_MAX_SIZE
  unsigned int    addr; // address in intel hex record
  IntelRecordType code; // intel hex record type (0=data, etc.)
  unsigned int    num;  // number of data bytes in intel hex record

  uint32_t base;    // base address to be added to intel hex 16-bit addr
  uint32_t min;     // min address in hex file
  uint32_t max;     // max address in hex file

  int   eof;        // set true on intel hex EOF (code = 1)
  int   lines;      // number of hex records received

  char prevData[HEX_LINE_MAX_SIZE]; // Buffer to hold data between frames
  unsigned int  prevDataLen;        // Length of data in prevData
} hex_info_t;

// Utility Functions
void read_ascii_line( Stream *serial, char *line, int maxbytes );
int parse_hex_line(const char *HexData, unsigned int HexDataLen, hex_info_t *HexInfo);
int  parse_hex_line( const char *theline, char *bytes, unsigned int *addr, unsigned int *num, unsigned int *code );
int  process_hex_record(hex_info_t *hex );
void update_firmware( Stream *in, Stream *out, uint32_t buffer_addr, uint32_t buffer_size );

#endif
