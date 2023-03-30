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

        /// Convert the given packet to a JS Napi value.
        /// The returned value will contain properties for `timestamp`, `type`, `subtype` and `payload`.
        static Napi::Value ToJsValue(const Packet& packet, const Napi::Env& env);

        /// Convert the given JS value to a timestamp in nanoseconds.
        /// The JS value must contain the `timestamp`, `type`, `subtype` and `payload` properties.
        ///
        /// The payload of the resulting packet will point to JS managed memory, so it may be cleared if
        /// the JS packet is cleaned up.
        static Packet FromJsValue(const Napi::Value& jsPacket, const Napi::Env& env);
    };

}  // namespace nbs

#endif  // NBS_PACKET_HPP
