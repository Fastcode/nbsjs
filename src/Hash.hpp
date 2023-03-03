#ifndef HASH_HPP
#define HASH_HPP

#include <napi.h>

#include "xxhash/xxhash.h"

namespace nbs {

    namespace Hash {

        /// Create a XX64 hash from the given JS value, which could be a string or a buffer.
        /// String values will be hashed, and buffers will be interpreted as a XX64 hash.
        static uint64_t FromJsValue(const Napi::Value& jsHash, const Napi::Env& env) {
            uint64_t hash = 0;

            // If we have a string, apply XXHash to get the hash
            if (jsHash.IsString()) {
                std::string s = jsHash.As<Napi::String>().Utf8Value();
                hash          = XXH64(s.c_str(), s.size(), 0x4e55436c);
            }
            // Otherwise try to interpret it as a buffer that contains the hash
            else if (jsHash.IsTypedArray()) {
                Napi::TypedArray typedArray = jsHash.As<Napi::TypedArray>();
                Napi::ArrayBuffer buffer    = typedArray.ArrayBuffer();

                uint8_t* data  = reinterpret_cast<uint8_t*>(buffer.Data());
                uint8_t* start = data + typedArray.ByteOffset();
                uint8_t* end   = start + typedArray.ByteLength();

                if (std::distance(start, end) == 8) {
                    std::memcpy(&hash, start, 8);
                }
                else {
                    throw std::runtime_error("provided Buffer length is not 8");
                }
            }
            else {
                throw std::runtime_error("expected a string or Buffer");
            }

            return hash;
        }

        /// Convert the given XX64 hash to a JS Buffer value
        static Napi::Value ToJsValue(const uint64_t& hash, const Napi::Env& env) {
            return Napi::Buffer<uint8_t>::Copy(env, reinterpret_cast<const uint8_t*>(&hash), sizeof(uint64_t))
                .As<Napi::Value>();
        }

    }  // namespace Hash

}  // namespace nbs


#endif  // HASH_HPP