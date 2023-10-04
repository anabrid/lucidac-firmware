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
    : client(client), envelope_out(envelopeOut) {}

void client::RunDataNotificationHandler::handle(float *data, size_t outer_count, size_t inner_count,
                                                const run::Run &run) {
  // UDP object creating, begin/end packet overhead ~5 microseconds
  net::EthernetUDP udp;
  udp.beginPacket(client.remoteIP(), 5733);

  /*
  // TCP: Timing ~180 microseconds for BUFFER_SIZE = 32
  // TCP: Timing ~140 microseconds for BUFFER_SIZE = 32, when converting int(*float data)
  // UDP: Timing ~50 microseconds for BUFFER_SIZE = 32 and data = list[float]
  envelope_out.clear();
  envelope_out["type"] = "run_data";
  auto msg = envelope_out.createNestedObject("msg");
  msg["run_id"] = run.id;
  msg["t_0"] = 0;
  msg["state"] = run::RunStateNames[static_cast<size_t>(run::RunState::OP)];
  auto outer_array = msg.createNestedArray("data");
  for (size_t i = 0; i < outer_count; i++) {
    auto inner_array = outer_array.createNestedArray();
    for (size_t j = 0; j < inner_count; j++) {
      inner_array.add(*(data + i * inner_count + j));
    }
  }
  serializeJson(envelope_out, udp);
  */

  // UDP binary write: Timing ~5 microseconds
  udp.write(reinterpret_cast<const uint8_t *>(data), outer_count*inner_count*sizeof(decltype(data)));

  udp.write("\n");
  udp.endPacket();
}

void client::RunDataNotificationHandler::handle(volatile int *data, size_t outer_count, size_t inner_count,
                                                const run::Run &run) {
  /*
  // Timing ~40 microseconds
  client.writeFully("#");
  for (size_t i = 0; i < outer_count*inner_count; i++) {
    client.writeFully("ABCDEF");
  }
  client.writeFully("\n");
  */

  Serial.println("Sending UDP packet.");
  net::EthernetUDP udp;
  udp.beginPacket(client.remoteIP(), 5733);
  udp.write("b");
  udp.write("\n");
  //udp.send(client.remoteIP(), 5733, (const uint8_t *)data, outer_count*inner_count*sizeof(decltype(data)));

  // Timing ~130 microseconds for BUFFER_SIZE = 32
  envelope_out.clear();
  envelope_out["type"] = "run_data";
  auto msg = envelope_out.createNestedObject("msg");
  msg["run_id"] = run.id;
  msg["t_0"] = 0;
  msg["state"] = run::RunStateNames[static_cast<size_t>(run::RunState::OP)];
  auto outer_array = msg.createNestedArray("data");
  for (size_t i = 0; i < outer_count*inner_count; i++) {
    outer_array.add(*(data + i));
  }
  // serializeJson(envelope_out, Serial);
  // Serial.write("\n");
  serializeJson(envelope_out, udp);
  //client.writeFully("\n");

  udp.endPacket();
}
