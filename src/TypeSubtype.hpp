#ifndef NBS_TYPESUBTYPE_HPP
#define NBS_TYPESUBTYPE_HPP

#include <cstdint>

namespace nbs {
    struct TypeSubtype {
        /// The hash type of the message
        uint64_t type;

        /// The id field of the message if it exists, else 0 (used for things like camera ids)
        uint32_t subtype;
    };

    // Compares two TypeSubtype objects for ordering using <
    inline bool operator<(const TypeSubtype& lhs, const TypeSubtype& rhs) {
        return (lhs.type < rhs.type) || ((lhs.type == rhs.type) && (lhs.subtype < rhs.subtype));
    };
}  // namespace nbs

#endif  // NBS_TYPESUBTYPE_HPP
