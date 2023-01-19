// From https://forum.pjrc.com/threads/69845-T4-memory-to-memory-using-dma

#include <Arduino.h>
#include <DMAChannel.h>

DMAChannel m2m(false);
int16_t buffer1[128 * 3] __attribute__ ((used, aligned(32)));
int16_t buffer2[128 * 2] __attribute__ ((used, aligned(32)));


void setup() {
    Serial.begin(9600);
    while(!Serial);
    Serial.println("Go..");

    for (int j = 0; j < 128 * 3; j++) {
        buffer1[j] = j;
    }
    arm_dcache_flush(buffer1, sizeof(buffer1));

    m2m.begin();
    //m2m.sourceCircular(buffer1, sizeof(buffer1));
    m2m.sourceBuffer(buffer1,sizeof(buffer1));
    m2m.destinationBuffer(buffer2, sizeof(buffer2));
    //m2m.transferCount(sizeof(buffer2));
    m2m.enable();
    m2m.disableOnCompletion();
    m2m.triggerContinuously();



    Serial.print("buffer1[10] = ");
    Serial.println(buffer1[10]);

    while (!m2m.complete()) {
        // wait
    }
    arm_dcache_delete(buffer2, sizeof(buffer2));
    Serial.print("buffer2[10] = ");
    Serial.println(buffer2[10]);
}


void loop() {
}
