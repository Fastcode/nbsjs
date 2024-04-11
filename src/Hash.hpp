#ifndef HASH_HPP
#define HASH_HPP

#include <napi.h>

#include "third-party/xxhash/xxhash.h"

namespace nbs {

    namespace hash {

        /**
         * Create a XX64 hash from a given JS value
         *
         * @param jsHash Hash as a JS value. This could be a string which will be hashed,
         *               or an 8 byte Buffer which will be interpreted as a XX64 hash.
         * @param env    JS environment
         * @return JS Hash converted to a uint64_t
         */
        uint64_t FromJsValue(const Napi::Value& jsHash, const Napi::Env& env);

        /**
         * Convert a XX64 hash to a JS Buffer value
         *
         * @param hash XX64 hash.
         * @param env  JS environment.
         * @return Hash as a JS Buffer.
         */
        Napi::Value ToJsValue(const uint64_t& hash, const Napi::Env& env);

    }  // namespace hash

}  // namespace nbs


#endif  // HASH_HPP
