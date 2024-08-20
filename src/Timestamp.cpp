
#include "Timestamp.hpp"

#include <stdexcept>

namespace nbs {

    namespace timestamp {

        uint64_t FromJsValue(const Napi::Value& jsTimestamp, const Napi::Env& env) {

            uint64_t timestamp = 0;

            if (jsTimestamp.IsNumber()) {
                timestamp = jsTimestamp.As<Napi::Number>().Int64Value();
            }
            else if (jsTimestamp.IsBigInt()) {
                bool lossless = true;
                timestamp     = jsTimestamp.As<Napi::BigInt>().Uint64Value(&lossless);
            }
            else if (jsTimestamp.IsObject()) {
                auto ts = jsTimestamp.As<Napi::Object>();

                if (!ts.Has("seconds") || !ts.Has("nanos")) {
                    throw std::runtime_error("expected object with `seconds` and `nanos` keys");
                }

                if (!ts.Get("seconds").IsNumber() || !ts.Get("nanos").IsNumber()) {
                    throw std::runtime_error("`seconds` and `nanos` must be numbers");
                }

                uint64_t seconds = ts.Get("seconds").As<Napi::Number>().Int64Value();
                uint64_t nanos   = ts.Get("nanos").As<Napi::Number>().Int64Value();

                timestamp = seconds * 1000000000L + nanos;
            }
            else {
                throw std::runtime_error("expected positive number or BigInt or timestamp object");
            }
            return timestamp;
        }

        Napi::Value ToJsValue(const uint64_t timestamp, const Napi::Env& env) {
            Napi::Object jsTimestamp = Napi::Object::New(env);
            jsTimestamp.Set("seconds", Napi::Number::New(env, timestamp / 1000000000L));
            jsTimestamp.Set("nanos", Napi::Number::New(env, timestamp % 1000000000L));
            return jsTimestamp;
        }

    }  // namespace timestamp

}  // namespace nbs
