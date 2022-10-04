/*
 * FlexIO Output Test
 *
 * Mostly based on
 *  - https://forum.pjrc.com/threads/58228-T4-FlexIO-Looking-back-at-my-T4-beta-testing-library-FlexIO_t4
 *  - https://forum.pjrc.com/threads/66201-Teensy-4-1-How-to-start-using-FlexIO
 *
 *  Expected output
 *  - 1-width peak, 3-width peak, 1-width peak on pin 11
 *
 */

#include <Arduino.h>
#include "FlexIO_t4.h"
#include "DMAChannel.h"
#define SHIFTNUM 4 // number of shifters used (must be 1, 2, 4, or 8)
#define SHIFTER_DMA_REQUEST 0 // only 0, 1, 2, 3 expected to work
#define SHIFT_CLOCK_DIVIDER 20 // shift clock is 120 MHz divided by 20 = 6 MHz

FlexIOHandler *pFlex;
IMXRT_FLEXIO_t *p;
const FlexIOHandler::FLEXIO_Hardware_t *hw;
DMAChannel flexDma;

#define DATABUFBYTES 32
uint32_t DMAMEM databuf[DATABUFBYTES/sizeof(uint32_t)]; // declare as uint32_t to ensure word alignment

void FlexIO_Init(){
    /* Get a FlexIO channel */
    pFlex = FlexIOHandler::flexIOHandler_list[1]; // use FlexIO2

    /* Pointer to the port structure in the FlexIO channel */
    p = &pFlex->port();
    
    /* Pointer to the hardware structure in the FlexIO channel */
    hw = &pFlex->hardware();

    /* Basic pin setup */
    pinMode(10, OUTPUT); // FlexIO2:0 //10
    pinMode(12, OUTPUT); // FlexIO2:1
    pinMode(11, OUTPUT); // FlexIO2:2
    pinMode(13, OUTPUT); // FlexIO2:3
    /*
    pinMode(43, OUTPUT); // FlexIO2:7
    pinMode(44, OUTPUT); // FlexIO2:8
    pinMode(45, OUTPUT); // FlexIO2:9
    pinMode(6, OUTPUT); // FlexIO2:10
    pinMode(9, OUTPUT); // FlexIO2:11
    */

    
    /* High speed and drive strength configuration */
    *(portControlRegister(10)) = 0xFF;
    *(portControlRegister(12)) = 0xFF;
    *(portControlRegister(11)) = 0xFF;
    *(portControlRegister(13)) = 0xFF;
    /*
    *(portControlRegister(43)) = 0xFF;
    *(portControlRegister(44)) = 0xFF;
    *(portControlRegister(45)) = 0xFF;
    *(portControlRegister(6)) = 0xFF;
    *(portControlRegister(9)) = 0xFF;
    */

    /* Set clock */
    pFlex->setClockSettings(3, 1, 1); // 120 MHz FlexIO clock (480 MHz PLL3_SW_CLK clock, CLK_PRED=2, CLK_PODF=2)
    // Remark: 120 MHz is the maximum FlexIO frequency shown in the reference manual, but testing shows that 240 MHz works.
    // 480 MHz has also been shown to work but some bugs may occur if CPU speed is less than 600 MHz...
    
    /* Set up pin mux */
    pFlex->setIOPinToFlexMode(10);
    pFlex->setIOPinToFlexMode(12);
    pFlex->setIOPinToFlexMode(11);
    pFlex->setIOPinToFlexMode(13);
    /*
    pFlex->setIOPinToFlexMode(43);
    pFlex->setIOPinToFlexMode(44);
    pFlex->setIOPinToFlexMode(45);
    pFlex->setIOPinToFlexMode(6);
    pFlex->setIOPinToFlexMode(9);
    */

    /* Enable the clock */
    hw->clock_gate_register |= hw->clock_gate_mask  ;

    /* Enable the FlexIO with fast access */
    p->CTRL = FLEXIO_CTRL_FLEXEN | FLEXIO_CTRL_FASTACC;

}


