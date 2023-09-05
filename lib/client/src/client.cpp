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
