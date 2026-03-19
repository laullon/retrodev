// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

//
// Suppress intentionally-unused parameter warnings for parameters reserved for future use.
// Usage: UNUSED(paramName);
//
#define UNUSED(x) (void)(x)

//
// Logging — must be included first so Log::Info/Warning/Error are
// available to every other subsystem header that follows
//
#include <log/log.h>

//
// Image functions
// Is used as source data for conversion and also to store
// a preview of the converted graphic
//
#include <assets/image/image.h>

//
// Project functions
// All related to project management (load, save, add items,etc)
//
#include <project/project.h>

//
// Converter functions
// All related to convert assets to a target system
//
#include <convert/converters.h>

//
// Image processors
//
#include <process/image/resize.h>
#include <process/image/quantization.h>
#include <process/image/dithering.h>
#include <process/image/color.correction.h>

namespace RetrodevLib {
	//
	// Initialize and shutdown the RetroDev library
	//
	bool RetroDevInit();
	void RetroDevShutdown();
} // namespace RetrodevLib