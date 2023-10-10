#ifndef NBS_TYPESUBTYPE_HPP
#define NBS_TYPESUBTYPE_HPP

#include <cstdint>
#include <napi.h>
#include <vector>

#include "Hash.hpp"

namespace nbs {
    struct TypeSubtype {
        /// The hash type of the message
        uint64_t type;

        /// The id field of the message if it exists, else 0 (used for things like camera ids)
        uint32_t subtype;

        /// Convert the given JS object with `type` and `subtype` properties to a TypeSubtype struct
        static TypeSubtype FromJsValue(const Napi::Value& jsValue, const Napi::Env& env);

        /// Convert a possible JS array containing objects with `type` and `subtype properties to 
        /// a vector of TypeSubtype structs
        static std::vector<TypeSubtype> FromJsArray(const Napi::Value& jsValue, const Napi::Env& env);
    };

    // Compares two TypeSubtype objects for ordering using <
    inline bool operator<(const TypeSubtype& lhs, const TypeSubtype& rhs) {
        return (lhs.type < rhs.type) || ((lhs.type == rhs.type) && (lhs.subtype < rhs.subtype));
    }
}  // namespace nbs

#endif  // NBS_TYPESUBTYPE_HPP
