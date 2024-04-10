#ifndef NBS_ENCODER_HPP
#define NBS_ENCODER_HPP

#include <fstream>
#include <napi.h>

#include "Packet.hpp"
#include "third-party/zstr/zstr.hpp"

namespace nbs {
    class Encoder : public Napi::ObjectWrap<Encoder> {
    public:
        /**
         * Initialize Encoder class NAPI binding
         */
        static Napi::Object Init(Napi::Env& env, Napi::Object& exports);

        /**
         * Create a new Encoder for an nbs file.
         *
         * @param info JS request containing a string as first argument,
         *             representing the path of the file to write to.
         */
        Encoder(const Napi::CallbackInfo& info);

        /**
         * Write an NBS packet to the file.
         *
         * @param info JS request containing packet as first argument.
         * @return     The total number of bytes written to the NBS file.
         */
        Napi::Value Write(const Napi::CallbackInfo& info);

        /**
         * Get the total number of bytes written to the file.
         *
         * @param info JS request. Does not require any arguments.
         * @return     Total number of bytes written to the NBS file.
         */
        Napi::Value GetBytesWritten(const Napi::CallbackInfo& info);

        /**
         * Close the writer to NBS file and its index file.
         *
         * @param info JS request. Does not require any arguments.
         */
        void Close(const Napi::CallbackInfo& info);

        /**
         * Check if the file being written to is still open.
         *
         * @param info JS request. Does not require any arguments.
         * @return     Boolean value indicating if the file is still open.
         */
        Napi::Value IsOpen(const Napi::CallbackInfo& info);

    private:
        /// The size of the radiation symbol at the start of each packet
        static constexpr int HEADER_SIZE = 3;

        /// The nbs file being written to
        std::unique_ptr<std::ofstream> outputFile;

        /// The index file of the nbs file being written to
        std::unique_ptr<zstr::ofstream> indexFile;

        /// The total number of bytes written to the nbs file so far
        uint64_t bytesWritten{0};

        /// Write the packet to the output nbs file
        uint64_t writePacket(const Packet& packet);

        /// Write the index of a packet to the output index file
        void writeIndex(const Packet& packet, const uint32_t& size);
    };
}  // namespace nbs

#endif  // NBS_ENCODER_HPP
