//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>
#include <retrodev.lib.h>
#include <memory>

namespace RetrodevGui {
	//
	// Result returned by TileExtractionWidget::Render
	//
	struct TileExtractionWidgetResult {
		bool parametersChanged = false;
		bool gridStructureChanged = false;
		bool extractRequested = false;
	};
	//
	// Tile extraction parameters widget
	// Allows editing of tile extraction settings (size, offset, padding)
	//
	class TileExtractionWidget {
	public:
		//
		// Render the tile extraction parameters UI
		// tileParams: pointer to tile extraction parameters to edit
		// tileExtractor: tile extractor instance for displaying info
		// converter: bitmap converter for getting image dimensions
		// params: conversion parameters for getting converted image
		// Returns result indicating if parameters changed or extraction requested
		//
		static TileExtractionWidgetResult Render(RetrodevLib::TileExtractionParams* tileParams, std::shared_ptr<RetrodevLib::ITileExtractor> tileExtractor,
												 std::shared_ptr<RetrodevLib::IBitmapConverter> converter, RetrodevLib::GFXParams* params);

	private:
		//
		// Collapsing header states (persistent across frames)
		//
		static bool m_extractionOpen;
		//
		// Pixel-grid constraint state (persistent across frames)
		//
		static bool m_constrainToGrid;
		static int m_constraintW;
		static int m_constraintH;
		//
		// Previous constraint values — used to detect mode changes and reset the checkbox
		//
		static int m_prevConstraintW;
		static int m_prevConstraintH;
		//
		// Previous values for direction-aware snapping
		//
		static int m_prevTileWidth;
		static int m_prevTileHeight;
	};
} // namespace RetrodevGui
