// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "protocol/client.h"

#include <bitset>

#include "daq/daq.h"
#include "logging.h"
#include "run/run.h"

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
  // Streaming happens in equal-sized chunk, except possibly for the last one.
  // For the last one, we may have to move the MESSAGE_END and send out less data.
  auto new_buffer_length = calculate_total_buffer_length(outer_count, inner_count);
  if (new_buffer_length < actual_buffer_length) {
    // Calculate position after the last of the now fewer outer_count data packages.
    auto new_end_of_data = calculate_outer_buffer_position(outer_count, inner_count);
    // Fill buffer behind with zeros
    memset(str_buffer + new_end_of_data, '\0', actual_buffer_length - new_buffer_length);
    // Copy MESSAGE_END after last data package
    memcpy(str_buffer + new_end_of_data - sizeof(',' /* trailing comma */), MESSAGE_END,
           sizeof(MESSAGE_END) - 1);
    // Add a newline
    *(str_buffer + new_end_of_data + sizeof(MESSAGE_END) - 2) = '\n';
    // From now, only send out that much data
    actual_buffer_length = new_buffer_length;
  } else if (new_buffer_length > actual_buffer_length) {
    // This should never happen, since we would need to call prepare() again,
    // which we do not want.
    LOG_ERROR("RunDataNotificationHandler::handle should not have to increase buffer size.")
    return;
  }

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

  actual_buffer_length = calculate_total_buffer_length(outer_count, inner_count);
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

size_t client::RunDataNotificationHandler::calculate_inner_buffer_length(size_t inner_count) {
  // For each inner array, we need '[],', inner_count*6 for 'sD.FFF' and (inner_count-1) bytes for all ','.
  return 3 + inner_count * 7 - 1;
}

size_t client::RunDataNotificationHandler::calculate_outer_buffer_position(size_t outer_count,
                                                                           size_t inner_count) {
  // Returns the position of the N-th outer data package
  auto inner_length = calculate_inner_buffer_length(inner_count);
  return BUFFER_LENGTH_STATIC + outer_count * inner_length;
}

size_t client::RunDataNotificationHandler::calculate_total_buffer_length(size_t outer_count,
                                                                         size_t inner_count) {
  auto inner_length = calculate_inner_buffer_length(inner_count);
  // For message end, we need a few more bytes.
  return BUFFER_LENGTH_STATIC + outer_count * inner_length - sizeof(',' /* trailing comma */) +
         sizeof(MESSAGE_END) - sizeof('\0' /* null byte in MESSAGE_END */) + sizeof('\n' /* newline */);
}
