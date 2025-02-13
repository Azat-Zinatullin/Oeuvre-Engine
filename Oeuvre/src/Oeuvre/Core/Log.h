#pragma once

#include "spdlog/spdlog.h"

// Core log macros
#define OV_CORE_TRACE(...)    spdlog::trace(__VA_ARGS__)
#define OV_CORE_INFO(...)     spdlog::info(__VA_ARGS__)
#define OV_CORE_WARN(...)     spdlog::warn(__VA_ARGS__)
#define OV_CORE_ERROR(...)    spdlog::error(__VA_ARGS__)
#define OV_CORE_CRITICAL(...) spdlog::critical(__VA_ARGS__)

// Client log macros
#define OV_TRACE(...)         spdlog::trace(__VA_ARGS__)
#define OV_INFO(...)          spdlog::info(__VA_ARGS__)
#define OV_WARN(...)          spdlog::warn(__VA_ARGS__)
#define OV_ERROR(...)         spdlog::error(__VA_ARGS__)
#define OV_CRITICAL(...)      spdlog::critical(__VA_ARGS__)