static void FLEXIO_8080_ConfigMulBeatWR(){
    uint32_t i;
    uint8_t MulBeatWR_BeatQty = SHIFTNUM * sizeof(uint32_t) / sizeof(uint8_t);                                //Number of beats = number of shifters * beats per shifter
    Serial.printf("Multi Beat Quantity: %d \n", MulBeatWR_BeatQty);
    /* Disable and reset FlexIO */
    p->CTRL &= ~FLEXIO_CTRL_FLEXEN;

    /* Configure the shifters */
    for(i=0; i<=SHIFTNUM-1; i++)
    {
        p->SHIFTCFG[i] = 
        FLEXIO_SHIFTCFG_INSRC*(1U)                                               /* Shifter input from next shifter's output */
      | FLEXIO_SHIFTCFG_SSTOP(0U)                                               /* Shifter stop bit disabled */
      | FLEXIO_SHIFTCFG_SSTART(0U)                                              /* Shifter start bit disabled and loading data on enabled */
      | FLEXIO_SHIFTCFG_PWIDTH(8U-1U);              /* 8 bit shift width */
    }

    p->SHIFTCTL[0] = 
    FLEXIO_SHIFTCTL_TIMSEL(0)                         /* Shifter's assigned timer index */
      | FLEXIO_SHIFTCTL_TIMPOL*(0U)                                              /* Shift on posedge of shift clock */
      | FLEXIO_SHIFTCTL_PINCFG(3U)                                              /* Shifter's pin configured as output */
      | FLEXIO_SHIFTCTL_PINSEL(1)                    /* Shifter's pin start index */
      | FLEXIO_SHIFTCTL_PINPOL*(0U)                                              /* Shifter's pin active high */
      | FLEXIO_SHIFTCTL_SMOD(2U);               /* shifter mode transmit */

    for(i=1; i<=SHIFTNUM-1; i++)
    {
        p->SHIFTCTL[i] = 
        FLEXIO_SHIFTCTL_TIMSEL(0)                         /* Shifter's assigned timer index */
      | FLEXIO_SHIFTCTL_TIMPOL*(0U)                                              /* Shift on posedge of shift clock */
      | FLEXIO_SHIFTCTL_PINCFG(0U)                                              /* Shifter's pin configured as output disabled */
      | FLEXIO_SHIFTCTL_PINSEL(1)                    /* Shifter's pin start index */
      | FLEXIO_SHIFTCTL_PINPOL*(0U)                                              /* Shifter's pin active high */
      | FLEXIO_SHIFTCTL_SMOD(2U);               /* shifter mode transmit */          
    }

    /* Configure the timer for shift clock */
    p->TIMCMP[0] = 
        ((MulBeatWR_BeatQty * 2U - 1) << 8)                                     /* TIMCMP[15:8] = number of beats x 2 – 1 */
      | (SHIFT_CLOCK_DIVIDER/2U - 1U);                            /* TIMCMP[7:0] = shift clock divide ratio / 2 - 1 */
      
    p->TIMCFG[0] =   FLEXIO_TIMCFG_TIMOUT(0U)                                                /* Timer output logic one when enabled and not affected by reset */
      | FLEXIO_TIMCFG_TIMDEC(0U)                                                /* Timer decrement on FlexIO clock, shift clock equals timer output */
      | FLEXIO_TIMCFG_TIMRST(0U)                                                /* Timer never reset */
      | FLEXIO_TIMCFG_TIMDIS(2U)                                                /* Timer disabled on timer compare */
      | FLEXIO_TIMCFG_TIMENA(2U)                                                /* Timer enabled on trigger high */
      | FLEXIO_TIMCFG_TSTOP(0U)                                                 /* Timer stop bit disabled */
      | FLEXIO_TIMCFG_TSTART*(0U);                                              /* Timer start bit disabled */

    p->TIMCTL[0] = 
        FLEXIO_TIMCTL_TRGSEL((((SHIFTNUM-1) << 2) | 1U))                             /* Timer trigger selected as highest shifter's status flag */
      | FLEXIO_TIMCTL_TRGPOL*(1U)                                                /* Timer trigger polarity as active low */
      | FLEXIO_TIMCTL_TRGSRC*(1U)                                                /* Timer trigger source as internal */
      | FLEXIO_TIMCTL_PINCFG(3U)                                                /* Timer' pin configured as output */
      | FLEXIO_TIMCTL_PINSEL(0)                         /* Timer' pin index: WR pin */
      | FLEXIO_TIMCTL_PINPOL*(1U)                                                /* Timer' pin active low */
      | FLEXIO_TIMCTL_TIMOD(1U);                                                 /* Timer mode 8-bit baud counter */

  Serial.printf("CCM_CDCDR: %x\n", CCM_CDCDR);
  Serial.printf("VERID:%x PARAM:%x CTRL:%x PIN: %x\n", IMXRT_FLEXIO2_S.VERID, IMXRT_FLEXIO2_S.PARAM, IMXRT_FLEXIO2_S.CTRL, IMXRT_FLEXIO2_S.PIN);
  Serial.printf("SHIFTSTAT:%x SHIFTERR=%x TIMSTAT=%x\n", IMXRT_FLEXIO2_S.SHIFTSTAT, IMXRT_FLEXIO2_S.SHIFTERR, IMXRT_FLEXIO2_S.TIMSTAT);
  Serial.printf("SHIFTSIEN:%x SHIFTEIEN=%x TIMIEN=%x\n", IMXRT_FLEXIO2_S.SHIFTSIEN, IMXRT_FLEXIO2_S.SHIFTEIEN, IMXRT_FLEXIO2_S.TIMIEN);
  Serial.printf("SHIFTSDEN:%x SHIFTSTATE=%x\n", IMXRT_FLEXIO2_S.SHIFTSDEN, IMXRT_FLEXIO2_S.SHIFTSTATE);
  for(int i=0; i<SHIFTNUM; i++){
    Serial.printf("SHIFTCTL[%d]:%x \n", i, IMXRT_FLEXIO2_S.SHIFTCTL[i]);
    } 

  for(int i=0; i<SHIFTNUM; i++){
    Serial.printf("SHIFTCFG[%d]:%x \n", i, IMXRT_FLEXIO2_S.SHIFTCFG[i]);
    }   
  
  Serial.printf("TIMCTL:%x %x %x %x\n", IMXRT_FLEXIO2_S.TIMCTL[0], IMXRT_FLEXIO2_S.TIMCTL[1], IMXRT_FLEXIO2_S.TIMCTL[2], IMXRT_FLEXIO2_S.TIMCTL[3]);
  Serial.printf("TIMCFG:%x %x %x %x\n", IMXRT_FLEXIO2_S.TIMCFG[0], IMXRT_FLEXIO2_S.TIMCFG[1], IMXRT_FLEXIO2_S.TIMCFG[2], IMXRT_FLEXIO2_S.TIMCFG[3]);
  Serial.printf("TIMCMP:%x %x %x %x\n", IMXRT_FLEXIO2_S.TIMCMP[0], IMXRT_FLEXIO2_S.TIMCMP[1], IMXRT_FLEXIO2_S.TIMCMP[2], IMXRT_FLEXIO2_S.TIMCMP[3]);
  
    /* Enable FlexIO */

   p->CTRL |= FLEXIO_CTRL_FLEXEN;
   p->SHIFTSDEN |= 1U << (SHIFTER_DMA_REQUEST); // enable DMA trigger when shifter status flag is set on shifter SHIFTER_DMA_REQUEST
    
}


