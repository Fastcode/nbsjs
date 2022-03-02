#ifndef NBS_INDEXITEM_HPP
#define NBS_INDEXITEM_HPP

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

#include "mio/mmap.hpp"

namespace nbs {
    // Make the packing match what it would be in the file.
#pragma pack(push, 1)
    struct IndexItem {
        /// The hash type of the message
        uint64_t type;

        /// The id field of the message if it exists, else 0 (used for things like camera's id)
        uint32_t subtype;

        /// The timestamp from the protcol buffer if it exists, else the timestamp from the NBS file
        uint64_t timestamp;

        /// The offset of the message from the start of the file
        uint64_t offset;

        /// The length of the packet in bytes (including the header)
        uint32_t length;
    };
#pragma pack(pop)

    struct IndexItemFile {
        /// The index item
        IndexItem item;

        /// When multiple nbs files are loaded, this indicates which file the index item is from.
        /// It's the index of the file in the list of files that were passed for opening.
        int fileno;
    };
}  // namespace nbs

#endif  // NBS_INDEXITEM_HPP
