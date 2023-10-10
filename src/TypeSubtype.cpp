
#include "TypeSubtype.hpp"

namespace nbs {

    TypeSubtype TypeSubtype::FromJsValue(const Napi::Value& jsValue, const Napi::Env& env) {
        if (!jsValue.IsObject()) {
            throw std::runtime_error("expected object");
        }

        auto typeSubtype = jsValue.As<Napi::Object>();

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

    std::vector<TypeSubtype> TypeSubtype::FromJsArray(const Napi::Array& jsArray, const Napi::Env& env) {

        // Container for return values
        std::vector<TypeSubtype> types;

        // Attempt to convert each value to TypeSubtype
        for (std::size_t i = 0; i < jsArray.Length(); i++) {
            types.push_back(TypeSubtype::FromJsValue(jsArray.Get(i), env));
        }

        return types;
    }
}