bool WR_DMATransferDone;
uint32_t MulBeatCountRemain;
uint8_t *MulBeatDataRemain;
uint32_t TotalSize;
bool DMAFillColorMode = false;


static void FLEXIO_8080_MulBeatWR_Callback(void)
{

    Serial.println("DMA callback triggred");

    /* the interrupt is called when the final DMA transfer completes writing to the shifter buffers, which would generally happen while
    data is still in the process of being shifted out from the second-to-last major iteration. In this state, all the status flags are cleared.
    when the second-to-last major iteration is fully shifted out, the final data is transfered from the buffers into the shifters which sets all the status flags.
    if you have only one major iteration, the status flags will be immediately set before the interrupt is called, so the while loop will be skipped. */

    while(0 == (p->SHIFTSTAT & (1U << (SHIFTNUM-1))))
    {
    }

    /* Wait the last multi-beat transfer to be completed. Clear the timer flag
    before the completing of the last beat. The last beat may has been completed
    at this point, then code would be dead in the while() below. So mask the
    while() statement and use the software delay .*/
    p->TIMSTAT |= (1U << 0U);

#if 1
    /* Wait timer flag to be set to ensure the completing of the last beat. */
    while(0 == (p->TIMSTAT & (1U << 0U)))
    {
    }
#else
    /* Wait some time to ensure the completing of the last beat. */
        for(uint32_t i=0; i<50U; i++)
        {
        }
#endif

    /* the for loop is probably not sufficient to complete the transfer. Shifting out all 32 bytes takes (32 beats)/(6 MHz) = 5.333 microseconds which is over 3000 CPU cycles.
    If you really need to wait in this callback until all the data has been shifted out, the while loop is probably the correct solution and I don't think it risks an infinite loop.
    however, it seems like a waste of time to wait here, since the process otherwise completes in the background and the shifter buffers are ready to receive new data while the transfer completes.
    I think in most applications you could continue without waiting. You can start a new DMA transfer as soon as the first one completes (no need to wait for FlexIO to finish shifting). */

    WR_DMATransferDone = true;
//    flexDma.disable(); // not necessary because flexDma is already configured to disable on completion
}

