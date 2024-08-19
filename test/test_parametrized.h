// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

// Use like RUN_PARAM_TEST(test_function, 5, "bla");
#define RUN_PARAM_TEST(func, ...) UnityDefaultTestRun([]() { return func(__VA_ARGS__); }, #func, __LINE__)
