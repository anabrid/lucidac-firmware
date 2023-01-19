// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

#include <Arduino.h>
#include "QNEthernet.h"

namespace qn = qindesign::network;

qn::EthernetUDP udp;
IPAddress client_ip{192, 168, 100, 155};


void setup() {
    // Make qnethernet's printf use udp stream
    qn::stdPrint = &udp;

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
    printf("Iteration = %u\r\n", iteration);
    printf("Local IP = %x.%X.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
    udp.endPacket();

    iteration++;
    delay(1000);
    yield();
}
