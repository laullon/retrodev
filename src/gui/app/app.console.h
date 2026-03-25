// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Console output panel -- captures and displays build/script/find log messages.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>
#include <string>
#include <vector>

namespace RetrodevGui {

	//
	// Console log window with per-channel tab display
	// Channels: Output (general), Find (search results), Script (script engine), Build (assembler)
	//
	class AppConsole {
	public:
		//
		// Log message severity levels
		//
		enum class LogLevel { Info, Warning, Error };
		//
		// Output channels -- each has its own entry list
		// Find, Script and Build are cleared before each new operation
		//
		enum class Channel { Output, Find, Script, Build };

		//
		// Render the console contents (to be called from within an existing child window)
		//
		static void Render();
		//
		// Add a message to the Output channel
		//
		static void AddLog(LogLevel level, const char* message);
		//
		// Add a formatted message to the Output channel (printf-style)
		//
		static void AddLogF(LogLevel level, const char* format, ...);
		//
		// Add a message to a specific channel
		//
		static void AddLog(Channel channel, LogLevel level, const char* message);
		//
		// Add a formatted message to a specific channel (printf-style)
		//
		static void AddLogF(Channel channel, LogLevel level, const char* format, ...);
		//
		// Clear all entries in all channels
		//
		static void Clear();
		//
		// Clear all entries in a specific channel
		// Call this before starting a new Find, Script or Build operation
		//
		static void Clear(Channel channel);
		//
		// Returns true once when a warning or error was logged to any channel,
		// consuming the request. The caller should uncollapse and switch to outChannel.
		//
		static bool TakeRevealRequest(Channel& outChannel);
		//
		// Programmatically switch the active channel tab
		//
		static void SetActiveChannel(Channel channel);
		//
		// Returns true when the auto-hide behavior is active
		//
		static bool GetAutoHide();
		//
		// Set the auto-hide state (used by INI restore)
		//
		static void SetAutoHide(bool value);

	private:
		//
		// Single log entry with message and severity level
		//
		struct LogEntry {
			std::string message;
			LogLevel level;
			LogEntry(const std::string& msg, LogLevel lvl) : message(msg), level(lvl) {}
		};
		//
		// Per-channel entry lists
		//
		static std::vector<LogEntry> m_channels[4];
		//
		// Currently selected tab
		//
		static Channel m_activeChannel;
		//
		// Auto-scroll to bottom when new messages arrive
		//
		static bool m_autoScroll;
		//
		// Minimum log level to display (levels below this are hidden)
		//
		static LogLevel m_filterLevel;
		//
		// Pending reveal -- set when a warning or error is logged, consumed by TakeRevealRequest
		//
		static bool m_revealPending;
		static Channel m_revealChannel;
		//
		// Pending programmatic tab switch -- consumed by the tab bar on the next render
		//
		static bool m_pendingChannelSwitch;
		//
		// Auto-hide: collapse the panel when the user clicks outside it
		//
		static bool m_autoHide;
		//
		// Returns true when a message on the given channel and level should trigger a reveal.
		// Single source of truth used by all AddLog / AddLogF overloads.
		//
		static bool ShouldReveal(Channel channel, LogLevel level);
		//
		// Render the entry list for a given channel
		//
		static void RenderChannel(Channel channel);
		//
		// Get ImGui color for the given log level
		//
		static ImVec4 GetColorForLevel(LogLevel level);
	};

}
