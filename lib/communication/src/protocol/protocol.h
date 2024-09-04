// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#ifdef ARDUINO

#include <ArduinoJson.h>
#include <Print.h>
#include <list>

#include "net/auth.h"
#include "net/ethernet.h"
#include "protocol/handler.h"
#include "utils/durations.h"
#include "utils/print-multiplexer.h"

namespace carrier {
class Carrier;
}

namespace msg {

/**
 * This class is a responder that handles incoming messages in the
 * JsonLines Protocol language over various inputs, which can be
 * Serial input, a TCP/IP input or even a string input.
 *
 * Handles messages according to the global msg::handler::Registry.
 * \ingroup Singletons.
 *
 * TODO: Probably rename this class to sth like "JsonLinesResponder"
 **/
class JsonLinesProtocol {
public:
  DynamicJsonDocument *envelope_in, *envelope_out;
  utils::PrintMultiplexer broadcast;

  JsonLinesProtocol() { broadcast.add_Serial(); }

  void init(size_t envelope_size); ///< Allocates storage
  static JsonLinesProtocol &get(); ///< Singleton

  /**
   * Handles the JSON document currently stored in `envelope_in` and
   * stores the answer in the `envelope_out` accordingly.
   *
   * Note that some out-of-band messages don't run throught this method,
   * for examle @see client::RunStateChangeNotificationHandler::handle().
   **/
  void handleMessage(net::auth::AuthentificationContext &user_context, Print &output);

  void process_serial_input(net::auth::AuthentificationContext &user_context);
  bool process_tcp_input(net::EthernetClient &stream, net::auth::AuthentificationContext &user_context);
  void process_string_input(const std::string &envelope_in, std::string &envelope_out,
                            net::auth::AuthentificationContext &user_context);

  void process_out_of_band_handlers(carrier::Carrier &carrier);
};

} // namespace msg

#endif // ARDUINO
