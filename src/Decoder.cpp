#include "Decoder.hpp"

#include <napi.h>
#include <string>

#include "IndexItem.hpp"
#include "Packet.hpp"
#include "xxhash/xxhash.h"

namespace nbs {

    Napi::Object Decoder::Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func =
            DefineClass(env,
                        "Decoder",
                        {
                            InstanceMethod<&Decoder::GetAvailableTypes>(
                                "getAvailableTypes",
                                static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                            InstanceMethod<&Decoder::GetTimestampRange>(
                                "getTimestampRange",
                                static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                            InstanceMethod<&Decoder::GetPackets>(
                                "getPackets",
                                static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                        });

        Napi::FunctionReference* constructor = new Napi::FunctionReference();

        // Create a persistent reference to the class constructor. This will allow
        // a function called on a class prototype and a function
        // called on instance of a class to be distinguished from each other.
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("Decoder", func);

        // Store the constructor as the add-on instance data. This will allow this
        // add-on to support multiple instances of itself running on multiple worker
        // threads, as well as multiple instances of itself running in different
        // contexts on the same thread.
        //
        // By default, the value set on the environment here will be destroyed when
        // the add-on is unloaded using the `delete` operator, but it is also
        // possible to supply a custom deleter.
        env.SetInstanceData<Napi::FunctionReference>(constructor);

        return exports;
    }

    Decoder::Decoder(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Decoder>(info) {
        Napi::Env env = info.Env();

        // Validate the `paths` argument
        if (info.Length() == 0) {
            Napi::TypeError::New(env, "missing `paths` argument: provide an array of nbs file paths")
                .ThrowAsJavaScriptException();
            return;
        }

        if (!info[0].IsArray()) {
            Napi::TypeError::New(env, "invalid argument `paths`: expected array").ThrowAsJavaScriptException();
            return;
        }

        auto argPaths = info[0].As<Napi::Array>();

        if (argPaths.Length() == 0) {
            Napi::TypeError::New(env, "invalid argument `paths`: expected non-empty array")
                .ThrowAsJavaScriptException();
            return;
        }

        std::vector<std::string> paths;

        // Get the nbs file paths
        for (std::size_t i = 0; i < argPaths.Length(); i++) {
            auto item = argPaths.Get(i);

            if (!item.IsString()) {
                Napi::TypeError::New(env, "invalid item in `paths` array: expected string")
                    .ThrowAsJavaScriptException();
                return;
            }

            paths.push_back(item.As<Napi::String>().Utf8Value());
        }

        // Make an index for all the files
        try {
            this->index = Index(paths);
        }
        catch (const std::exception& e) {
            Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
            return;
        }

        // Memory map each nbs file for reading packets later
        for (auto& path : paths) {
            this->memoryMaps.emplace_back(path, 0, mio::map_entire_file);
        }
    }

    Napi::Value Decoder::GetAvailableTypes(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();

        auto availableTypes = this->index.getTypes();

        auto jsTypes = Napi::Array::New(env, availableTypes.size());

        for (size_t i = 0; i < availableTypes.size(); i++) {
            auto jsType = Napi::Object::New(env);

            jsType.Set("type", this->HashToJsValue(availableTypes[i].first, env));
            jsType.Set("subtype", Napi::Number::New(env, availableTypes[i].second));

            jsTypes[i] = jsType;
        }

        return jsTypes;
    }

    Napi::Value Decoder::GetTimestampRange(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();

        std::pair<uint64_t, uint64_t> range{};

        if (info.Length() > 0) {
            try {
                auto typeSubtype = this->TypeSubtypeFromJsValue(info[0], env);
                range            = this->index.getTimestampRange(typeSubtype);
            }
            catch (const std::exception& ex) {
                Napi::TypeError::New(env, "invalid type for argument `typeSubtype`: " + std::string(ex.what()))
                    .ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }
        else {
            range = this->index.getTimestampRange();
        }

        auto jsRange = Napi::Array::New(env, 2);

        size_t i       = 0;
        jsRange[i + 0] = this->TimestampToJsValue(range.first, env);
        jsRange[i + 1] = this->TimestampToJsValue(range.second, env);

        return jsRange;
    }

    Napi::Value Decoder::GetPackets(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();

        uint64_t timestamp = 0;

        try {
            timestamp = this->TimestampFromJsValue(info[0], env);
        }
        catch (const std::exception& ex) {
            Napi::TypeError::New(env, std::string("invalid type for argument `timestamp`: ") + ex.what())
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        std::vector<std::pair<uint64_t, uint32_t>> types;

        if (info[1].IsArray()) {
            auto argTypes = info[1].As<Napi::Array>();

            // Get the types and subtypes
            for (std::size_t i = 0; i < argTypes.Length(); i++) {
                auto item = argTypes.Get(i);

                try {
                    auto typeSubtype = this->TypeSubtypeFromJsValue(item, env);
                    types.push_back(typeSubtype);
                }
                catch (const std::exception& ex) {
                    Napi::TypeError::New(env, "invalid item type in `types` array: " + std::string(ex.what()))
                        .ThrowAsJavaScriptException();
                    return env.Undefined();
                }
            }
        }
        else if (info[1].IsUndefined()) {
            types = this->index.getTypes();
        }
        else {
            Napi::TypeError::New(env, "invalid type for argument `types`: expected array or undefined")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        auto packets = this->GetMatchingPackets(timestamp, types);

        auto jsPackets = Napi::Array::New(env, packets.size());

        for (size_t i = 0; i < packets.size(); i++) {
            auto jsPacket = Napi::Object::New(env);

            jsPacket.Set("timestamp", this->TimestampToJsValue(packets[i].timestamp, env));
            jsPacket.Set("type", HashToJsValue(packets[i].type, env));
            jsPacket.Set("subtype", Napi::Number::New(env, packets[i].subtype));

            if (packets[i].payload != nullptr) {
                jsPacket.Set("payload", Napi::Buffer<uint8_t>::Copy(env, packets[i].payload, packets[i].length));
            }
            else {
                jsPacket.Set("payload", env.Undefined());
            }

            jsPackets[i] = jsPacket;
        }

        return jsPackets;
    }

    std::vector<Packet> Decoder::GetMatchingPackets(uint64_t timestamp,
                                                    const std::vector<std::pair<uint64_t, uint32_t>>& types) {
        std::vector<Packet> packets;

        auto matchingIndexItems = this->index.getIteratorsForTypes(types);

        for (auto& iteratorPair : matchingIndexItems) {
            auto& begin = iteratorPair.first;
            auto& end   = iteratorPair.second;

            IndexItemFile compValue;
            compValue.item.timestamp = timestamp;

            // Find the first item with a timestamp greater than or equal to the requested timestamp
            auto found = std::upper_bound(begin, end, compValue, [](const IndexItemFile& a, const IndexItemFile& b) {
                return a.item.timestamp < b.item.timestamp;
            });

            if (found == begin) {
                // No packet found for this type at the requested timestamp, insert an empty packet
                Packet emptyPacket;
                emptyPacket.timestamp = timestamp;
                emptyPacket.type      = iteratorPair.first->item.type;
                emptyPacket.subtype   = iteratorPair.first->item.subtype;
                emptyPacket.payload   = nullptr;
                emptyPacket.length    = 0;

                packets.push_back(emptyPacket);
            }
            else {
                Packet packet = this->Read(*std::prev(found, 1));
                packets.push_back(packet);
            }
        }

        return packets;
    }

    Packet Decoder::Read(const IndexItemFile& item) {
        Packet packet;

        packet.timestamp = item.item.timestamp;
        packet.type      = item.item.type;
        packet.subtype   = item.item.subtype;

        uint8_t* packetOffset = &this->memoryMaps[item.fileno][item.item.offset];

        constexpr int headerLength = 3                    // 3 is length of ☢ symbol
                                     + sizeof(uint32_t)   // packet length
                                     + sizeof(uint64_t)   // type hash
                                     + sizeof(uint64_t);  // packet timestamp

        packet.payload = packetOffset + headerLength;
        packet.length  = item.item.length - headerLength;

        return packet;
    }

    uint64_t Decoder::HashFromJsValue(const Napi::Value& jsType, const Napi::Env& env) {
        uint64_t hash = 0;

        // If we have a string, apply XXHash to get the hash
        if (jsType.IsString()) {
            std::string s = jsType.As<Napi::String>().Utf8Value();
            hash          = XXH64(s.c_str(), s.size(), 0x4e55436c);
        }
        // Otherwise try to interpret it as a buffer that contains the hash
        else if (jsType.IsTypedArray()) {
            Napi::TypedArray typedArray = jsType.As<Napi::TypedArray>();
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

    Napi::Value Decoder::HashToJsValue(const uint64_t& type, const Napi::Env& env) {
        return Napi::Buffer<uint8_t>::Copy(env, reinterpret_cast<const uint8_t*>(&type), sizeof(uint64_t))
            .As<Napi::Value>();
    }

    std::pair<uint64_t, uint32_t> Decoder::TypeSubtypeFromJsValue(const Napi::Value& jsValue, const Napi::Env& env) {
        if (!jsValue.IsObject()) {
            throw std::runtime_error("expected object");
        }

        auto typeSubtype = jsValue.As<Napi::Object>();

        if (!typeSubtype.Has("type") || !typeSubtype.Has("subtype")) {
            throw std::runtime_error("expected object with `type` and `subtype` keys");
        }

        uint64_t type = 0;

        try {
            type = this->HashFromJsValue(typeSubtype.Get("type"), env);
        }
        catch (const std::exception& ex) {
            throw std::runtime_error("invalid `.type`: " + std::string(ex.what()));
        }

        uint32_t subtype = 0;

        if (typeSubtype.Get("subtype").IsNumber()) {
            subtype = typeSubtype.Get("subtype").As<Napi::Number>().Uint32Value();
        }
        else {
            throw std::runtime_error("invalid `.subtype`: expected number");
        }

        return {type, subtype};
    }

    uint64_t Decoder::TimestampFromJsValue(const Napi::Value& jsValue, const Napi::Env& env) {
        uint64_t timestamp = 0;

        if (jsValue.IsNumber()) {
            timestamp = jsValue.As<Napi::Number>().Int64Value();
        }
        else if (jsValue.IsBigInt()) {
            bool lossless = true;
            timestamp     = jsValue.As<Napi::BigInt>().Uint64Value(&lossless);
        }
        else if (jsValue.IsObject()) {
            auto ts = jsValue.As<Napi::Object>();

            if (!ts.Has("seconds") || !ts.Has("nanos")) {
                throw std::runtime_error("expected object with `seconds` and `nanos` keys");
            }

            if (!ts.Get("seconds").IsNumber() || !ts.Get("nanos").IsNumber()) {
                throw std::runtime_error("`seconds` and `nanos` must be numbers");
            }

            uint64_t seconds = ts.Get("seconds").As<Napi::Number>().Int64Value();
            uint64_t nanos   = ts.Get("nanos").As<Napi::Number>().Int64Value();

            timestamp = seconds * 1e9 + nanos;
        }
        else {
            throw std::runtime_error("expected positive number or BigInt or timestamp object");
        }

        return timestamp;
    }

    Napi::Value Decoder::TimestampToJsValue(const uint64_t& timestamp, const Napi::Env& env) {
        Napi::Object obj = Napi::Object::New(env);
        obj.Set("seconds", Napi::Number::New(env, timestamp / 1000000000L));
        obj.Set("nanos", Napi::Number::New(env, timestamp % 1000000000L));
        return obj;
    }

}  // namespace nbs
