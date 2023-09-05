// From https://github.com/manitou48/teensy4/blob/master/flexiopwm.ino

// flexio pwm  from SDK
// flexio2 clock 480 mhz.  pin 12 2:1   TIMER 0   no shifter

#include <Arduino.h>
#include <DMAChannel.h>
#include <FlexIO_t4.h>
#include <string>
#include <bitset>

#define _ERROR_OUT_                                                                                           \
  while (true) { digitalToggleFast(PIN_LED); delay(50);}

constexpr uint8_t PIN_CNVST = 3;
constexpr uint8_t PIN_CLK = 4;
constexpr uint8_t PIN_MISO = 5;
constexpr uint8_t PIN_LED = 13;

// DMA Stuff
DMAChannel dmaChannel;
constexpr uint16_t DMABUFFER_SIZE = 2048;
volatile uint32_t dmaBuffer[DMABUFFER_SIZE];


void inputDMAInterrupt()
{
    Serial.print(micros());
    Serial.print("  ");
    Serial.print(dmaBuffer[0]);
    Serial.println();
	dmaChannel.clearInterrupt();	// tell system we processed it.
	asm("DSB");						// this is a memory barrier
}

#define PRINT_REG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

#define PWMHZ 50000

FlexIOHandler *flexio;
uint8_t _miso_shifter_idx;

