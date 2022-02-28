#ifndef NBS_DECODER_HPP
#define NBS_DECODER_HPP

#include <napi.h>
#include <vector>

#include "Index.hpp"
#include "Packet.hpp"
#include "mio/mmap.hpp"

namespace nbs {
    class Decoder : public Napi::ObjectWrap<Decoder> {
    public:
        static Napi::Object Init(Napi::Env env, Napi::Object exports);

        Decoder(const Napi::CallbackInfo& info);
        Napi::Value GetAvailableTypes(const Napi::CallbackInfo& info);
        Napi::Value GetTimestampRange(const Napi::CallbackInfo& info);
        Napi::Value GetPackets(const Napi::CallbackInfo& info);

    private:
        std::vector<Packet> GetMatchingPackets(uint64_t timestamp,
                                               const std::vector<std::pair<uint64_t, uint32_t>>& types);
        Packet Read(const IndexItemFile& item);

        uint64_t HashFromJsValue(const Napi::Value& jsType, const Napi::Env& env);
        Napi::Value HashToJsValue(const uint64_t& type, const Napi::Env& env);
        std::pair<uint64_t, uint32_t> TypeSubtypeFromJsValue(const Napi::Value& jsValue, const Napi::Env& env);
        uint64_t TimestampFromJsValue(const Napi::Value& jsValue, const Napi::Env& env);
        Napi::Value TimestampToJsValue(const uint64_t& timestamp, const Napi::Env& env);

        /// Holds the index for all the nbs files
        Index index;

        /// Memory maps for each file in the index
        std::vector<mio::basic_mmap_source<uint8_t>> memoryMaps;
    };

}  // namespace nbs

#endif  // NBS_DECODER_HPP
