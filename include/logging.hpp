#pragma once
#include <string_view>
#include "paper/shared/logger.hpp"

#define INFO(str, ...) Paper::Logger::fmtLog<Paper::LogLevel::INF>(str __VA_OPT__(, __VA_ARGS__))
#define ERROR(str, ...) Paper::Logger::fmtLog<Paper::LogLevel::ERR>(str __VA_OPT__(, __VA_ARGS__))
#define CRITICAL(str, ...) Paper::Logger::fmtLog<Paper::LogLevel::ERR>(str __VA_OPT__(, __VA_ARGS__))
#define DEBUG(str, ...) Paper::Logger::fmtLog<Paper::LogLevel::DBG>(str __VA_OPT__(, __VA_ARGS__))
#define WARNING(str, ...) Paper::Logger::fmtLog<Paper::LogLevel::WRN>(str __VA_OPT__(, __VA_ARGS__))
