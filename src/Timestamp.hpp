#ifndef TIMESTAMP_HPP
#define TIMESTAMP_HPP

#include <iostream>
#include <napi.h>

namespace nbs {
    namespace Timestamp {

        /// Convert the given JS value to a timestamp in nanoseconds.
        /// The JS value can be a number, BigInt, or an object with `seconds` and `nanos` properties.
        static uint64_t FromJsValue(const Napi::Value& jsTimestamp, const Napi::Env& env) {

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

                timestamp = seconds * 1e9 + nanos;
            }
            else {
                throw std::runtime_error("expected positive number or BigInt or timestamp object");
            }

            std::cout << "Timestamp: " << timestamp << std::endl;
            return timestamp;
        }

        /// Convert the given timestamp to a JS object with `seconds` and `nanos` properties
        static Napi::Value ToJsValue(const uint64_t timestamp, const Napi::Env& env) {
            Napi::Object jsTimestamp = Napi::Object::New(env);
            jsTimestamp.Set("seconds", Napi::Number::New(env, timestamp / 1000000000L));
            jsTimestamp.Set("nanos", Napi::Number::New(env, timestamp % 1000000000L));
            return jsTimestamp;
        }

    }  // namespace Timestamp
}  // namespace nbs

#endif  // TIMESTAMP_HPP