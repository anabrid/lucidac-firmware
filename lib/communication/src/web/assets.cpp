// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "web/assets.h"

// This stub allows the code compile, the attic is empty,
// no static files are included in the build.

// Note that the code in lib/communication/assets/ overwrites
// this file with proper load instructions.

web::StaticAttic::StaticAttic() : files(NULL), number_of_files(0) {}
