#ifndef TIMESTAMP_HPP
#define TIMESTAMP_HPP

#include <napi.h>

namespace nbs {
    namespace timestamp {

        /// Convert the given JS value to a timestamp in nanoseconds.
        /// The JS value can be a number, BigInt, or an object with `seconds` and `nanos` properties.
        uint64_t FromJsValue(const Napi::Value& jsTimestamp, const Napi::Env& env);

        /// Convert the given timestamp to a JS object with `seconds` and `nanos` properties
        Napi::Value ToJsValue(const uint64_t timestamp, const Napi::Env& env);

    }  // namespace timestamp
}  // namespace nbs

#endif  // TIMESTAMP_HPP
