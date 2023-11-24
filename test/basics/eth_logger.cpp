// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

/*
 *  CONFIG IN PLATFORMIO:
 *    build_flags = -std=gnu++17 -DQNETHERNET_ENABLE_CUSTOM_WRITE
 */

#include <Arduino.h>
#include "QNEthernet.h"

namespace qn = qindesign::network;

qn::EthernetUDP udp;
IPAddress client_ip{192, 168, 100, 58};


void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWriteFast(LED_BUILTIN, HIGH);

    // Make qnethernet's printf use udp stream
    qn::stdoutPrint = &udp;

    /*
     * If you use a fixed IP address, the first few UDP packets are lost in my tests.
     * Would need to do something similar to waitForLocalIP.
    IPAddress ip{192, 168, 100, 56};
    IPAddress netmask{255, 255, 255, 0};
    IPAddress gateway{192, 168, 100, 1};
    qn::Ethernet.begin(ip, netmask, gateway);
    */
    qn::Ethernet.begin();
    qn::Ethernet.waitForLocalIP(10000);
}


void loop() {
    static unsigned long iteration = 0;
    IPAddress ip = qn::Ethernet.localIP();

    udp.beginPacket(client_ip, 8000);
    printf("Iteration = %lu\r\n", iteration);
    printf("Local IP = %x.%X.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
    udp.endPacket();

    iteration++;
    digitalToggleFast(LED_BUILTIN);
    delay(1000);
    yield();
}
