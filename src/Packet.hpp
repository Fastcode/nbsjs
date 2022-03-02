#ifndef NBS_PACKET_HPP
#define NBS_PACKET_HPP

#include <cstdint>

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

        /// The packet length in bytes, including the header
        uint32_t length;
    };
}  // namespace nbs

#endif  // NBS_PACKET_HPP
