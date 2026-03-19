// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <cstdarg>

namespace RetrodevLib {

	//
	// Log message severity levels
	// Matches the three levels used by AppConsole on the GUI side
	//
	enum class LogLevel { Info, Warning, Error };
	//
	// Log channel — identifies which subsystem emitted the message.
	// The host can use this to route messages to separate console tabs.
	//   General — general library messages (project, converters, etc.)
	//   Build   — assembler / compiler output
	//   Script  — AngelScript engine and export script output
	//
	enum class LogChannel { General, Build, Script };
	//
	// Unified callback type for all log events from the library.
	// All subsystems (general, build, script) share this single callback.
	// level:   severity of the message
	// channel: originating subsystem
	// message: null-terminated, already-formatted UTF-8 string
	//
	using LogCallback = void (*)(LogLevel level, LogChannel channel, const char* message);
	//
	// Internal library logger
	//
	// Usage inside the library — General channel (default):
	//   Log::Info("Loaded %d sprites", count);
	//   Log::Warning("Texture size %dx%d exceeds limit", w, h);
	//   Log::Error("Failed to open file: %s", path);
	//
	// Usage with explicit channel:
	//   Log::Info(LogChannel::Build, "Assembling %s", file);
	//   Log::Error(LogChannel::Script, "Script error: %s", msg);
	//
	// Usage from the host application:
	//   RetrodevLib::Log::SetCallback([](RetrodevLib::LogLevel level, RetrodevLib::LogChannel channel, const char* msg) {
	//       // route to appropriate console channel based on channel
	//   });
	//
	class Log {
	public:
		//
		// Register the application callback that receives all log events from every channel.
		// Pass nullptr to unregister; messages are silently dropped when no callback is set.
		//
		static void SetCallback(LogCallback callback);
		//
		// Log an informational message on the General channel (printf-style format string)
		//
		static void Info(const char* format, ...);
		//
		// Log an informational message on the specified channel (printf-style format string)
		//
		static void Info(LogChannel channel, const char* format, ...);
		//
		// Log a warning message on the General channel (printf-style format string)
		//
		static void Warning(const char* format, ...);
		//
		// Log a warning message on the specified channel (printf-style format string)
		//
		static void Warning(LogChannel channel, const char* format, ...);
		//
		// Log an error message on the General channel (printf-style format string)
		//
		static void Error(const char* format, ...);
		//
		// Log an error message on the specified channel (printf-style format string)
		//
		static void Error(LogChannel channel, const char* format, ...);

	private:
		//
		// Internal formatter — invokes the registered callback with the formatted message
		//
		static void Write(LogLevel level, LogChannel channel, const char* format, va_list args);
		//
		// Currently registered callback (nullptr = no output)
		//
		static inline LogCallback m_callback = nullptr;
	};

} // namespace RetrodevLib
