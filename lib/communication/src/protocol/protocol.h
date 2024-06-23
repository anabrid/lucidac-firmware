// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <ArduinoJson.h>
#include <list>
#include <Print.h>

#include "protocol/handler.h"
#include "net/auth.h"
#include "net/ethernet.h"
#include "utils/durations.h"
#include "utils/print-multiplexer.h"

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

    JsonLinesProtocol() { broadcast.add(&Serial); }

    void init(size_t envelope_size); ///< Allocates storage
    static JsonLinesProtocol& get(); ///< Singleton

    void handleMessage(net::auth::AuthentificationContext &user_context);

    void process_serial_input(net::auth::AuthentificationContext &user_context);
    bool process_tcp_input(net::EthernetClient& stream, net::auth::AuthentificationContext &user_context);
    void process_string_input(const std::string& envelope_in, std::string& envelope_out, net::auth::AuthentificationContext &user_context);
};


} // namespace msg