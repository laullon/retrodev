// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Shared GUI includes and forward declarations.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <cstring>

//
// ImGui and SDL3 includes
//
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <imgui.splitter.h>
#include <imgui.zoomable.image.h>
#include <imgui.filedialog.h>
#include <SDL3/SDL.h>

//
// Our helper library to build and manipulate the project
//
#include <retrodev.lib.h>

//
// Log facility
//
#include <app/app.console.h>

#if !defined(_WIN32)
inline int strncpy_s(char* dest, const char* src, size_t count) {
	if (!dest || count == 0) {
		return 0;
	}
	std::strncpy(dest, src ? src : "", count);
	dest[count] = '\0';
	return 0;
}
#endif
