// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Error message dialog.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>
#include <retrodev.gui.h>

namespace RetrodevGui {

	class ErrorDialog {
	public:
		static void Show(const std::string& message);
		static void Render();

	private:
		static inline std::string m_message;
	};

}