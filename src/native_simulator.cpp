// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

/**
 * @file Main entrance file for the Native environment
 * 
 * This file serves as entrance for the `pio run -e native` environment.
 * Currently is it supposed to be a placeholder to allow this command
 * to run successfully. It is implied by `pio run` without any specified
 * environment.
 **/


#include <stdio.h>
int main() { fprintf(stderr, "Main Firmware not yet supported in native simulator\n");  }

