// macro sugar for ArduinoJSON

#include <ArduinoJson.h>


// this probably needs conf.toJson(target.to<JsonObject>())
#define JSON_CONVERT_SUGAR(type) \
    inline void convertFromJson(JsonVariantConst src, type& conf) { conf.fromJson(src.as<JsonObjectConst>()); } \
    inline void convertToJson(const type& conf, JsonVariant target) { conf.toJson(target.to<JsonObject>()); }

#define JSON_GET(src, key)          if(src.containsKey(#key)) key = src[#key];
#define JSON_GET_AS(src, key, type) if(src.containsKey(#key)) key = src[#key].as<type>();
#define JSON_SET(target, key)       target[#key] = key;