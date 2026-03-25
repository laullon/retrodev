// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Tile extraction widget -- grid parameters and extract controls.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

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
		bool removeDuplicatesRequested = false;
		bool undeleteAllRequested = false;
		bool packToGridRequested = false;
		bool packSampleBg = false;
		bool packEnabled = false;
		float packBgR = 1.0f;
		float packBgG = 0.0f;
		float packBgB = 1.0f;
		int packMergeGap = 0;
		int packCellPad = 0;
		int packColumns = 0;
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
		//
		// Set the pack background colour from outside (e.g. after sampling pixel 0,0)
		//
		static void SetPackBg(float r, float g, float b) {
			m_packBgR = r;
			m_packBgG = g;
			m_packBgB = b;
		}

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
		// Previous constraint values -- used to detect mode changes and reset the checkbox
		//
		static int m_prevConstraintW;
		static int m_prevConstraintH;
		//
		// Previous values for direction-aware snapping
		//
		static int m_prevTileWidth;
		static int m_prevTileHeight;
		//
		// Pack-to-Grid persistent settings (separator colour + options)
		//
		static float m_packBgR;
		static float m_packBgG;
		static float m_packBgB;
		static int m_packMergeGap;
		static int m_packCellPad;
		static int m_packColumns;
		static bool m_packBgPickedFromImage;
		//
		// Last tileParams pointer -- used to detect when a new document is loaded
		// so pack settings can be synchronised from the persisted params.
		//
		static RetrodevLib::TileExtractionParams* m_lastTileParams;
	};
}