void dmaISR(){
    flexDma.clearInterrupt();
    asm volatile ("dsb"); // prevent interrupt from re-entering
    FLEXIO_8080_MulBeatWR_Callback();
}

void FLEXIO_8080_MulBeatWR_nPrm(uint32_t const cmdIdx, uint32_t const * buf, uint32_t const len){ 
    uint32_t BeatsPerMinLoop = SHIFTNUM * sizeof(uint32_t) / sizeof(uint8_t);      // Number of shifters * number of 8 bit values per shifter
    uint32_t majorLoopCount, minorLoopBytes;
    uint32_t destinationModulo = 31-(__builtin_clz(SHIFTNUM*sizeof(uint32_t))); // defines address range for circular DMA destination buffer 

    FLEXIO_8080_ConfigMulBeatWR();
    
    MulBeatCountRemain = len % BeatsPerMinLoop;
    MulBeatDataRemain = (uint8_t*) buf + (len - MulBeatCountRemain); // pointer to the next unused byte (overflow if MulBeatCountRemain = 0)
    TotalSize = len - MulBeatCountRemain;               /* in bytes */
    minorLoopBytes = SHIFTNUM * sizeof(uint32_t);
    majorLoopCount = TotalSize/minorLoopBytes;
    Serial.printf("Length: %d, Count remain: %d, Data remain: %d, TotalSize: %d, majorLoopCount: %d \n",len, MulBeatCountRemain, MulBeatDataRemain, TotalSize, majorLoopCount );

    /* Configure FlexIO with multi-beat write configuration */
    flexDma.begin();
    DMA_CR |= DMA_CR_EMLM; // enable minor loop mapping (not used)
        
    flexDma.TCD->SADDR = (uint32_t*)buf;  // source address
    flexDma.TCD->DADDR = (uint32_t*)&p->SHIFTBUF[0]; // destination address
    flexDma.TCD->ATTR = DMA_TCD_ATTR_SMOD(0U) | DMA_TCD_ATTR_DMOD(destinationModulo) | DMA_TCD_ATTR_SSIZE(2U) | DMA_TCD_ATTR_DSIZE(2U);  // Source data and destination data transfer size 
    flexDma.TCD->SOFF = sizeof(uint32_t); // Source address signed offset
    flexDma.TCD->DOFF = sizeof(uint32_t); // Destination address signed offset
    flexDma.TCD->NBYTES = minorLoopBytes;   // Minor byte transfer count 
    flexDma.TCD->CITER = majorLoopCount; // Current major iteration count
    flexDma.TCD->BITER = majorLoopCount; // Starting major iteration count
    flexDma.TCD->SLAST = -TotalSize; // Source address offset at completion (reset)
    // Note: with this source address offset at completion, the whole transfer can be repeated without doing all the setup again, simply by calling flexDma.enable(). 

    flexDma.triggerAtHardwareEvent(hw->shifters_dma_channel[SHIFTER_DMA_REQUEST]);
    flexDma.disableOnCompletion();
    flexDma.interruptAtCompletion();
    flexDma.clearComplete();
    
    Serial.println("Dma setup done");

    /* Start data transfer by using DMA */
    WR_DMATransferDone = false;
    flexDma.attachInterrupt( dmaISR );
    flexDma.enable();

    Serial.println("Starting transfer");
}


void setup() {
  Serial.begin(115200);
  Serial.print(CrashReport);
  Serial.println("Start setup");

  /* initialize databuf (cannot be initialized in declaration because it is declared DMAMEM) */
  uint8_t databuf_tmp[DATABUFBYTES] = {0xFF, 0x05, 0xFF, 0xFF, 0xFF, 0x05, 0xFF, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05} ;
  memcpy(databuf, databuf_tmp, DATABUFBYTES); 
  arm_dcache_flush((void*)databuf, sizeof(databuf)); // always flush cache after writing to DMAMEM variable that will be accessed by DMA  
  
  FlexIO_Init();
  FLEXIO_8080_MulBeatWR_nPrm(0x02, databuf, DATABUFBYTES);
}

void loop() {
  if(flexDma.error()){
    Serial.print("DMA error: ");
    Serial.println(DMA_ES, HEX);
  }
  delay(1000);

  flexDma.enable(); // repeat transfer
}
