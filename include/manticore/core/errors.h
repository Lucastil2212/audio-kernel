#pragma once

#include <string>

#include "manticore/engine/runtime_config.h"

namespace manticore {

enum class ErrorCode {
    Ok,
    InvalidConfig,
    BackendOpenFailed,
    BackendStartFailed,
    FileOpenFailed,
    FileWriteFailed,
    FileReadFailed,
    InvalidParameter
};

inline const char* errorCodeToString(ErrorCode code)
{
    switch (code) {
        case ErrorCode::Ok: return "Ok";
        case ErrorCode::InvalidConfig: return "InvalidConfig";
        case ErrorCode::BackendOpenFailed: return "BackendOpenFailed";
        case ErrorCode::BackendStartFailed: return "BackendStartFailed";
        case ErrorCode::FileOpenFailed: return "FileOpenFailed";
        case ErrorCode::FileWriteFailed: return "FileWriteFailed";
        case ErrorCode::FileReadFailed: return "FileReadFailed";
        case ErrorCode::InvalidParameter: return "InvalidParameter";
    }
    return "Unknown";
}

bool validateRuntimeConfig(const RuntimeConfig& config, std::string& errorMessage);

}  // namespace manticore
