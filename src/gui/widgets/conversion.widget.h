// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Conversion widget -- shared bitmap conversion parameter UI.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>
#include <retrodev.lib.h>

namespace RetrodevGui {

	//
	// Result structure for conversion widget render
	// Contains information about parameter changes and renames
	//
	struct ConversionWidgetResult {
		bool parametersChanged = false; // True if any conversion parameter was modified
		bool itemRenamed = false;		// True if the build item was renamed
		std::string newName;			// New name if itemRenamed is true
	};

	//
	// Conversion widget for displaying and editing image conversion parameters
	//
	class ConversionWidget {
	public:
		//
		// Render the conversion widget contents (to be called from within an existing child window)
		// params: conversion parameters
		// converter: bitmap converter instance (optional)
		// buildItemName: name of the build item being edited (for renaming)
		// buildType: type of build item (Bitmap, Tilemap, Sprite)
		// Returns a result structure with change and rename information
		//
		static ConversionWidgetResult Render(RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IBitmapConverter> converter, const std::string& buildItemName,
											 RetrodevLib::ProjectBuildType buildType);

	private:
		//
		// Collapsible section states
		//
		static bool m_targetOpen;
		static bool m_resizeOpen;
		static bool m_quantizationOpen;
		static bool m_ditheringOpen;
		static bool m_colorCorrectionOpen;
		//
		// Render individual sections
		// All return a result structure with change and rename information
		//
		static ConversionWidgetResult RenderTargetConversion(RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IBitmapConverter> converter,
															 const std::string& buildItemName, RetrodevLib::ProjectBuildType buildType);
		static bool RenderResize(RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IBitmapConverter> converter);
		static bool RenderQuantization(RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IBitmapConverter> converter);
		static bool RenderDithering(RetrodevLib::GFXParams* params);
		static bool RenderColorCorrection(RetrodevLib::GFXParams* params);
	};

}