void setup() {
    Serial.begin(115200);
    pinMode(PIN_LED, OUTPUT);
    digitalWriteFast(PIN_LED, HIGH);

    // Wait for serial connection
    //while (!Serial);
    //delay(1000);

    uint8_t PIN_FLEX_CNVST;
    flexio = FlexIOHandler::mapIOPinToFlexIOHandler(PIN_CNVST, PIN_FLEX_CNVST);
    uint8_t PIN_FLEX_CLK = flexio->mapIOPinToFlexPin(PIN_CLK);
    uint8_t PIN_FLEX_MISO = flexio->mapIOPinToFlexPin(PIN_MISO);
    if (PIN_FLEX_CNVST == 0xff || PIN_FLEX_CLK == 0xff || PIN_FLEX_MISO == 0xff)
      _ERROR_OUT_

    flexio->setClockSettings(3, 6, 5);

    /*
     *  Configure PWM
     */

    uint8_t _pwm_timer_idx = flexio->requestTimers(1);
    //flexio->port().TIMCMP[_pwm_timer_idx] = ((locnt - 1) << 8) | (hicnt - 1);
    // PWM [ 16 bit reserved ][ 8 bit low ][ 8 bit high ]
    flexio->port().TIMCMP[_pwm_timer_idx] = 0x0000'ff'03;

    flexio->port().TIMCTL[_pwm_timer_idx] = FLEXIO_TIMCTL_PINSEL(PIN_FLEX_CNVST) | FLEXIO_TIMCTL_TIMOD(2)
                                            | FLEXIO_TIMCTL_PINCFG(3);

    flexio->port().TIMCFG[_pwm_timer_idx] = 0;

    flexio->setIOPinToFlexMode(PIN_CNVST);


    /*
     *  Configure CLK
     */

    uint8_t _clk_timer_idx = flexio->requestTimers(1);

    // Control timer (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.2999)
    // TODO: Currently triggering on PIN, but it should probably trigger on TIMER N TRIGGER OUTPUT
    flexio->port().TIMCTL[_clk_timer_idx] =
            FLEXIO_TIMCTL_TRGSEL(2 * PIN_FLEX_CNVST) | FLEXIO_TIMCTL_TRGSRC | FLEXIO_TIMCTL_TRGPOL |
            FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(PIN_FLEX_CLK) |
            FLEXIO_TIMCTL_TIMOD(1);
    // Configure timer (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.3002)
    flexio->port().TIMCFG[_clk_timer_idx] = FLEXIO_TIMCFG_TIMOUT(1) | FLEXIO_TIMCFG_TIMDIS(2) | FLEXIO_TIMCFG_TIMENA(6);
    // Dual 8-bit counter baud mode: [ 16 bit reserved ][ 8 bit number of bits * 2 - 1 ][ 8 bit clock divider ]
    flexio->port().TIMCMP[_clk_timer_idx] = 0x0000'20'00;

    flexio->setIOPinToFlexMode(PIN_CLK);


    /*
     *  Configure data shifter
     */

    pinMode(PIN_MISO, INPUT);
    flexio->setIOPinToFlexMode(PIN_MISO);

    // For FlexIO1, Shifter 3-7 support parallel receive, but only 0-3 support DMA
    _miso_shifter_idx = 3;
    // Add additional shifters for more than 32 bit data width (currently 1 -> 64bit = 4*16bit)
    uint8_t _miso_shifter_chain_count = 0;
    if (!flexio->claimShifter(_miso_shifter_idx))
      _ERROR_OUT_

    // Control shifter (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.2992)
    // TIMSEL=clk_timer select timer from above for shifting
    // TIMPOL=1 (default=0) shift on negative edge of shift clock
    // PINCFG=0 (default) disable shifter output
    // PINSEL=miso flex pin
    // PINPOL=0 (=default) pin is active high
    // SMOD=1 (default=0) receive mode, load into SHIFTBUF on timer expiration
    flexio->port().SHIFTCTL[_miso_shifter_idx] = FLEXIO_SHIFTCTL_TIMSEL(_clk_timer_idx) | FLEXIO_SHIFTCTL_TIMPOL |
                                                 FLEXIO_SHIFTCTL_PINSEL(PIN_FLEX_MISO) |
                                                 FLEXIO_SHIFTCTL_SMOD(1);

    // Configure shifter (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.2993)
    // PWIDTH=0 (=default) parallel shift width
    // INSRC=0 (=default) input from pin, not from other shifter
    // SSTOP=0 (=default) shifter stop bit
    // SSTART=0 (=default) shifter start bit
    flexio->port().SHIFTCFG[_miso_shifter_idx] = FLEXIO_SHIFTCFG_PWIDTH(_miso_shifter_chain_count);

    // Chain a few more shift buffers to this one
    for (decltype(_miso_shifter_chain_count) chain_idx = 1; chain_idx < _miso_shifter_chain_count + 1; chain_idx++) {
        // Shiftbuffers are chained N -> N-1, N-1 -> N-2, ...
        auto _idx = _miso_shifter_idx - chain_idx;
        if (!flexio->claimShifter(_idx))
          _ERROR_OUT_
        flexio->port().SHIFTCTL[_idx] =
                FLEXIO_SHIFTCTL_TIMSEL(_clk_timer_idx) | FLEXIO_SHIFTCTL_TIMPOL | FLEXIO_SHIFTCTL_SMOD(1);
        flexio->port().SHIFTCFG[_idx] = FLEXIO_SHIFTCFG_PWIDTH(_miso_shifter_chain_count) | FLEXIO_SHIFTCFG_INSRC;
    }


    /*
     *  Setup DMA
     */

    // TODO: Make a simpler test example with just a timer and a shifter shifting zeros or something and triggering DMA

    dmaChannel.begin();

    dmaChannel.source(flexio->port().SHIFTBUFBIS[_miso_shifter_idx]);
    /*
    // TODO: SADDR is wrong
    dmaChannel.TCD->SADDR = &(flexio->port().SHIFTBUF[_miso_shifter_idx]);
    dmaChannel.TCD->SOFF = 4;
    dmaChannel.TCD->ATTR_SRC =
            (5 << 3) | 2;  // See https://forum.pjrc.com/threads/66201-Teensy-4-1-How-to-start-using-FlexIO
    dmaChannel.TCD->SLAST = 0;
    */

    dmaChannel.destinationCircular(dmaBuffer, DMABUFFER_SIZE);
    /*
    dmaChannel.TCD->DADDR = dmaBuffer;
    dmaChannel.TCD->DOFF = 4;
    dmaChannel.TCD->ATTR_DST = 2;
    dmaChannel.TCD->DLASTSGA = -DMABUFFER_SIZE * 4;
     */

    //dmaChannel.transferSize(4);
    //dmaChannel.transferCount(4);

    /*
    // TODO: NBYTES is wrong (2*32bit buffers = 64bit = 8byte)
    dmaChannel.TCD->NBYTES = 8 * 4;
    // TODO: BITER/CITER is wrong
    dmaChannel.TCD->BITER = DMABUFFER_SIZE / 8;
    dmaChannel.TCD->CITER = DMABUFFER_SIZE / 8;
    */

    dmaChannel.TCD->CSR &= ~(DMA_TCD_CSR_DREQ);                            // do not disable the channel after it completes - so it just keeps going
    dmaChannel.TCD->CSR |=
            DMA_TCD_CSR_INTMAJOR | DMA_TCD_CSR_INTHALF;    // interrupt at completion and at half completion

    //dmaChannel.attachInterrupt( inputDMAInterrupt );
	dmaChannel.triggerAtHardwareEvent( flexio->shiftersDMAChannel(_miso_shifter_idx-_miso_shifter_chain_count) );
    flexio->port().SHIFTSDEN |= 1 << (_miso_shifter_idx - _miso_shifter_chain_count);

    dmaChannel.enable();


    /*
     *  Enable FlexIO
     */

    flexio->port().CTRL |= FLEXIO_CTRL_FLEXEN;
    digitalWriteFast(PIN_LED, LOW);
}

