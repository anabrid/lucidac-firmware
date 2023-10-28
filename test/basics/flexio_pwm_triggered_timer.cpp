// From https://github.com/manitou48/teensy4/blob/master/flexiopwm.ino

// flexio pwm  from SDK
// flexio2 clock 480 mhz.  pin 12 2:1   TIMER 0   no shifter

#include <Arduino.h>
#include <FlexIO_t4.h>

#define _ERROR_OUT_                                                                                           \
  while (true) { digitalToggleFast(PIN_LED); delay(50);}

constexpr uint8_t PIN_CNVST = 3;
constexpr uint8_t PIN_CLK = 4;
constexpr uint8_t PIN_LED = 13;

#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

#define PWMHZ 50000


void setup() {
    Serial.begin(9600);
    pinMode(PIN_LED, OUTPUT);
    digitalWriteFast(PIN_LED, HIGH);

    // Wait for serial connection
    //while (!Serial);
    //delay(1000);

    uint8_t PIN_FLEX_CNVST;
    auto flexio = FlexIOHandler::mapIOPinToFlexIOHandler(PIN_CNVST, PIN_FLEX_CNVST);
    uint8_t PIN_FLEX_CLK = flexio->mapIOPinToFlexPin(PIN_CLK);
    if (PIN_FLEX_CNVST == 0xff || PIN_FLEX_CLK == 0xff)
      _ERROR_OUT_

    flexio->setClockSettings(3, 7, 5);

    /*
     *  Configure PWM
     */

    uint8_t _pwm_timer_idx = flexio->requestTimers(1);
    //flexio->port().TIMCMP[_pwm_timer_idx] = ((locnt - 1) << 8) | (hicnt - 1);
    // PWM [ 16 bit reserved ][ 8 bit low ][ 8 bit high ]
    flexio->port().TIMCMP[_pwm_timer_idx] = 0x0000'ff'04;

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
    flexio->port().TIMCTL[_clk_timer_idx] = FLEXIO_TIMCTL_TRGSEL(2 * PIN_FLEX_CNVST) | FLEXIO_TIMCTL_TRGSRC | FLEXIO_TIMCTL_TRGPOL |
                                            FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(PIN_FLEX_CLK) |
                                            FLEXIO_TIMCTL_TIMOD(1);
    // Configure timer (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.3002)
    flexio->port().TIMCFG[_clk_timer_idx] = FLEXIO_TIMCFG_TIMOUT(1) | FLEXIO_TIMCFG_TIMDIS(2) | FLEXIO_TIMCFG_TIMENA(6);
    // Dual 8-bit counter baud mode: [ 16 bit reserved ][ 8 bit number of bits * 2 - 1 ][ 8 bit clock divider ]
    flexio->port().TIMCMP[_clk_timer_idx] = 0x00001f05;

    flexio->setIOPinToFlexMode(PIN_CLK);


    /*
     *  Enable FlexIO
     */

    flexio->port().CTRL |= FLEXIO_CTRL_FLEXEN;
    digitalWriteFast(PIN_LED, LOW);
}

void loop() {
    delay(500);
    digitalToggleFast(PIN_LED);
}