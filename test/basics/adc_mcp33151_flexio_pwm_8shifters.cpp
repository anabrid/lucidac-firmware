// From https://github.com/manitou48/teensy4/blob/master/flexiopwm.ino

// flexio pwm  from SDK
// flexio2 clock 480 mhz.  pin 12 2:1   TIMER 0   no shifter

#include <Arduino.h>
#include <DMAChannel.h>
#include <FlexIO_t4.h>
#include "QNEthernet.h"
#include <bitset>

#define ERROR while (true) { digitalToggleFast(PIN_LED); delay(50);}

namespace qn = qindesign::network;

qn::EthernetUDP udp;
IPAddress client_ip{192, 168, 100, 155};

constexpr uint8_t PIN_CNVST = 6;
constexpr uint8_t PIN_CLK = 9;
constexpr uint8_t PINS_MISO[8] = {10, 12, 11, /* not 13=LED */32, 8, 7, 36, 37};


constexpr uint8_t PIN_LED = 13;

#define PRINT_REG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

FlexIOHandler *flexio;


void setup() {
    pinMode(PIN_LED, OUTPUT);
    digitalWriteFast(PIN_LED, HIGH);

    qn::Ethernet.begin();
    if (!qn::Ethernet.waitForLocalIP(10000)) ERROR
    qn::stdPrint = &udp;
    udp.beginPacket(client_ip, 8000);
    printf("Hello Client, will send you data soon.\n");

    uint8_t PIN_FLEX_CNVST;
    flexio = FlexIOHandler::mapIOPinToFlexIOHandler(PIN_CNVST, PIN_FLEX_CNVST);
    uint8_t PIN_FLEX_CLK = flexio->mapIOPinToFlexPin(PIN_CLK);
    if (PIN_FLEX_CNVST == 0xff || PIN_FLEX_CLK == 0xff) ERROR

    flexio->setClockSettings(3, 7, 7);

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
    flexio->port().TIMCMP[_clk_timer_idx] = 0x0000'40'00;

    flexio->setIOPinToFlexMode(PIN_CLK);


    /*
     *  Configure data shifter
     */

    for (auto PIN_MISO: PINS_MISO) {
        pinMode(PIN_MISO, INPUT);
        flexio->setIOPinToFlexMode(PIN_MISO);
        uint8_t pin_miso_flex_idx = flexio->mapIOPinToFlexPin(PIN_MISO);
        if (PIN_MISO == 0xff) ERROR

        uint8_t shifter_idx = flexio->requestShifter();
        printf("Configuring shifter FlexIO%i:%i for pin %i.\n", flexio->FlexIOIndex() + 1, pin_miso_flex_idx, PIN_MISO);
        // Trigger all shifters from CLK
        flexio->port().SHIFTCTL[shifter_idx] = FLEXIO_SHIFTCTL_TIMSEL(_clk_timer_idx) | FLEXIO_SHIFTCTL_TIMPOL |
                                               FLEXIO_SHIFTCTL_PINSEL(pin_miso_flex_idx) |
                                               FLEXIO_SHIFTCTL_SMOD(1);

        flexio->port().SHIFTCFG[shifter_idx] = 0;
    }

    /*
     *  Enable FlexIO
     */

    flexio->port().CTRL |= FLEXIO_CTRL_FLEXEN;
    digitalWriteFast(PIN_LED, LOW);
    udp.endPacket();
}

void loop() {
    yield();
    delay(500);
    digitalToggleFast(PIN_LED);

    udp.beginPacket(client_ip, 8000);
    for (auto idx: {0, 1, 2, 3, 4, 5, 6, 7}) {
        volatile uint32_t value = flexio->port().SHIFTBUFBIS[idx];
        printf("SHIFTBUF %i: %s = %li\n", idx, std::bitset<32>(value).to_string().c_str(), (value >> 16));
    }
    printf("\n");
    udp.endPacket();
}