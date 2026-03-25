// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Application entry point and main SDL/ImGui loop.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>

namespace RetrodevGui {

	class Application {
	public:
		Application() = default;
		~Application() = default;

		static bool Initialize();
		static void Run();
		static void Shutdown();

		// Get the SDL renderer for texture creation
		static SDL_Renderer* GetRenderer() { return renderer; }

		// Thickness of the splitters between panels
		inline static float splitterThickness = 4.0f;

		// Default UI font (Ubuntu Medium)
		inline static ImFont* AppFont = nullptr;

		// Monospaced font used by the text editor (Fixedsys Excelsior Mono)
		inline static ImFont* EditorFont = nullptr;

	private:
		// Setup ImGui custom Amstrad CPC-inspired color scheme and style
		static void SetupImGuiStyle(float scale);

		// Display DPI scaling factor for high-DPI displays
		inline static float screenScale = 1.0f;

		// Application window title
		inline static std::string title = "RetroDev";

		// Base window width in logical pixels
		inline static int width = 1280;

		// Base window height in logical pixels
		inline static int height = 800;

		// SDL window handle
		inline static SDL_Window* window = nullptr;

		// SDL renderer for hardware-accelerated rendering
		inline static SDL_Renderer* renderer = nullptr;

		// ImGui context for the UI framework
		inline static ImGuiContext* imguiContext = nullptr;

		// Absolute path to retrodev.ini -- must outlive the ImGui context
		inline static std::string iniFilePath;
	};

}