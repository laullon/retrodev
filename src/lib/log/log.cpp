// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Log subsystem -- channel-based message routing to the console panel.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "log.h"
#include <cstdio>

namespace RetrodevLib {

	void Log::SetCallback(LogCallback callback) {
		m_callback = callback;
	}
	//
	// Public logging methods -- General channel variants start a va_list and delegate to Write
	//
	void Log::Info(const char* format, ...) {
		va_list args;
		va_start(args, format);
		Write(LogLevel::Info, LogChannel::General, format, args);
		va_end(args);
	}
	void Log::Info(LogChannel channel, const char* format, ...) {
		va_list args;
		va_start(args, format);
		Write(LogLevel::Info, channel, format, args);
		va_end(args);
	}
	void Log::Warning(const char* format, ...) {
		va_list args;
		va_start(args, format);
		Write(LogLevel::Warning, LogChannel::General, format, args);
		va_end(args);
	}
	void Log::Warning(LogChannel channel, const char* format, ...) {
		va_list args;
		va_start(args, format);
		Write(LogLevel::Warning, channel, format, args);
		va_end(args);
	}
	void Log::Error(const char* format, ...) {
		va_list args;
		va_start(args, format);
		Write(LogLevel::Error, LogChannel::General, format, args);
		va_end(args);
	}
	void Log::Error(LogChannel channel, const char* format, ...) {
		va_list args;
		va_start(args, format);
		Write(LogLevel::Error, channel, format, args);
		va_end(args);
	}
	//
	// Format the message and forward it to the registered callback
	// Messages are silently dropped when m_callback is nullptr
	//
	void Log::Write(LogLevel level, LogChannel channel, const char* format, va_list args) {
		if (m_callback == nullptr)
			return;
		char buf[2048];
		std::vsnprintf(buf, sizeof(buf), format, args);
		m_callback(level, channel, buf);
	}

}
