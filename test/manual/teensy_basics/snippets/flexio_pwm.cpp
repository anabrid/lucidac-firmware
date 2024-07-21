// Based on https://github.com/manitou48/teensy4/blob/master/flexiopwm.ino
// flexio pwm  from SDK
// flexio2 clock 480 mhz.  pin 12 2:1   TIMER 0   no shifter

#include <Arduino.h>
#include <FlexIO_t4.h>

constexpr uint8_t PIN_CNVST = 3;
constexpr uint8_t PIN_LED = 13;

#define PRREG(x)                                                                                              \
  Serial.print(#x " 0x");                                                                                     \
  Serial.println(x, HEX)

#define PWMHZ 50000

void setup() {
  Serial.begin(9600);
  pinMode(PIN_LED, OUTPUT);
  digitalWriteFast(PIN_LED, HIGH);

  // Wait for serial connection
  // while (!Serial);
  // delay(1000);

  uint8_t PIN_FLEX_CNVST;
  auto flexio = FlexIOHandler::mapIOPinToFlexIOHandler(PIN_CNVST, PIN_FLEX_CNVST);

  // CCM_CCGR3 |= CCM_CCGR3_FLEXIO2(CCM_CCGR_ON);
  // CCM_CS1CDR &= ~(CCM_CS1CDR_FLEXIO2_CLK_PODF(7) | CCM_CS1CDR_FLEXIO2_CLK_PRED(7)); // clear
  // CCM_CS1CDR |= CCM_CS1CDR_FLEXIO2_CLK_PODF(7) | CCM_CS1CDR_FLEXIO2_CLK_PRED(5);
  flexio->setClockSettings(3, 7, 5);

  int flexhz = 480000000 / 8 / 6; // 480mhz
  int sum = (flexhz * 2 / PWMHZ + 1) / 2;
  int duty = 100 - 25; // 25% high
  int locnt = (sum * duty / 50 + 1) / 2;
  int hicnt = sum - locnt;

  uint8_t _pwm_timer_idx = flexio->requestTimers(1);
  // FLEXIO2_TIMCMP0 = ((locnt - 1) << 8 ) | (hicnt - 1);
  flexio->port().TIMCMP[_pwm_timer_idx] = ((locnt - 1) << 8) | (hicnt - 1);

  // FLEXIO2_TIMCTL0 = FLEXIO_TIMCTL_PINSEL(1) | FLEXIO_TIMCTL_TRGPOL | FLEXIO_TIMCTL_TIMOD(2)
  //                  | FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_TRGSRC;
  flexio->port().TIMCTL[_pwm_timer_idx] = FLEXIO_TIMCTL_PINSEL(PIN_FLEX_CNVST) | FLEXIO_TIMCTL_TRGPOL |
                                          FLEXIO_TIMCTL_TIMOD(2) | FLEXIO_TIMCTL_PINCFG(3) |
                                          FLEXIO_TIMCTL_TRGSRC;

  // FLEXIO2_TIMCFG0 = 0;
  flexio->port().TIMCFG[_pwm_timer_idx] = 0;

  Serial.printf("flexhz %d  pwmhz %d  lo %d hi %d\n", flexhz, PWMHZ, locnt, hicnt);

  // *(portConfigRegister(PIN_CNVST)) = 4; // alt
  flexio->setIOPinToFlexMode(PIN_CNVST);

  flexio->port().CTRL |= FLEXIO_CTRL_FLEXEN;
  digitalWriteFast(PIN_LED, LOW);
}

void loop() {
  delay(500);
  digitalToggleFast(PIN_LED);
}
