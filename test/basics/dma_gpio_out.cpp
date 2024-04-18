// Based on https://forum.pjrc.com/threads/63353-Teensy-4-1-How-to-start-using-DMA?p=267122&viewfull=1#post267122

#include <Arduino.h>
#include <DMAChannel.h>

DMAChannel dmachannel;

#define DMABUFFER_SIZE	4096
uint32_t dmaBuffer[DMABUFFER_SIZE];

int counter = 0;
unsigned long prevTime;
unsigned long currTime;
bool error = false;
bool dmaDone = false;
uint32_t errA, errB, errorIndex;

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


void outputDMAInterrupt()
{
    dmachannel.clearInterrupt();	// tell system we processed it.
    asm("DSB");						// this is a memory barrier

    prevTime = currTime;
    currTime = micros();

    error = false;

    dmaDone = true;
}


void setupOutputDMA()
{
    // prepare the output buffer
    for( int i=0; i<DMABUFFER_SIZE; ++i )
    {
        if (i % 6)
            dmaBuffer[i] = ( 0xFF ) << 18;
        else
            dmaBuffer[i] = 0;
    }

    // set GPIO1 to output
    GPIO1_GDIR |= 0x03FC0000u;

    // Need to switch the IO pins back to GPI1 from GPIO6
    IOMUXC_GPR_GPR26 &= ~(0x03FC0000u);

    // configure DMA channels
    dmachannel.begin();
    dmachannel.sourceBuffer( dmaBuffer, DMABUFFER_SIZE * 4 );
    dmachannel.destination( GPIO1_DR );

    dmachannel.interruptAtCompletion();
    dmachannel.attachInterrupt( outputDMAInterrupt );

    // set the IOMUX mode to 3, to route it to FlexPWM
    IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_06 = 1;

    // setup flexPWM to generate clock signal on pin 4
    FLEXPWM2_MCTRL		|= FLEXPWM_MCTRL_CLDOK( 1 );

    FLEXPWM2_SM0CTRL	 = FLEXPWM_SMCTRL_FULL | FLEXPWM_SMCTRL_PRSC(0);
    FLEXPWM2_SM0CTRL2	 = FLEXPWM_SMCTRL2_INDEP | FLEXPWM_SMCTRL2_CLK_SEL(0);			// wait enable? debug enable?
    FLEXPWM2_SM0INIT	 = 0;
    FLEXPWM2_SM0VAL0	 = 0;									// midrange value to position the signale
    FLEXPWM2_SM0VAL1	 = 66;									// max value
    FLEXPWM2_SM0VAL2	 = 0;									// start A
    FLEXPWM2_SM0VAL3	 = 33;								// end A
    FLEXPWM2_OUTEN		|= FLEXPWM_OUTEN_PWMA_EN( 1 );

    FLEXPWM2_SM0TCTRL	 = FLEXPWM_SMTCTRL_PWAOT0;

    FLEXPWM2_MCTRL		|= FLEXPWM_MCTRL_LDOK( 1 );
    FLEXPWM2_MCTRL		|= FLEXPWM_MCTRL_RUN( 1 );

    // clock XBAR - apparently not on by default!
    CCM_CCGR2 |= CCM_CCGR2_XBAR1(CCM_CCGR_ON);

    // Tell XBAR to dDMA on Falling edge
    // DMA will output next data piece on falling edge
    // so the client can receive it on rising
    XBARA1_CTRL0 = XBARA_CTRL_STS0 | XBARA_CTRL_EDGE0(2) | XBARA_CTRL_DEN0;

    // connect the clock signal to DMA_CH_MUX_REQ30
    xbar_connect( XBARA1_IN_FLEXPWM2_PWM1_OUT_TRIG0, XBARA1_OUT_DMA_CH_MUX_REQ30);

    // trigger our DMA channel at the request from XBAR
    dmachannel.triggerAtHardwareEvent( DMAMUX_SOURCE_XBAR1_0 );
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

    setupOutputDMA();

    kickOffDMA();
}


void loop()
{
    delay( 100 );

    for (auto i : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}) {
        Serial.printf("%i: %d\n", i, dmaBuffer[i] & 0x00080000);
    }

    if ( dmaDone )
    {
        Serial.printf( "Counter %8d Buffer 0x%08X time %8u  %s", counter, dmaBuffer[0], currTime - prevTime, error ?  "ERROR" : "no error"  );

        if ( error )
        {
            Serial.printf( " [%d] 0x%08X 0x%08X", errorIndex, errA, errB );
        }

        Serial.printf( "\n");

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