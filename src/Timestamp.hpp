#ifndef TIMESTAMP_HPP
#define TIMESTAMP_HPP

#include <napi.h>

namespace nbs {
    namespace timestamp {

        /**
         * Convert the given JS value to a timestamp in nanoseconds.
         *
         * @param jsTimestamp JS value. This can be a Number, BigInt, or object with `seconds` and `nanos` properties.
         * @param env         JS environment.
         * @return            Timestamp in nanoseconds.
         */
        uint64_t FromJsValue(const Napi::Value& jsTimestamp, const Napi::Env& env);

        /**
         * Convert a timestamp in nanoseconds to a JS object.
         *
         * @param timestamp Timestamp in nanoseconds.
         * @param env       JS environment
         * @return          JS object with `seconds` and `nanos` properties.
         */
        Napi::Value ToJsValue(const uint64_t timestamp, const Napi::Env& env);

    }  // namespace timestamp
}  // namespace nbs

#endif  // TIMESTAMP_HPP
