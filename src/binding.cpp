#include <napi.h>

#include "Decoder.hpp"
#include "Encoder.hpp"

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    nbs::Decoder::Init(env, exports);
    nbs::Encoder::Init(env, exports);
    return exports;
}

NODE_API_MODULE(nbs_decoder, Init)
