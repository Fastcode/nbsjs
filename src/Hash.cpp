#include "Hash.hpp"

#include <stdexcept>

namespace nbs {

    namespace hash {

        uint64_t FromJsValue(const Napi::Value& jsHash, const Napi::Env& env) {
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

        Napi::Value ToJsValue(const uint64_t& hash, const Napi::Env& env) {
            return Napi::Buffer<uint8_t>::Copy(env, reinterpret_cast<const uint8_t*>(&hash), sizeof(uint64_t))
                .As<Napi::Value>();
        }

    }  // namespace hash

}  // namespace nbs
