// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Help dialog -- built-in documentation viewer with topic navigator.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>
#include <SDL3/SDL.h>

namespace RetrodevGui {

	class HelpDialog {
	public:
		//
		// Open the dialog on the next Render() call
		//
		static void Show();
		//
		// Render the dialog; call every frame from the menu render function
		//
		static void Render();

	private:
		//
		// Render a clickable cross-reference link that jumps to the given topic index
		//
		static void RenderLink(const char* label, int topicIndex);
		//
		// Render topic body text with hanging-indent support for two-column lines
		//
		static void RenderContent(const char* text);
		//
		// True when the dialog should open on the next frame
		//
		static inline bool m_open = false;
		//
		// Index of the currently selected topic in the topics table
		//
		static inline int m_selectedTopic = 0;
		//
		// When >= 0 the content panel scrolls to the top on the next frame (used by links)
		//
		static inline bool m_scrollToTop = false;
		//
		// Optional logo texture loaded from the "gui.res.images.doc-logo.png" embedded resource
		//
		static inline SDL_Texture* m_logoTexture = nullptr;
		//
		// True once the resource load has been attempted (success or failure)
		//
		static inline bool m_logoLoaded = false;
	};

}
