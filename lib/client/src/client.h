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

#pragma once

#include <ArduinoJson.h>
#include <QNEthernetClient.h>
#include <QNEthernetUDP.h>

#include "run.h"

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
  net::EthernetClient &client;
  DynamicJsonDocument &envelope_out;
  char str_buffer[1000] = R"({ "run_id": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx", "data": [sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF] })";
  char sec_buffer[1000] = R"({ "run_id": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx", "data": [sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF,sD.FFF] })";
  static constexpr size_t BUFFER_IDX_DATA = 61;
  net::EthernetUDP udp{};

public:
  RunDataNotificationHandler(net::EthernetClient &client, DynamicJsonDocument &envelopeOut);

  void init() override;
  void handle(volatile uint32_t *data, size_t outer_count, size_t inner_count, const run::Run &run) override;
  void stream(volatile uint32_t *buffer, run::Run &run) override;
};

} // namespace client