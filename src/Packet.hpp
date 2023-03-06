#ifndef NBS_PACKET_HPP
#define NBS_PACKET_HPP

#include <cstdint>

#include "Hash.hpp"
#include "Timestamp.hpp"

namespace nbs {
    struct Packet {
        /// The packet timestamp in the NBS file
        uint64_t timestamp;

        /// The hash type of the message
        uint64_t type;

        /// The id field of the message if it exists, else 0 (used for things like camera ids)
        uint32_t subtype;

        /// A pointer to the beginning of the packet payload
        uint8_t* payload;

        /// The length of the payload in bytes (excluding the header)
        uint32_t length;

        /// Convert the given JS value to a timestamp in nanoseconds.
        /// The JS value must contain the `timestamp`, `type`, `subtype` and `payload` properties.
        static inline Packet FromJsValue(const Napi::Value jsPacket, const Napi::Env& env) {

            if (!jsPacket.IsObject()) {
                throw std::runtime_error("expected packet object");
            }

            auto jsObject = jsPacket.As<Napi::Object>();

            // Check keys are valid
            if (!jsObject.Has("timestamp")) {
                throw std::runtime_error("expected object with `timestamp` key");
            }
            if (!jsObject.Has("type")) {
                throw std::runtime_error("expected object with `type` key");
            }
            if (!jsObject.Has("subtype")) {
                throw std::runtime_error("expected object with `subtype` key");
            }
            if (!jsObject.Has("payload")) {
                throw std::runtime_error("expected object with `payload` key");
            }

            // Check types are valid
            uint64_t timestamp = 0;
            try {
                timestamp = Timestamp::FromJsValue(jsObject.Get("timestamp"), env);
            }
            catch (const std::exception& ex) {
                throw std::runtime_error(std::string("error in `timestamp`: ") + ex.what());
            }

            uint64_t type = 0;
            try {
                type = Hash::FromJsValue(jsObject.Get("type"), env);
            }
            catch (const std::exception& ex) {
                throw std::runtime_error(std::string("error in `type`: ") + ex.what());
            }

            if (!jsObject.Get("subtype").IsNumber()) {
                throw std::runtime_error("expected `subtype` to be number");
            }
            if (!jsObject.Get("payload").IsBuffer()) {
                throw std::runtime_error("expected `payload` to be buffer object");
            }

            auto payloadBuffer = jsObject.Get("payload").As<Napi::Buffer<uint8_t>>();
            auto subtype       = jsObject.Get("subtype").As<Napi::Number>().Uint32Value();

            Packet packet;
            packet.timestamp = timestamp;
            packet.type      = type;
            packet.subtype   = subtype;
            packet.length    = payloadBuffer.ByteLength();
            packet.payload   = payloadBuffer.Data();

            return packet;
        }
    };
}  // namespace nbs

#endif  // NBS_PACKET_HPP
