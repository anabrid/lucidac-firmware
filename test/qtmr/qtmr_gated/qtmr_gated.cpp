#include <Arduino.h>

// QTIMER1   pin capture test  qtmr 1 ch 2  pin 11 B0_02,  ch 52
// free-running 16-bit timer,v2 use 0xffff compare for 32-bit
#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)

#define TICKS_PER_MICRO (150./4)  // pcs 8+2

#define CAP_MAX 8
volatile uint32_t ticks, oflows, cap_index, cap_vals[CAP_MAX];

void my_isr() {  // capture and overflow
  Serial.println("interrupt");
  if (TMR1_SCTRL2 & TMR_SCTRL_IEF) { // capture
    Serial.println("captured");
    TMR1_SCTRL2 &= ~(TMR_SCTRL_IEF);  // clear
  }
  asm volatile ("dsb");  // wait for clear  memory barrier
}

void capture_init() {
  CCM_CCGR6 |= CCM_CCGR6_QTIMER1(CCM_CCGR_ON);

  TMR1_CTRL2 = 0; // stop
  TMR1_SCTRL2 = 0;
  TMR1_LOAD2 = 0;
  TMR1_CSCTRL2 = 0;
  TMR1_LOAD2 = 0;  // start val after compare
  TMR1_COMP12 = 0xffff;  // count up to this val, interrupt,  and start again
  TMR1_CMPLD12 = 0xffff;

  /*
  TMR1_SCTRL2 = TMR_SCTRL_CAPTURE_MODE(1);  //rising
  attachInterruptVector(IRQ_QTIMER1, my_isr);
  TMR1_SCTRL2 |= TMR_SCTRL_IEFIE;  // enable interrupts
  NVIC_ENABLE_IRQ(IRQ_QTIMER1);
   */
  TMR1_CTRL2 =  TMR_CTRL_CM(0) | TMR_CTRL_PCS(8 + 7) | TMR_CTRL_SCS(2);
  TMR1_CTRL2 |= TMR_CTRL_CM(3);
  *(portConfigRegister(11)) = 1;  // ALT 1
}

void setup()   {
  Serial.begin(0);
  while (!Serial);
  delay(1000);

  // Connect pin23 to pin11
  pinMode(23, OUTPUT);
  digitalWriteFast(23, LOW);

  capture_init();

  PRREG(TMR1_SCTRL2);
  PRREG(TMR1_CSCTRL2);
  PRREG(TMR1_CTRL2);
  PRREG(TMR1_LOAD2);
  PRREG(TMR1_COMP12);
  PRREG(TMR1_CMPLD12);
  PRREG(TMR1_COMP22);
  PRREG(TMR1_CMPLD22);
}

void loop()
{
  //Serial.printf("ticks %d oflows %d v0 %d v1 %d\n",
  //              ticks, oflows, cap_vals[0], cap_vals[1]);
  Serial.println(TMR1_CNTR2);
  digitalToggleFast(23);
  delay(1000);
}
