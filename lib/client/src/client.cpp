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
#include "run.h"

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

client::RunDataNotificationHandler::RunDataNotificationHandler(carrier::Carrier &carrier,
                                                               net::EthernetClient &client,
                                                               DynamicJsonDocument &envelopeOut)
    : carrier(carrier), client(client), envelope_out(envelopeOut) {}

void client::RunDataNotificationHandler::handle(volatile uint32_t *data, size_t outer_count,
                                                size_t inner_count, const run::Run &run) {
  // digitalWriteFast(LED_BUILTIN, HIGH);
  auto buffer = str_buffer + BUFFER_LENGTH_STATIC;
  size_t inner_length = 3 + inner_count * 7 - 1;
  for (size_t outer_i = 0; outer_i < outer_count; outer_i++) {
    for (size_t inner_i = 0; inner_i < inner_count; inner_i++) {
      memcpy(buffer + outer_i * inner_length + 1 + inner_i * 7,
             daq::BaseDAQ::raw_to_str(data[outer_i * inner_count + inner_i]), 6);
    }
  }
  // digitalWriteFast(LED_BUILTIN, LOW);

  // digitalWriteFast(18, HIGH);
  client.writeFully(str_buffer, actual_buffer_length);
  client.flush();
  // digitalWriteFast(18, LOW);
}

void client::RunDataNotificationHandler::init() {
  envelope_out.clear();
  envelope_out.createNestedArray("data");
}

void client::RunDataNotificationHandler::stream(volatile uint32_t *buffer, run::Run &run) {
  // TODO: Remove
}

void client::RunDataNotificationHandler::prepare(run::Run &run) {
  memcpy(str_buffer, MESSAGE_START, strlen(MESSAGE_START));
  memcpy(str_buffer + BUFFER_IDX_ENTITY_ID, carrier.get_entity_id().c_str(), BUFFER_LENGTH_ENTITY_ID);

  size_t inner_count = run.daq_config.get_num_channels();
  // We always stream half the buffer
  // TODO: Do not access daq::dma::BUFFER_SIZE directly, probably make it a template parameter.
  size_t outer_count = daq::dma::BUFFER_SIZE / inner_count / 2;

  // For each inner array, we need '[],', inner_count*6 for 'sD.FFF' and (inner_count-1) bytes for all ','.
  size_t inner_length = 3 + inner_count * 7 - 1;
  // For message end, we need a few more bytes.
  actual_buffer_length = BUFFER_LENGTH_STATIC + outer_count * inner_length - sizeof(',' /* trailing comma */) +
                         sizeof(MESSAGE_END) - sizeof('\0' /* null byte in MESSAGE_END */) +
                         sizeof('\n' /* newline */);
  memset(str_buffer + BUFFER_LENGTH_STATIC, '-', actual_buffer_length - BUFFER_LENGTH_STATIC);
  memcpy(str_buffer + BUFFER_IDX_RUN_ID, run.id.c_str(), BUFFER_LENGTH_RUN_ID);
  auto buffer = str_buffer + BUFFER_LENGTH_STATIC - 1;
  for (size_t outer_i = 0; outer_i < outer_count; outer_i++) {
    *(++buffer) = '[';
    for (size_t inner_i = 0; inner_i < inner_count; inner_i++) {
      buffer += 7;
      *buffer = ',';
    }
    *buffer = ']';
    *(++buffer) = ',';
  }
  memcpy(buffer, MESSAGE_END, sizeof(MESSAGE_END) - 1);
  str_buffer[actual_buffer_length - 1] = '\n';
}
