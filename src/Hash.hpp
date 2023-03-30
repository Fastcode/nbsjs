#ifndef HASH_HPP
#define HASH_HPP

#include <napi.h>

#include "xxhash/xxhash.h"

namespace nbs {

    namespace hash {

        /// Create a XX64 hash from the given JS value, which could be a string or a buffer.
        /// String values will be hashed, and buffers will be interpreted as a XX64 hash.
        uint64_t FromJsValue(const Napi::Value& jsHash, const Napi::Env& env);

        /// Convert the given XX64 hash to a JS Buffer value
        Napi::Value ToJsValue(const uint64_t& hash, const Napi::Env& env);

    }  // namespace hash

}  // namespace nbs


#endif  // HASH_HPP
