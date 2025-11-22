#pragma once

// SPDLOG
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <spdlog/spdlog.h>

#include <string>
#include <memory>
#include <windows.h>

// Logger wrapper around spdlog.
// Preserves the existing static interface and printf-style formatting for compatibility.
class Logger {
public:
    // Initialize the logger (sinks, patterns, etc.)
    static void Initialize();

    // Wide char versions (printf-style)
    template<typename... Args>
    static void Info(const wchar_t* fmt, Args... args) {
        if (auto logger = spdlog::default_logger()) {
            logger->info(FormatString(fmt, args...));
        }
    }

    template<typename... Args>
    static void Warn(const wchar_t* fmt, Args... args) {
        if (auto logger = spdlog::default_logger()) {
            logger->warn(FormatString(fmt, args...));
        }
    }

    template<typename... Args>
    static void Error(const wchar_t* fmt, Args... args) {
        if (auto logger = spdlog::default_logger()) {
            logger->error(FormatString(fmt, args...));
        }
    }

    template<typename... Args>
    static void Debug(const wchar_t* fmt, Args... args) {
#ifdef _DEBUG
        if (auto logger = spdlog::default_logger()) {
            logger->debug(FormatString(fmt, args...));
        }
#endif
    }

    // ASCII versions (auto-convert to wide for consistency)
    static void Info(const char* msg) { 
        if (auto logger = spdlog::default_logger()) logger->info(msg); 
    }
    static void Warn(const char* msg) { 
        if (auto logger = spdlog::default_logger()) logger->warn(msg); 
    }
    static void Error(const char* msg) { 
        if (auto logger = spdlog::default_logger()) logger->error(msg); 
    }
    static void Debug(const char* msg) { 
#ifdef _DEBUG
        if (auto logger = spdlog::default_logger()) logger->debug(msg);
#endif
    }

    // Helper for HRESULT logging
    static void LogResult(const wchar_t* context, HRESULT hr) {
        if (FAILED(hr)) {
            Error(L"%s failed. HRESULT: 0x%08X", context, static_cast<unsigned long>(hr));
        }
    }

private:
    // Helper to keep using printf-style formatting with spdlog for now
    template<typename... Args>
    static std::wstring FormatString(const wchar_t* fmt, Args... args) {
        int size = _scwprintf(fmt, args...);
        if (size <= 0) return std::wstring(fmt);
        
        size += 1; 
        std::unique_ptr<wchar_t[]> buf(new wchar_t[size]); 
        swprintf_s(buf.get(), size, fmt, args...);
        
        return std::wstring(buf.get());
    }
    
    static std::wstring FormatString(const wchar_t* fmt) {
        return std::wstring(fmt);
    }
};
