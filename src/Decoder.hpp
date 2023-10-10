#ifndef NBS_DECODER_HPP
#define NBS_DECODER_HPP

#include <napi.h>
#include <vector>

#include "Index.hpp"
#include "Packet.hpp"
#include "TypeSubtype.hpp"
#include "mio/mmap.hpp"

namespace nbs {
    class Decoder : public Napi::ObjectWrap<Decoder> {
    public:
        /// Initialize the Decoder class NAPI binding
        static Napi::Object Init(Napi::Env& env, Napi::Object& exports);

        /// Constructor: takes a list of file paths from JS and constructs a Decoder
        Decoder(const Napi::CallbackInfo& info);

        /// Get a list of the available types in the nbs files of this decoder
        /// Returns a JS array of type subtype objects
        Napi::Value GetAvailableTypes(const Napi::CallbackInfo& info);

        /// Get a list of the available types in the nbs files of this decoder
        /// Returns a JS array with two elements: the start timestamp object and the end timestamp object
        Napi::Value GetTimestampRange(const Napi::CallbackInfo& info);

        /// Get a list of packets at the given timestamp matching the given list of types and subtypes
        /// Returns a JS array of packet objects
        Napi::Value GetPackets(const Napi::CallbackInfo& info);

        Napi::Value NextTimestamp(const Napi::CallbackInfo& info);

        /**
         * Close the readers to this decoder's nbs files.
         *
         * @param info JS request. Does not require any arguments.
         */
        void Close(const Napi::CallbackInfo& info);

    private:
        /// Holds the index for the nbs files loaded in this decoder
        Index index;

        /// Memory maps for each nbs file in the index, indexed by the file order in the list
        /// of file paths used to construct this decoder
        std::vector<mio::basic_mmap_source<uint8_t>> memoryMaps;

        /// Get the list of packets at the given timestamp matching the given list of types and subtypes
        std::vector<Packet> GetMatchingPackets(const uint64_t& timestamp, const std::vector<TypeSubtype>& types);

        /// Read the packet for the given index item
        Packet Read(const IndexItemFile& item);
    };

}  // namespace nbs

#endif  // NBS_DECODER_HPP
