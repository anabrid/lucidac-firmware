#include <Arduino.h>

// QTIMER1   pin capture test  qtmr 1 ch 2  pin 11 B0_02,  ch 52
// free-running 16-bit timer,v2 use 0xffff compare for 32-bit
#define PRREG(x)                                                                                              \
  Serial.print(#x " 0x");                                                                                     \
  Serial.println(x, HEX)

#define TICKS_PER_MICRO (150. / 4) // pcs 8+2

#define CAP_MAX 8
volatile uint32_t ticks, oflows, cap_index, cap_vals[CAP_MAX];

void my_isr() { // capture and overflow
  Serial.println("interrupt");
  if (TMR1_SCTRL1 & TMR_SCTRL_IEF) { // capture
    Serial.println("captured");
    TMR1_SCTRL1 &= ~(TMR_SCTRL_IEF); // clear
  }
  asm volatile("dsb"); // wait for clear  memory barrier
}

void capture_init() {
  CCM_CCGR6 |= CCM_CCGR6_QTIMER1(CCM_CCGR_ON);

  /*
   * Configure Timer 1 of first QTMR module to do input-gated counting
   */

  TMR1_CTRL1 = 0; // stop
  TMR1_CNTR1 = 0; // reset counter
  TMR1_SCTRL1 = 0;
  TMR1_LOAD1 = 0;
  TMR1_CSCTRL1 = 0;
  TMR1_LOAD1 = 0;       // start val after compare
  TMR1_COMP11 = 0xffff; // count up to this val, interrupt,  and start again
  TMR1_CMPLD11 = 0xffff;

  /*
  TMR1_SCTRL1 = TMR_SCTRL_CAPTURE_MODE(1);  //rising
  attachInterruptVector(IRQ_QTIMER1, my_isr);
  TMR1_SCTRL1 |= TMR_SCTRL_IEFIE;  // enable interrupts
  NVIC_ENABLE_IRQ(IRQ_QTIMER1);
   */
  TMR1_CTRL1 = TMR_CTRL_CM(0) | TMR_CTRL_PCS(8) | TMR_CTRL_SCS(1);
  // Invert secondary signal (gating when HIGH, counting when LOW)
  TMR1_SCTRL1 = TMR_SCTRL_IPS;

  /*
   * Configure Timer 2 of first QTMR module to cascade from timer 1
   * This increases maximum counts from 16bit to 32bit
   */

  TMR1_CTRL2 = 0;
  TMR1_CNTR2 = 0; // reset counter
  TMR1_SCTRL2 = 0;
  TMR1_LOAD2 = 0;
  TMR1_CSCTRL2 = 0;
  TMR1_LOAD2 = 0;       // start val after compare
  TMR1_COMP12 = 0xffff; // count up to this val, interrupt,  and start again
  TMR1_CMPLD12 = 0xffff;

  TMR1_CTRL2 = TMR_CTRL_CM(0) | TMR_CTRL_PCS(4 + 1) /* | TMR_CTRL_LENGTH*/;

  /*
   * Enable timers
   */

  // Put pin12 in QTimer mode
  *(portConfigRegister(12)) = 1; // ALT 1
  // Enable timers in reverse order
  TMR1_CTRL2 |= TMR_CTRL_CM(7);
  TMR1_CTRL1 |= TMR_CTRL_CM(3);
}

void setup() {
  Serial.begin(0);
  while (!Serial)
    ;
  delay(1000);

  // Connect pin23 to pin12
  pinMode(23, OUTPUT);
  digitalWriteFast(23, HIGH);

  capture_init();

  PRREG(TMR1_SCTRL1);
  PRREG(TMR1_CSCTRL1);
  PRREG(TMR1_CTRL1);
  PRREG(TMR1_LOAD1);
  PRREG(TMR1_COMP11);
  PRREG(TMR1_CMPLD11);
  PRREG(TMR1_COMP21);
  PRREG(TMR1_CMPLD21);
}

void loop() {
  static unsigned long long t = 0;
  static unsigned long long t_prev = 0;
  static unsigned int delay_ns = 100;
  delay_ns *= 2;
  delay_ns %= 10'000'000;

  Serial.println();
  Serial.println(delay_ns);

  digitalWriteFast(23, LOW);
  delayNanoseconds(delay_ns);
  digitalWriteFast(23, HIGH);

  auto tmr1_cntr1 = TMR1_CNTR1;
  Serial.println(tmr1_cntr1);
  auto tmr1_cntr2 = TMR1_CNTR2;
  Serial.println(tmr1_cntr2);
  t = (tmr1_cntr2 * 0xFFFF + tmr1_cntr1) * 671 / 100;
  Serial.println(t);
  TMR1_CNTR1 = 0;
  TMR1_CNTR2 = 0;
  PRREG(TMR1_CNTR1);
  PRREG(TMR1_CNTR2);

  t_prev = t;
  delay(10000);
}
