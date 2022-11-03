// From https://github.com/manitou48/teensy4/blob/master/flexiopwm.ino

// flexio pwm  from SDK
// flexio2 clock 480 mhz.  pin 12 2:1   TIMER 0   no shifter

#include <Arduino.h>
#include <FlexIO_t4.h>
#include <string>
#include <bitset>

#define ERROR while (true) { digitalToggleFast(PIN_LED); delay(50);}

constexpr uint8_t PIN_CNVST = 3;
constexpr uint8_t PIN_CLK = 4;
constexpr uint8_t PIN_MISO = 5;
constexpr uint8_t PIN_LED = 13;

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
    if (PIN_FLEX_CNVST == 0xff || PIN_FLEX_CLK == 0xff || PIN_FLEX_MISO == 0xff) ERROR

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

    // Shifter 7 is one supporting parallel receive
    _miso_shifter_idx = 7;
    // Add additional shifters for more than 32 bit data width (currently 1 -> 64bit = 4*16bit)
    uint8_t _miso_shifter_chain_count = 1;
    if (!flexio->claimShifter(_miso_shifter_idx)) ERROR

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
    flexio->port().SHIFTCFG[_miso_shifter_idx] = FLEXIO_SHIFTCFG_PWIDTH(3);

    // Chain a few more shift buffers to this one
    for (decltype(_miso_shifter_chain_count) chain_idx = 1; chain_idx < _miso_shifter_chain_count + 1; chain_idx++) {
        // Shiftbuffers are chained N -> N-1, N-1 -> N-2, ...
        auto _idx = _miso_shifter_idx - chain_idx;
        if (!flexio->claimShifter(_idx)) ERROR
        flexio->port().SHIFTCTL[_idx] =
                FLEXIO_SHIFTCTL_TIMSEL(_clk_timer_idx) | FLEXIO_SHIFTCTL_TIMPOL | FLEXIO_SHIFTCTL_SMOD(1);
        flexio->port().SHIFTCFG[_idx] = FLEXIO_SHIFTCFG_PWIDTH(3) | FLEXIO_SHIFTCFG_INSRC;
    }


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

    // Check if data is there and read
    if (flexio->port().SHIFTSTAT) {
        auto a = flexio->port().SHIFTBUF[_miso_shifter_idx];
        auto b = flexio->port().SHIFTBUF[_miso_shifter_idx - 1];
        //Serial.println(a, BIN);
        //Serial.println(b, BIN);
        std::bitset<32> abits(a);
        std::bitset<32> bbits(b);
        Serial.println(abits.to_string().c_str());
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
    }
}