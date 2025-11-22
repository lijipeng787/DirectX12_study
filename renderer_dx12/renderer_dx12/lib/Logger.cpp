#include "stdafx.h"
#include "Logger.h"

#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <vector>

void Logger::Initialize() {
    try {
        auto sinks = std::vector<spdlog::sink_ptr>();
        
        // 1. MSVC Output Window Sink (replaces OutputDebugString)
        auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
        sinks.push_back(msvc_sink);

        // 2. File Sink (renderer.log)
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("renderer.log", true);
        sinks.push_back(file_sink);

        // 3. Console Sink (if console is allocated)
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        sinks.push_back(console_sink);

        // Create logger
        auto logger = std::make_shared<spdlog::logger>("dx12", begin(sinks), end(sinks));
        
        // Set pattern: [Time] [Level] Message
        logger->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
        
        // Set level
#ifdef _DEBUG
        logger->set_level(spdlog::level::debug);
#else
        logger->set_level(spdlog::level::info);
#endif
        
        // Register as default
        spdlog::set_default_logger(logger);
        spdlog::flush_on(spdlog::level::info);

        Logger::Info(L"Logger initialized via spdlog.");
        
    } catch (const spdlog::spdlog_ex& ex) {
        // Fallback if spdlog fails
        OutputDebugStringA("Logger initialization failed: ");
        OutputDebugStringA(ex.what());
        OutputDebugStringA("\n");
    }
}
