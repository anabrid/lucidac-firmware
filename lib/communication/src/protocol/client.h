// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <ArduinoJson.h>
#include <QNEthernetClient.h>
#include <QNEthernetUDP.h>

#include "carrier/carrier.h"
#include "daq/daq.h"
#include "run/run.h"

namespace net = qindesign::network;

namespace client {

class RunStateChangeNotificationHandler : public run::RunStateChangeHandler {
// TODO: is only public for some hacky fake data streaming for now
public:
  // TODO: This can become invalid on disconnects
  // TODO: Possibly needs locking/synchronizing with other writes
  net::EthernetClient &client;
  DynamicJsonDocument &envelope_out;

public:
  RunStateChangeNotificationHandler(net::EthernetClient &client, DynamicJsonDocument &envelopeOut)
      : client(client), envelope_out(envelopeOut) {}

  void handle(run::RunStateChange change, const run::Run &run) override;
};

class RunDataNotificationHandler : public run::RunDataHandler {
public:
  // TODO: This can become invalid on disconnects
  // TODO: Possibly needs locking/synchronizing with other writes
  carrier::Carrier& carrier;
  net::EthernetClient &client;
  DynamicJsonDocument &envelope_out;

private:
  // TODO: At least de-duplicate some strings, so it doesn't explode the second someone touches it.
  static constexpr decltype(auto) MESSAGE_START =
      R"({ "type": "run_data", "msg": { "id": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx", "entity": ["XX-XX-XX-XX-XX-XX", "0"], "data": [)";
  static constexpr decltype(auto) MESSAGE_END = "]}}";
  static constexpr size_t BUFFER_LENGTH_STATIC = sizeof(MESSAGE_START) - sizeof('\0');
  static constexpr size_t BUFFER_IDX_RUN_ID = 38;
  static constexpr size_t BUFFER_LENGTH_RUN_ID = 32 + 4;
  static constexpr size_t BUFFER_IDX_ENTITY_ID = 89;
  static constexpr size_t BUFFER_LENGTH_ENTITY_ID = 12 + 5;
  static constexpr size_t BUFFER_LENGTH =
      BUFFER_LENGTH_STATIC + daq::dma::BUFFER_SIZE / 2 * sizeof("[sD.FFF]") + sizeof(MESSAGE_END);
  char str_buffer[BUFFER_LENGTH]{};
  net::EthernetUDP udp{};

  size_t actual_buffer_length = BUFFER_LENGTH;

public:
  RunDataNotificationHandler(carrier::Carrier &carrier, net::EthernetClient &client,
                             DynamicJsonDocument &envelopeOut);

  static size_t calculate_inner_buffer_length(size_t inner_count);
  static size_t calculate_outer_buffer_position(size_t outer_count, size_t inner_count);
  static size_t calculate_total_buffer_length(size_t outer_count, size_t inner_count);

  void init() override;
  void prepare(run::Run &run) override;
  void handle(volatile uint32_t *data, size_t outer_count, size_t inner_count, const run::Run &run) override;
  void stream(volatile uint32_t *buffer, run::Run &run) override;
};

} // namespace client