void loop() {
    delay(500);
    digitalToggleFast(PIN_LED);
    //PRINT_REG(flexio->port().SHIFTSTAT);
    //PRINT_REG(flexio->port().SHIFTERR);

    if (dmaChannel.error()) {
        Serial.println("DMA ERROR");
    }

    // Check if data is there and read
    // Since DMA handles data transfer now, SHIFTSTAT is cleared by DMA reads, thus if (true) ...
    if (true) { // (flexio->port().SHIFTSTAT) {
        auto a = flexio->port().SHIFTBUFBIS[_miso_shifter_idx];
        auto b = flexio->port().SHIFTBUF[_miso_shifter_idx - 1];
        //Serial.println(a, BIN);
        //Serial.println(b, BIN);
        std::bitset<32> abits(a);
        std::bitset<32> bbits(b);
        Serial.print("SHIFTBUF: ");
        Serial.print(abits.to_string().c_str());
        Serial.print("  ");
        Serial.print(a);
        Serial.println();

        Serial.print("DMABUF  : ");
        Serial.print(std::bitset<32>(dmaBuffer[0]).to_string().c_str());
        Serial.print("  ");
        Serial.print(dmaBuffer[0]);
        Serial.println();

        Serial.println();
        /*
        Serial.println(bbits.to_string().c_str());

        // Bit operations are too complicated for me :)
        std::bitset<16> collected_bits;
        collected_bits[15] = bbits[0];
        collected_bits[14] = bbits[4];
        collected_bits[13] = bbits[8];
        collected_bits[12] = bbits[12];
        collected_bits[11] = bbits[16];
        collected_bits[10] = bbits[20];
        collected_bits[9] = bbits[24];
        collected_bits[8] = bbits[28];
        collected_bits[7] = abits[0];
        collected_bits[6] = abits[4];
        collected_bits[5] = abits[8];
        collected_bits[4] = abits[12];
        collected_bits[3] = abits[16];
        collected_bits[2] = abits[20];
        collected_bits[1] = abits[24];
        collected_bits[0] = abits[28];
        Serial.println(collected_bits.to_string().c_str());
        auto value = collected_bits.to_ulong();
        Serial.println(value);
        Serial.println();
         */
    }
}