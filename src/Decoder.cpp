#include "Decoder.hpp"

#include <cmath>
#include <napi.h>
#include <string>

#include "Hash.hpp"
#include "IndexItem.hpp"
#include "Packet.hpp"
#include "Timestamp.hpp"
#include "TypeSubtype.hpp"

namespace nbs {

    Napi::Object Decoder::Init(Napi::Env& env, Napi::Object& exports) {
        Napi::Function func = DefineClass(
            env,
            "Decoder",
            {
                InstanceMethod<&Decoder::GetAvailableTypes>(
                    "getAvailableTypes",
                    napi_property_attributes(napi_writable | napi_configurable)),
                InstanceMethod<&Decoder::GetTimestampRange>(
                    "getTimestampRange",
                    napi_property_attributes(napi_writable | napi_configurable)),
                InstanceMethod<&Decoder::GetPackets>("getPackets",
                                                     napi_property_attributes(napi_writable | napi_configurable)),
                InstanceMethod<&Decoder::GetPacketByIndex>("getPacketByIndex",
                                                           napi_property_attributes(napi_writable | napi_configurable)),
                InstanceMethod<&Decoder::NextTimestamp>("nextTimestamp",
                                                        napi_property_attributes(napi_writable | napi_configurable)),
                InstanceMethod<&Decoder::Close>("close", napi_property_attributes(napi_writable | napi_configurable)),
                InstanceAccessor<&Decoder::GetIndices>("index"),
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
            Napi::TypeError::New(env, "missing argument `paths`: provide an array of nbs file paths")
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

    Napi::Value Decoder::GetIndices(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();

        auto type_iterators = this->index.getAllTypeIterators();
        auto indices        = Napi::Array::New(env, type_iterators.size());

        uint64_t index = 0;

        for (const auto& typeIterator : type_iterators) {
            auto typeSubType = typeIterator.first;
            auto iterators   = typeIterator.second;

            auto typeIndices = Napi::Object::New(env);

            auto jsType = Napi::Object::New(env);
            jsType.Set("type", hash::ToJsValue(typeSubType.type, env));
            jsType.Set("subtype", Napi::Number::New(env, typeSubType.subtype));

            auto timestamps = Napi::Array::New(env, std::distance(iterators.first, iterators.second));

            uint64_t timestampIndex = 0;
            for (auto it = iterators.first; it != iterators.second; it++) {
                auto index                   = *it;
                timestamps[timestampIndex++] = timestamp::ToJsValue(index.item.timestamp, env);
            }

            typeIndices.Set("typeSubType", jsType);
            typeIndices.Set("timestamps", timestamps);

            indices[index++] = typeIndices;
        }

        return indices;
    }

    Napi::Value Decoder::GetAvailableTypes(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();

        auto availableTypes = this->index.getTypes();

        auto jsTypes = Napi::Array::New(env, availableTypes.size());

        for (size_t i = 0; i < availableTypes.size(); i++) {
            auto jsType = Napi::Object::New(env);

            jsType.Set("type", hash::ToJsValue(availableTypes[i].type, env));
            jsType.Set("subtype", Napi::Number::New(env, availableTypes[i].subtype));

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
        jsRange[i + 0] = timestamp::ToJsValue(range.first, env);
        jsRange[i + 1] = timestamp::ToJsValue(range.second, env);

        return jsRange;
    }

    Napi::Value Decoder::NextTimestamp(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();

        uint64_t timestamp = 0;
        try {
            timestamp = timestamp::FromJsValue(info[0], env);
        }
        catch (const std::exception& ex) {
            Napi::TypeError::New(env, std::string("invalid type for argument `timestamp`: ") + ex.what())
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        // Array of typeSubtypes
        std::vector<TypeSubtype> types;
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

        // Single type
        else {
            try {
                types.push_back(this->TypeSubtypeFromJsValue(info[1], env));
            }
            catch (const std::exception& ex) {
                Napi::TypeError::New(
                    env,
                    std::string("invalid type for argument `typeSubtype`: invalid `.subtype`: expected number")
                        + ex.what())
                    .ThrowAsJavaScriptException();
                return env.Undefined();
            }
        }

        int steps;
        try {
            steps = !info[2].IsUndefined() ? info[2].As<Napi::Number>().Int64Value() : 1;
        }
        catch (const std::exception& ex) {
            Napi::TypeError::New(env, std::string("invalid type for argument `step`: expected number") + ex.what())
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        // Sort and step(+-) index to find the new timestamp.
        auto index_timestamp = this->index.nextTimestamp(timestamp, types, steps);

        // Convert timestamp back to Napi format and return.
        auto new_timestamp = timestamp::ToJsValue(index_timestamp, env).As<Napi::Number>();
        return new_timestamp;
    }

    Napi::Value Decoder::GetPackets(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();

        uint64_t timestamp = 0;

        try {
            timestamp = timestamp::FromJsValue(info[0], env);
        }
        catch (const std::exception& ex) {
            Napi::TypeError::New(env, std::string("invalid type for argument `timestamp`: ") + ex.what())
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        std::vector<TypeSubtype> types;

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

        auto packets   = this->GetMatchingPackets(timestamp, types);
        auto jsPackets = Napi::Array::New(env, packets.size());

        for (size_t i = 0; i < packets.size(); i++) {
            jsPackets[i] = Packet::ToJsValue(packets[i], env);
        }

        return jsPackets;
    }

    Napi::Value Decoder::GetPacketByIndex(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();

        uint64_t index = 0;
        auto jsIndex   = info[0];
        if (jsIndex.IsNumber()) {
            index = jsIndex.As<Napi::Number>().Int64Value();
        }
        else {
            Napi::TypeError::New(env, "invalid type for argument `index`: expected integer")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        TypeSubtype type;
        try {
            type = this->TypeSubtypeFromJsValue(info[1], env);
        }
        catch (const std::exception& ex) {
            Napi::TypeError::New(env, "invalid type for argument `type`: " + std::string(ex.what()))
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        auto typeIterator = this->index.getIteratorForType(type);

        // If the index is out of range return undefined
        if (std::distance(typeIterator.first, typeIterator.second) < index) {
            return env.Undefined();
        }

        auto packetLocation = std::next(typeIterator.first, index);
        auto packet         = this->Read(*packetLocation);
        return Packet::ToJsValue(packet, env);
    }

    std::vector<Packet> Decoder::GetMatchingPackets(const uint64_t& timestamp, const std::vector<TypeSubtype>& types) {
        std::vector<Packet> packets;

        auto matchingIndexItems = this->index.getIteratorsForTypes(types);

        for (auto& iteratorPair : matchingIndexItems) {
            auto& begin = iteratorPair.first;
            auto& end   = iteratorPair.second;

            IndexItemFile compValue;
            compValue.item.timestamp = timestamp;

            // Find the first item with a timestamp strictly greater than timestamp
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

    TypeSubtype Decoder::TypeSubtypeFromJsValue(const Napi::Value& jsTypeSubtype, const Napi::Env& env) {
        if (!jsTypeSubtype.IsObject()) {
            throw std::runtime_error("expected object");
        }

        auto typeSubtype = jsTypeSubtype.As<Napi::Object>();

        if (!typeSubtype.Has("type") || !typeSubtype.Has("subtype")) {
            throw std::runtime_error("expected object with `type` and `subtype` keys");
        }

        uint64_t type = 0;

        try {
            type = hash::FromJsValue(typeSubtype.Get("type"), env);
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

    void Decoder::Close(const Napi::CallbackInfo& info) {
        for (auto& map : memoryMaps) {
            map.unmap();
        }
    }

}  // namespace nbs
