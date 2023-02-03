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
IPAddress client_ip{192, 168, 100, 58};

constexpr uint8_t PIN_CNVST = 6;
constexpr uint8_t PIN_CLK = 9;
constexpr uint8_t PINS_MISO[8] = {10, 12, 11, /* not 13=LED */32, 8, 7, 36, 37};

DMAChannel dma(false);
// Buffer for 8 values, coincidentally = sizeof PINS_MISO
volatile uint32_t dma_buffer[8] __attribute__ ((used, aligned(32))) = {1, 2, 3, 4, 5, 6, 7, 8};
constexpr uint8_t PIN_LED = 13;

#define PRINT_REG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

FlexIOHandler *flexio;


void inputDMAInterrupt()
{
    digitalToggleFast(PIN_LED);

    // Clear interrupt
    dma.clearInterrupt();
    // Memory barrier
    asm("DSB");
}


void setup() {
    pinMode(PIN_LED, OUTPUT);
    digitalWriteFast(PIN_LED, HIGH);

    qn::Ethernet.begin();
    if (!qn::Ethernet.waitForLocalIP(10000)) ERROR

    qn::stdPrint = &udp;
    udp.beginPacket(client_ip, 8000);
    printf("Hello Client, will send you data soon.\n");
    udp.endPacket();

    udp.beginPacket(client_ip, 8000);
    printf("DMA Buffer content:\n");
    for (const auto buffer_line: dma_buffer) {
        printf("%s\n", std::bitset<8*sizeof dma_buffer[0]>(buffer_line).to_string().c_str());
    }
    printf("\n");

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
     *  Configure DMA
     */

    // Select shifter zero to generate DMA events.
    // Which shifter is selected should not matter, as long as it is used.
    uint8_t shifter_dma_idx = 0;
    if (flexio->claimShifter(shifter_dma_idx)) ERROR

    // Set shifter to generate DMA events.
    flexio->port().SHIFTSDEN = 1 << shifter_dma_idx;
    // Configure DMA channel
    dma.begin();
    //dma.sourceBuffer(flexio->port().SHIFTBUFBIS, 8);
    //dma.destinationCircular(dma_buffer, 16);

    // One DMA request (SHIFTBUF full) triggers one minor loop
    // CITER "current major loop iteration count"
    // *but* is reduced every minor loop. When it reaches zero, major loop is done.
    dma.TCD->CITER = 2;
    // BITER "beginning iteration count" = number of minor loops in one major loop
    dma.TCD->BITER = 2;

    dma.TCD->SADDR = flexio->port().SHIFTBUFBIS;
    dma.TCD->NBYTES = 8;  // 64bit total transfer (=2 shift buffer)
    dma.TCD->SOFF = 4;  // 32bit offset (=1 shift buffer)
    dma.TCD->ATTR_SRC = B00011010;  // [5bit MOD, 00011=3lower bits may change][3bit SIZE, 010=32bit]

    dma.TCD->DADDR = dma_buffer;
    dma.TCD->DOFF = 4;
    dma.TCD->ATTR_DST = B00101010;  // [5bit MOD, 00101=5lower bits of address may change, 32bytes = 4*2*32bit buffer][3bit SIZE, 010=32bit]

    // Call an interrupt when done
    dma.attachInterrupt(inputDMAInterrupt);
    dma.interruptAtCompletion();
    // Trigger from "shifter full" DMA event
    dma.triggerAtHardwareEvent( flexio->shiftersDMAChannel(shifter_dma_idx));
    // Enable dma channel
    dma.enable();

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
    // Use PIN_LED in DMA interrupt
    //digitalToggleFast(PIN_LED);

    udp.beginPacket(client_ip, 8000);
    for (auto idx: {0, 1, 2, 3, 4, 5, 6, 7}) {
        volatile uint32_t value = flexio->port().SHIFTBUFBIS[idx];
        printf("SHIFTBUF %i: %s = %li\n", idx, std::bitset<32>(value).to_string().c_str(), value);
    }
    printf("\n");

    printf("DMA Buffer @%08lX content:\n", (unsigned long)dma_buffer);
    for (const auto buffer_line: dma_buffer) {
        printf("%s = %i\n", std::bitset<8*sizeof(dma_buffer[0])>(buffer_line).to_string().c_str(), buffer_line);
    }
    printf("\n");
    udp.endPacket();
}