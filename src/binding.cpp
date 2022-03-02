#include <napi.h>

#include "Decoder.hpp"

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    nbs::Decoder::Init(env, exports);
    return exports;
}

NODE_API_MODULE(nbs_decoder, Init)
