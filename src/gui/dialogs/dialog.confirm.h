// --------------------------------------------------------------------------------------------------------------
//
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>
#include <functional>
#include <retrodev.gui.h>

namespace RetrodevGui {

	//
	// Modal confirmation dialog.
	// Call Show() to queue a prompt; the supplied callback is invoked when the user clicks Confirm.
	// Call Render() once per frame from the main render loop.
	//
	class ConfirmDialog {
	public:
		//
		// Queue a confirmation prompt.
		// message: question shown to the user.
		// onConfirm: callback invoked when the user clicks Confirm.
		//
		static void Show(const std::string& message, std::function<void()> onConfirm);
		//
		// Render the modal; must be called once per frame from the top-level render loop.
		//
		static void Render();

	private:
		static inline std::string m_message;
		static inline std::function<void()> m_onConfirm;
	};

} // namespace RetrodevGui
