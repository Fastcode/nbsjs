#include "Encoder.hpp"

#include <napi.h>

#include "Packet.hpp"
#include "Timestamp.hpp"

namespace nbs {

    Napi::Object Encoder::Init(Napi::Env& env, Napi::Object& exports) {

        Napi::Function func =
            DefineClass(env,
                        "Encoder",
                        {
                            InstanceMethod<&Encoder::Write>(
                                "write",
                                static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                            InstanceMethod<&Encoder::GetBytesWritten>(
                                "getBytesWritten",
                                static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                            InstanceMethod<&Encoder::Close>(
                                "close",
                                static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                            InstanceMethod<&Encoder::IsOpen>(
                                "isOpen",
                                static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
                        });

        Napi::FunctionReference* constructor = new Napi::FunctionReference();

        // Create a persistent reference to the class constructor. This will allow
        // a function called on a class prototype and a function
        // called on instance of a class to be distinguished from each other.
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("Encoder", func);

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

    Encoder::Encoder(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Encoder>(info) {
        Napi::Env env = info.Env();

        if (info.Length() == 0) {
            Napi::TypeError::New(env, "missing argument `path`: provide a path to write to")
                .ThrowAsJavaScriptException();
            return;
        }

        if (!info[0].IsString()) {
            Napi::TypeError::New(env, "invalid argument `path`: expected string").ThrowAsJavaScriptException();
            return;
        }

        auto path = info[0].As<Napi::String>().Utf8Value();

        this->output_file.open(path);
        this->index_file.open(path + ".idx");
    }

    Napi::Value Encoder::Write(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();

        if (info.Length() == 0) {
            Napi::TypeError::New(env, "missing argument `data`: provide data to write to the file")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        uint64_t emit_timestamp = 0;
        Packet packet;

        try {
            // Get timestamp in micros
            emit_timestamp = Timestamp::FromJsValue(info[0], env) / 1000;
        }
        catch (const std::exception& ex) {
            Napi::TypeError::New(env, std::string("invalid type for argument `timestamp`: ") + ex.what())
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        try {
            packet = Packet::FromJsValue(info[1], env);
        }
        catch (const std::exception& ex) {
            Napi::TypeError::New(env, std::string("invalid type for argument `packet`: ") + ex.what())
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        // NBS File Format
        // Name      | Type               |  Description
        // ------------------------------------------------------------
        // header    | char[3]            | NBS packet header ☢ { 0xE2, 0x98, 0xA2 }
        // length    | uint32_t           | Length of this packet after this value
        // timestamp | uint64_t           | Timestamp the data was emitted in microseconds
        // hash      | uint64_t           | the 64bit hash for the payload type
        // payload   | char[length - 16]  | the data payload

        // The size of our output timestamp, hash and data
        uint32_t size = sizeof(packet.timestamp) + sizeof(packet.type) + packet.length;

#pragma pack(push, 1)
        struct PacketHeader {
            std::array<char, 3> header = {char(0xE2), char(0x98), char(0xA2)};
            uint32_t size{0};
            uint64_t timestamp{0};
            uint64_t hash{0};

            PacketHeader(const uint32_t& size, const uint64_t& timestamp, const uint64_t& hash)
                : size(size), timestamp(timestamp), hash(hash){};
        };
#pragma pack(pop)

        // Build the packet
        std::vector<uint8_t> packetBytes(sizeof(PacketHeader), '\0');

        // Placement new the header to put the data in
        new (packetBytes.data()) PacketHeader(size, packet.timestamp, packet.type);

        // Load the data into the packet
        const auto* data_c = reinterpret_cast<const uint8_t*>(packet.payload);
        packetBytes.insert(packetBytes.end(), data_c, data_c + packet.length);

        // Write out the packet
        output_file.write(reinterpret_cast<const char*>(packetBytes.data()), int64_t(packetBytes.size()));

        uint32_t full_size = packetBytes.size();

        bytes_written += packet.length;
        return this->GetBytesWritten(info);
    }

    Napi::Value Encoder::GetBytesWritten(const Napi::CallbackInfo& info) {
        return Napi::BigInt::New(info.Env(), this->bytes_written);
    }

    void Encoder::Close(const Napi::CallbackInfo& info) {
        if (this->IsOpen(info).As<Napi::Boolean>()) {
            this->output_file.close();
            this->index_file.close();
        }
    }

    Napi::Value Encoder::IsOpen(const Napi::CallbackInfo& info) {
        return Napi::Boolean::New(info.Env(), this->output_file.is_open());
    }

}  // namespace nbs