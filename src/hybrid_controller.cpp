// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

/**
 * @file Main entrance file for the LUCIDAC/REDAC firmware.
 * 
 * This file contains the Arduino-style setup() and loop()
 * functions and serves as main entrance point for the PlatformIO
 * `teensy41` environment (`pio run -e teensy41`). It includes
 * all the actual code from the `lib` directory.
 **/

#include <Arduino.h>
#include <cstring>
#include <list>

#include "lucidac/lucidac.h"
#include "build/distributor.h"
#include "utils/logging.h"
#include "protocol/registry.h"
#include "protocol/handler.h"
#include "protocol/jsonl_logging.h"
#include "protocol/protocol.h"
#include "protocol/jsonl_server.h"
#include "net/auth.h"
#include "net/settings.h"
#include "net/settings.h"
#include "utils/hashflash.h"
#include "web/server.h"
#include "mode/mode.h"

platform::LUCIDAC carrier_;
auto& netconf = net::StartupConfig::get();
bool network_working;

void leds(uint8_t val) {
  if(carrier_.front_panel) {
    carrier_.front_panel->leds.set_all(val);
    carrier_.front_panel->write_to_hardware();
  }
}

void indicate_led_error() {
  size_t num_blinks = 10;
  for(size_t i=0; i<num_blinks; i++) {
    leds(0x55);
    delay(100);
    leds(0xaa);
    delay(100);
  }
}

/*void setup_remote_log() {
  IPAddress remote{192,168,68,96};
  static EthernetClient client;
  client.connect(remote, 1234);
  msg::Log::get().sinks.add(&client);
}*/

void setup() {
  // Initialize serial communication
  Serial.begin(0);
  while (!Serial && millis() < 4000) {
    // Wait for Serial, but not forever
  }

  msg::Log::get().sinks.add(&Serial);
  msg::Log::get().sinks.add(&msg::StartupLog::get());

  bus::init();
  net::register_settings();

  LOG_ALWAYS(dist::ident());
  LOGV("Flash image (%d bytes) sha256 sum: %s\n",
      loader::flashimage::len(),
      loader::flashimage::sha256sum().to_string().c_str()
  );

  // Initialize carrier board
  // TODO, _ERROR_OUT_ shall not be used, see #116
  LOG(ANABRID_DEBUG_INIT, "Initializing carrier board...");
  if (!carrier_.init()) {
    LOG_ERROR("Error initializing carrier board.");
    indicate_led_error();
  }

  // all LEDs on: Both visual self-test and showing that now ethernet search takes place.
  leds(0xFF);

  LOG(ANABRID_DEBUG_INIT, "Starting up Ethernet...");
  int net_successful = netconf.begin_ip();
  if(net_successful && netconf.enable_mdns)
    netconf.begin_mdns();
  if(net_successful && netconf.enable_jsonl)
    msg::JsonlServer::get().begin();
  if(net_successful && netconf.enable_webserver)
    web::LucidacWebServer::get().begin();

  //setup_remote_log();
  //LOG_ALWAYS("Have set up remote log");

  // Initialize things related to runs
  // TODO, _ERROR_OUT_ shall not be used, see #116
  if (!mode::FlexIOControl::init(mode::DEFAULT_IC_TIME, mode::DEFAULT_OP_TIME)) {
    LOG_ERROR("Error initializing FlexIO mode control.");
    indicate_led_error();
  }

  msg::handlers::Registry.init(carrier_); // registers all commonly known messages

  //LOG("msg::handlers::DynamicRegistry set up with handlers")
  //msg::handlers::DynamicRegistry::dump();

  msg::JsonLinesProtocol::get().init(4096); // Envelope size

  // Done.
  LOG(ANABRID_DEBUG_INIT, "Initialization done.");

  // visual effect to show that booting has been done
  int val = 0xFF;
  for(uint8_t i=0; i<8; i++) {
    val /= 2; leds(val); delay(100);
  }
}

void loop() {
  if(netconf.enable_jsonl)
    msg::JsonlServer::get().loop();

  static net::auth::AuthentificationContext admin_context{net::auth::UserPasswordAuthentification::admin};
  msg::JsonLinesProtocol::get().process_serial_input(admin_context);

  if(netconf.enable_webserver)
    web::LucidacWebServer::get().loop();

  msg::JsonLinesProtocol::get().process_out_of_band_handlers(carrier_);
}