// Copyright (c) 2022 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// ANABRID_BEGIN_LICENSE:GPL
// ANABRID_END_LICENSE

#include <Arduino.h>
#include <unity.h>

#include "hashflash.h"

using namespace utils;

#define SHOW_STR(a) sprintf(msg, #a " = %s", a.c_str()); TEST_MESSAGE(msg)
#define EQ_STR(a, b) std::string a = b; sprintf(msg, #a " = " #b " = %s", a.c_str()); TEST_MESSAGE(msg)

char msg[1000];

//template<typename c, int n>
std::string array2string(std::array<uint8_t, 32> array, const char* fmt="%02X", const char* sep=" ") {
    std::string res;
    for(int i=0;i<array.size(); i++) {
        sprintf(msg, fmt, array[i]);
        res += msg;
        if(i!=array.size()-1) res += sep;
    }
    return res;
}


void test_hash_flash() {
    sprintf(msg, "flashimagelen() = %d\n", flashimagelen());
    TEST_MESSAGE(msg);
    EQ_STR(hashflash, sha256_to_string(hash_flash_sha256()) );
}

void test_array2string() {
    auto hash = hash_flash_sha256();
    SHOW_STR(array2string(hash));
    SHOW_STR(sha256_to_string(hash));
}

void test_parse_sha256() {
    auto hash = hash_flash_sha256();
    EQ_STR(candidate1, sha256_to_string(hash));
    EQ_STR(candidate2, sha256_to_string(parse_sha256(sha256_to_string(hash))));
    TEST_ASSERT(candidate1.length() == 64);
    TEST_ASSERT(candidate2.length() == 64);
    TEST_ASSERT_MESSAGE(candidate1 == candidate2, "parse sha256 full circle doesnt work");
}

void test_sha256_equality() {
    auto hash1 = hash_flash_sha256();
    auto hash2 = hash_flash_sha256();
    TEST_ASSERT(hash1 == hash2);
}

void test_shorthash() {
    EQ_STR(hash1, "25a84bf381e5bdb0de2f1d5de7a9011471ced109fabc284e9d34c116489394bf");
    EQ_STR(hash2, "25a84bf");
    TEST_ASSERT_MESSAGE(sha256_test_short(hash1, hash2) == true, "1 != 2");

    EQ_STR(hash3, "25a84bf381e5bd");
    EQ_STR(hash4, "25");
    TEST_ASSERT_MESSAGE(sha256_test_short(hash3, hash4) == true, "3 != 4");

}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_hash_flash);
  RUN_TEST(test_array2string);
  RUN_TEST(test_parse_sha256);
  RUN_TEST(test_sha256_equality);
  RUN_TEST(test_shorthash);
  UNITY_END();
}

void loop() {}
