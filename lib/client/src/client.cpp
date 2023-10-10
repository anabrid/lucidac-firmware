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

#include "client.h"

#include <bitset>

#include "daq.h"

void client::RunStateChangeNotificationHandler::handle(const run::RunStateChange change, const run::Run &run) {
  envelope_out.clear();
  envelope_out["type"] = "run_state_change";
  auto msg = envelope_out.createNestedObject("msg");
  msg["id"] = run.id;
  msg["t"] = change.t;
  msg["old"] = run::RunStateNames[static_cast<size_t>(change.old)];
  msg["new"] = run::RunStateNames[static_cast<size_t>(change.new_)];
  serializeJson(envelope_out, Serial);
  Serial.write("\n");
  serializeJson(envelope_out, client);
  client.writeFully("\n");
}

client::RunDataNotificationHandler::RunDataNotificationHandler(net::EthernetClient &client,
                                                               DynamicJsonDocument &envelopeOut)
    : client(client), envelope_out(envelopeOut) {
}

void client::RunDataNotificationHandler::handle(volatile uint32_t *data, size_t outer_count,
                                                size_t inner_count, const run::Run &run) {
  //Serial.println(__PRETTY_FUNCTION__);
  for (size_t i = 0; i < outer_count*inner_count; i++) {
    //Serial.println(data[i]);
    //Serial.println(daq::BaseDAQ::raw_to_normalized(data[i]));
    strncpy(sec_buffer + BUFFER_IDX_DATA + (i*7), daq::BaseDAQ::raw_to_str(data[i]), 6);
  }
  digitalWriteFast(18, HIGH);
  //net::EthernetUDP udp;
  /*
  udp.beginPacket(client.remoteIP(), 5733);
  udp.write(reinterpret_cast<const char *>(str_buffer));
  udp.write('\n');
  udp.endPacket();
   */
  udp.send(client.remoteIP(), 5733, reinterpret_cast<const uint8_t *>(str_buffer), sizeof(str_buffer));
  digitalWriteFast(18, LOW);
}

void client::RunDataNotificationHandler::init() {
  envelope_out.clear();
  envelope_out.createNestedArray("data");
}

void client::RunDataNotificationHandler::stream(volatile uint32_t *buffer, run::Run &run) {
  digitalWriteFast(18, HIGH);

  if (first_data) {
    first_data = false;
    //Serial.println("first_data");
  } else if (last_data) {
    buffer += daq::dma::BUFFER_SIZE/2;
    last_data = false;
    //Serial.println("last_data");
  } else {
    return;
  }

  handle(buffer, daq::dma::BUFFER_SIZE/daq::NUM_CHANNELS/2, daq::NUM_CHANNELS, run);

  net::EthernetUDP udp;
  udp.beginPacket(client.remoteIP(), 5733);
  udp.write(reinterpret_cast<const char *>(str_buffer));
  udp.write("\n");
  udp.endPacket();
  digitalWriteFast(18, LOW);

  //run --op-time 120000 macht ganz komische dinge lol
}
