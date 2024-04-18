// Based on https://forum.pjrc.com/threads/63353-Teensy-4-1-How-to-start-using-DMA?p=266991&viewfull=1#post266991

// This example is slightly adapted to do DMA input of pins 14, 15, 40, 41, 17, 16, 22, 23.
// All are configured with INPUT_PULLDOWN, giving you a buffer filled with zeros at those pin positions (bit 19-26).
// Pull one of the pins high to see some data at bits 19-26 in the DMA buffer.

#include <Arduino.h>
#include <DMAChannel.h>

DMAChannel dmachannel;

#define DMABUFFER_SIZE 512
uint32_t dmaBuffer[DMABUFFER_SIZE];

int counter = 0;
unsigned long prevTime;
unsigned long currTime;
bool error = false;
bool dmaDone = false;
uint32_t errA, errB, errorIndex, num_of_ones;

// copied from pwm.c
void xbar_connect(unsigned int input, unsigned int output)
{
    if (input >= 88) return;
    if (output >= 132) return;

    volatile uint16_t *xbar = &XBARA1_SEL0 + (output / 2);
    uint16_t val = *xbar;
    if (!(output & 1)) {
        val = (val & 0xFF00) | input;
    } else {
        val = (val & 0x00FF) | (input << 8);
    }
    *xbar = val;
}


void dmaInterrupt()
{
    dmachannel.clearInterrupt();	// tell system we processed it.
    asm("DSB");						// this is a memory barrier

    prevTime = currTime;
    currTime = micros();

    error = false;

    // Example: Count the number of times during DMA transfer when signal was high on pin 40.
    num_of_ones = 0;
    for (auto value: dmaBuffer) {
        if (value & CORE_PIN40_BITMASK)
            num_of_ones++;
    }

    dmaDone = true;
}


void kickOffDMA()
{
    prevTime = micros();
    currTime = prevTime;

    dmachannel.enable();
}


void setup()
{
    Serial.begin(115200);

    // set the GPIO1 pins to input with pulldown
    // This is B0011111111000000000000000000,
    // corresponding to GPIO1 bits 19-26,
    // corresponding to Teensy pins 14, 15, 40, 41, 17, 16, 22, 23,
    // or maybe shifted by one, that's a bit confusing :)
    for (auto pin : {14, 15, 40, 41, 17, 16, 22, 23}) {
        pinMode(pin, INPUT_PULLDOWN);
    }

    // Need to switch the IO pins back to GPI1 from GPIO6
    IOMUXC_GPR_GPR26 &= ~(0x03FC0000u);

    // configure DMA channels
    dmachannel.begin();
    dmachannel.source( GPIO1_DR );
    dmachannel.destinationBuffer( dmaBuffer, DMABUFFER_SIZE * 4 );

    dmachannel.interruptAtCompletion();
    dmachannel.attachInterrupt( dmaInterrupt );

    // clock XBAR - apparently not on by default!
    CCM_CCGR2 |= CCM_CCGR2_XBAR1(CCM_CCGR_ON);

    // set the IOMUX mode to 3, to route it to XBAR
    IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_06 = 3;

    // set XBAR1_IO008 to INPUT
    IOMUXC_GPR_GPR6 &= ~(IOMUXC_GPR_GPR6_IOMUXC_XBAR_DIR_SEL_8);  // Make sure it is input mode

    // daisy chaining - select between EMC06 and SD_B0_04
    IOMUXC_XBAR1_IN08_SELECT_INPUT = 0;

    // Tell XBAR to dDMA on Rising
    XBARA1_CTRL0 = XBARA_CTRL_STS0 | XBARA_CTRL_EDGE0(1) | XBARA_CTRL_DEN0;

    // connect the IOMUX_XBAR_INOUT08 to DMA_CH_MUX_REQ30
    xbar_connect(XBARA1_IN_IOMUX_XBAR_INOUT08, XBARA1_OUT_DMA_CH_MUX_REQ30);

    // trigger our DMA channel at the request from XBAR
    dmachannel.triggerAtHardwareEvent( DMAMUX_SOURCE_XBAR1_0 );

    kickOffDMA();
}


void loop()
{
    delay( 100 );

    if ( dmaDone )
    {
        Serial.printf( "Counter %8d Buffer 0x%08X time %8u  %s\n", counter, dmaBuffer[0], currTime - prevTime, error ?  "ERROR" : "no error"  );
        Serial.printf("Numer of ones on pin 40: %d\n", num_of_ones);
        Serial.printf("Actual register value: 0x%08X\n", GPIO1_DR);

        dmaDone = false;
        delay( 1000 );
        Serial.printf( "Kicking off another \n" );
        kickOffDMA();
    }
    else
    {
        Serial.printf( "Waiting...\n" );
    }

    ++counter;
}