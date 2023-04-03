#include "Encoder.hpp"

#include <napi.h>

#include "Packet.hpp"

namespace nbs {

    /**
     * This represents the part of the NBS packet before the payload
     *
     * NBS File Packet Format
     * Name      | Type               |  Description
     * ------------------------------------------------------------
     * header    | char[3]            | NBS packet header ☢ { 0xE2, 0x98, 0xA2 }
     * length    | uint32_t           | Length of this packet after this value
     * timestamp | uint64_t           | Timestamp the data was emitted in microseconds
     * hash      | uint64_t           | the 64bit hash for the payload type
     * payload   | char[length - 16]  | the data payload
     */
    struct PacketHeader {
        std::array<char, 3> header = {char(0xE2), char(0x98), char(0xA2)};
        uint32_t size;
        uint64_t timestamp;
        uint64_t hash;

        PacketHeader(const uint32_t& size, const uint64_t& timestamp, const uint64_t& hash)
            : size(size), timestamp(timestamp), hash(hash){};
    } __attribute__((packed));

    /**
     * NBS Index File Format
     * Name      | Type               |  Description
     * ------------------------------------------------------------
     * hash      | uint64_t           | the 64bit hash for the payload type
     * subtype   | uint32_t           | the id field of the payload
     * timestamp | uint64_t           | Timestamp of the message or the emit timestamp in nanoseconds
     * offset    | uint64_t           | offset to start of radiation symbol ☢
     * size      | uint32_t           | Size of the whole packet from the radiation symbol
     */
    struct PacketIndex {
        uint64_t hash{0};
        uint32_t subtype{0};
        uint64_t timestamp{0};
        uint64_t offset{0};
        uint32_t size{0};

        PacketIndex(const uint64_t& hash,
                    const uint32_t& subtype,
                    const uint64_t& timestamp,
                    const uint64_t& offset,
                    const uint32_t& size)
            : hash(hash), subtype(subtype), timestamp(timestamp), offset(offset), size(size){};
    } __attribute__((packed));

    Napi::Object Encoder::Init(Napi::Env& env, Napi::Object& exports) {

        Napi::Function func = DefineClass(
            env,
            "Encoder",
            {
                InstanceMethod<&Encoder::Write>("write", napi_property_attributes(napi_writable | napi_configurable)),
                InstanceMethod<&Encoder::GetBytesWritten>("getBytesWritten",
                                                          napi_property_attributes(napi_writable | napi_configurable)),
                InstanceMethod<&Encoder::Close>("close", napi_property_attributes(napi_writable | napi_configurable)),
                InstanceMethod<&Encoder::IsOpen>("isOpen", napi_property_attributes(napi_writable | napi_configurable)),
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

        outputFile = std::make_unique<std::ofstream>(path);
        indexFile  = std::make_unique<zstr::ofstream>(path + ".idx");
    }

    Napi::Value Encoder::Write(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();

        if (info.Length() == 0) {
            Napi::TypeError::New(env, "missing argument `packet`: provide a packet to write to the file")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        // This packet instance's payload points to JS managed memory.
        // This is safe to do here since the packet becomes out of scope
        // and is cleaned up when this method finished.
        Packet packet;
        try {
            packet = Packet::FromJsValue(info[0], env);
        }
        catch (const std::exception& ex) {
            Napi::TypeError::New(env, std::string("invalid type for argument `packet`: ") + ex.what())
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        // Write the NBS Packet and get the full size as written
        uint32_t size = writePacket(packet);
        writeIndex(packet, size);

        bytesWritten += size;

        return this->GetBytesWritten(info);
    }

    Napi::Value Encoder::GetBytesWritten(const Napi::CallbackInfo& info) {
        return Napi::BigInt::New(info.Env(), this->bytesWritten);
    }

    void Encoder::Close(const Napi::CallbackInfo& info) {
        if (this->outputFile.get()->is_open()) {
            this->outputFile.get()->close();
            this->indexFile.get()->close();
        }
    }

    Napi::Value Encoder::IsOpen(const Napi::CallbackInfo& info) {
        return Napi::Boolean::New(info.Env(), this->outputFile.get()->is_open());
    }

    uint64_t Encoder::writePacket(const Packet& packet) {

        // The size of our output timestamp, hash and data
        uint32_t size            = sizeof(packet.timestamp) + sizeof(packet.type) + packet.length;
        uint64_t timestampMicros = packet.timestamp / 1000;

        // Build the header section of the packet
        std::vector<uint8_t> packetBytes(sizeof(PacketHeader), '\0');
        new (packetBytes.data()) PacketHeader(size, timestampMicros, packet.type);

        // Append the payload bytes to the packet
        const auto* data = reinterpret_cast<const uint8_t*>(packet.payload);
        packetBytes.insert(packetBytes.end(), data, data + packet.length);

        // Write out the packet to the nbs file
        outputFile.get()->write(reinterpret_cast<const char*>(packetBytes.data()), int64_t(packetBytes.size()));

        return packetBytes.size();
    }

    void Encoder::writeIndex(const Packet& packet, const uint32_t& size) {

        // Create byte data for the index
        std::vector<uint8_t> indexBytes(sizeof(PacketIndex), '\0');
        new (indexBytes.data()) PacketIndex(packet.type, packet.subtype, packet.timestamp, bytesWritten, size);

        // Write out the index to the index file
        indexFile.get()->write(reinterpret_cast<const char*>(indexBytes.data()), int64_t(indexBytes.size()));
    }

}  // namespace nbs
