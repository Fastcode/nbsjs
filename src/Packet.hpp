#ifndef NBS_PACKET_H
#define NBS_PACKET_H

#include <cstdint>

namespace nbs {
    struct Packet {
        uint64_t timestamp;
        uint64_t type;
        uint32_t subtype;
        uint8_t* payload;
        uint32_t length;
    };
}  // namespace nbs
#endif  // NBS_PACKET_H
