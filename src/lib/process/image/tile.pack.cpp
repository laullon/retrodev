// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Tile pack-to-grid -- detects content regions and rearranges them into a regular grid.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "tile.pack.h"
#include <log/log.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <unordered_map>

namespace RetrodevLib {
	//
	// Returns true when the pixel colour matches the background within tolerance
	//
	static bool IsBackground(RgbColor pixel, RgbColor bg, int tolerance) {
		int dr = (int)pixel.r - (int)bg.r;
		int dg = (int)pixel.g - (int)bg.g;
		int db = (int)pixel.b - (int)bg.b;
		if (dr < 0)
			dr = -dr;
		if (dg < 0)
			dg = -dg;
		if (db < 0)
			db = -db;
		return (dr <= tolerance && dg <= tolerance && db <= tolerance);
	}
	//
	// Detect content bounding boxes in the image using union-find connected-components.
	//
	// Strategy:
	//   1. Build a bool mask: true = content pixel (not background and not transparent).
	//   2. Dilate the mask by mergeGap pixels (square structuring element) so that regions
	//      within mergeGap pixels of each other in both X and Y become connected.
	//   3. Run union-find (4-connectivity) on the dilated mask to label each connected group.
	//   4. For every original content pixel (from step 1), expand the bounding box of its
	//      root label. Using original pixels (not dilated) keeps bounding boxes tight.
	//   5. Sort results top-left to bottom-right (row-major order).
	//
	static std::vector<TilePackRegion> DetectRegions(std::shared_ptr<Image> img, RgbColor bg, int bgTolerance, int mergeGap) {
		const int W = img->GetWidth();
		const int H = img->GetHeight();
		//
		// Build boolean content mask: true = non-background, non-transparent pixel.
		// If the image contains any fully-transparent pixels, use alpha=0 as the sole
		// background signal (the converter already keyed out the background colour).
		// Otherwise fall back to colour + tolerance matching against bg.
		//
		std::vector<bool> mask(W * H, false);
		{
			auto access = img->LockPixels();
			const uint8_t* pixels = access.Pixels;
			const int pitch = access.Pitch;
			const bool isPaletized = img->IsPaletized();
			//
			// First pass: check whether the image uses alpha transparency for the background
			//
			bool hasTransparentPixels = false;
			if (!isPaletized) {
				for (int y = 0; y < H && !hasTransparentPixels; y++) {
					const uint8_t* row = pixels + y * pitch;
					for (int x = 0; x < W && !hasTransparentPixels; x++) {
						if (row[x * 4 + 3] == 0)
							hasTransparentPixels = true;
					}
				}
			}
			//
			// Second pass: fill content mask
			//
			for (int y = 0; y < H; y++) {
				const uint8_t* row = pixels + y * pitch;
				for (int x = 0; x < W; x++) {
					if (isPaletized) {
						int idx = row[x];
						RgbColor px = img->GetPaletteColor(idx);
						if (px.a == 0)
							continue;
						if (!IsBackground(px, bg, bgTolerance))
							mask[y * W + x] = true;
					} else if (hasTransparentPixels) {
						//
						// Alpha-keyed image: only fully transparent pixels are background;
						// every opaque pixel (including the original bg colour) is content.
						// This lets the converter's transparent-colour pass act as the mask.
						//
						if (row[x * 4 + 3] != 0)
							mask[y * W + x] = true;
					} else {
						//
						// No alpha key: compare to the stored background colour + tolerance
						//
						RgbColor px(row[x * 4 + 0], row[x * 4 + 1], row[x * 4 + 2], row[x * 4 + 3]);
						if (!IsBackground(px, bg, bgTolerance))
							mask[y * W + x] = true;
					}
				}
			}
		}
		//
		// Dilate the mask by mergeGap pixels so that nearby regions become connected.
		// The union-find then naturally merges them into one component.
		//
		std::vector<bool> dilated = mask;
		if (mergeGap > 0) {
			for (int y = 0; y < H; y++) {
				for (int x = 0; x < W; x++) {
					if (!mask[y * W + x])
						continue;
					//
					// Expand a square of radius mergeGap around each content pixel
					//
					int y0 = (y - mergeGap < 0) ? 0 : y - mergeGap;
					int y1 = (y + mergeGap >= H) ? H - 1 : y + mergeGap;
					int x0 = (x - mergeGap < 0) ? 0 : x - mergeGap;
					int x1 = (x + mergeGap >= W) ? W - 1 : x + mergeGap;
					for (int dy = y0; dy <= y1; dy++) {
						for (int dx = x0; dx <= x1; dx++)
							dilated[dy * W + dx] = true;
					}
				}
			}
		}
		//
		// Union-Find with path compression and union-by-rank on the dilated mask
		//
		const int N = W * H;
		std::vector<int> parent(N, -1);
		std::vector<int> rnk(N, 0);
		auto Find = [&](int a) -> int {
			while (parent[a] != a) {
				parent[a] = parent[parent[a]];
				a = parent[a];
			}
			return a;
		};
		auto Union = [&](int a, int b) {
			a = Find(a);
			b = Find(b);
			if (a == b)
				return;
			if (rnk[a] < rnk[b])
				std::swap(a, b);
			parent[b] = a;
			if (rnk[a] == rnk[b])
				rnk[a]++;
		};
		//
		// Two-pass 4-connectivity scan on the dilated mask
		//
		for (int y = 0; y < H; y++) {
			for (int x = 0; x < W; x++) {
				int i = y * W + x;
				if (!dilated[i])
					continue;
				parent[i] = i;
				if (x > 0 && dilated[i - 1])
					Union(i, i - 1);
				if (y > 0 && dilated[i - W])
					Union(i, i - W);
			}
		}
		//
		// Collect bounding boxes from ORIGINAL content pixels (not dilated),
		// grouped by the root label assigned through the dilated mask
		//
		struct BBox {
			int x1, y1, x2, y2;
		};
		std::unordered_map<int, BBox> boxes;
		boxes.reserve(64);
		for (int y = 0; y < H; y++) {
			for (int x = 0; x < W; x++) {
				int i = y * W + x;
				if (!mask[i])
					continue;
				//
				// The dilated mask may not cover this pixel if mergeGap=0 and it is isolated,
				// but in that case it is its own root via the original mask scan.
				// Use dilated parent if available, else skip (isolated pixel not in dilated set).
				//
				if (parent[i] == -1)
					continue;
				int root = Find(i);
				auto it = boxes.find(root);
				if (it == boxes.end()) {
					boxes[root] = {x, y, x, y};
				} else {
					BBox& b = it->second;
					if (x < b.x1)
						b.x1 = x;
					if (y < b.y1)
						b.y1 = y;
					if (x > b.x2)
						b.x2 = x;
					if (y > b.y2)
						b.y2 = y;
				}
			}
		}
		//
		// Convert bounding boxes to TilePackRegion list
		//
		std::vector<TilePackRegion> regions;
		regions.reserve(boxes.size());
		for (auto& kv : boxes) {
			const BBox& b = kv.second;
			regions.push_back({b.x1, b.y1, b.x2 - b.x1 + 1, b.y2 - b.y1 + 1});
		}
		//
		// Sort top-left to bottom-right (row-major order)
		//
		std::sort(regions.begin(), regions.end(), [](const TilePackRegion& a, const TilePackRegion& b) {
			if (a.y != b.y)
				return a.y < b.y;
			return a.x < b.x;
		});
		Log::Info("[TilePack] DetectRegions: found %d region(s) (bg=%d,%d,%d tol=%d gap=%d).", (int)regions.size(), (int)bg.r, (int)bg.g, (int)bg.b, bgTolerance, mergeGap);
		return regions;
	}
	//
	// Pack detected regions into a new uniform-grid image
	//
	TilePackResult PackToGrid(std::shared_ptr<Image> sourceImage, RgbColor bgColor, int bgTolerance, int mergeGap, int cellPadding, int gridColumns) {
		TilePackResult result;
		if (!sourceImage) {
			Log::Warning("[TilePack] PackToGrid called with null source image.");
			return result;
		}
		//
		// Detect content bounding boxes
		//
		std::vector<TilePackRegion> regions = DetectRegions(sourceImage, bgColor, bgTolerance, mergeGap);
		if (regions.empty()) {
			Log::Warning("[TilePack] No content regions detected.");
			return result;
		}
		Log::Info("[TilePack] Detected %d region(s).", (int)regions.size());
		//
		// Find the minimum region dimensions -- this is the base tile unit.
		// Any region larger than minW x minH is a composite (e.g. a background made of
		// several tiles side-by-side) and must be subdivided into uniform sub-cells.
		//
		int minW = regions[0].w;
		int minH = regions[0].h;
		for (auto& r : regions) {
			if (r.w < minW)
				minW = r.w;
			if (r.h < minH)
				minH = r.h;
		}
		{
			std::vector<TilePackRegion> subdivided;
			subdivided.reserve(regions.size() * 4);
			for (auto& r : regions) {
				int subCols = (minW > 0) ? (r.w / minW) : 1;
				int subRows = (minH > 0) ? (r.h / minH) : 1;
				if (subCols < 1)
					subCols = 1;
				if (subRows < 1)
					subRows = 1;
				for (int sy = 0; sy < subRows; sy++) {
					for (int sx = 0; sx < subCols; sx++)
						subdivided.push_back({r.x + sx * minW, r.y + sy * minH, minW, minH});
				}
			}
			if ((int)subdivided.size() != (int)regions.size())
				Log::Info("[TilePack] Subdivision: %d region(s) -> %d sub-cell(s) (unit %dx%d).", (int)regions.size(), (int)subdivided.size(), minW, minH);
			regions = std::move(subdivided);
		}
		//
		// Compute the cell dimensions: all sub-cells are now uniform (minW x minH)
		//
		int cellW = minW;
		int cellH = minH;
		//
		// Determine grid layout
		//
		int N = (int)regions.size();
		int cols = (gridColumns > 0) ? gridColumns : (int)std::ceil(std::sqrt((double)N));
		if (cols < 1)
			cols = 1;
		int rows = (N + cols - 1) / cols;
		//
		// Create output image filled with background colour
		//
		int pad = (cellPadding > 0) ? cellPadding : 0;
		int outW = cols * cellW + (cols > 1 ? (cols - 1) * pad : 0);
		int outH = rows * cellH + (rows > 1 ? (rows - 1) * pad : 0);
		auto out = Image::ImageCreate(outW, outH);
		if (!out) {
			Log::Error("[TilePack] Failed to create output image (%dx%d).", outW, outH);
			return result;
		}
		//
		// Fill and copy using direct pixel access for performance.
		// Output is always RGBA32 (ImageCreate default).
		// Source may be RGBA32 or INDEX8; handle both cases.
		//
		{
			auto dstAccess = out->LockPixels();
			uint8_t* dst = dstAccess.Pixels;
			const int dstPitch = dstAccess.Pitch;
			//
			// Fill destination with background colour (RGBA32: R G B A)
			//
			for (int y = 0; y < outH; y++) {
				uint8_t* row = dst + y * dstPitch;
				for (int x = 0; x < outW; x++) {
					row[x * 4 + 0] = bgColor.r;
					row[x * 4 + 1] = bgColor.g;
					row[x * 4 + 2] = bgColor.b;
					row[x * 4 + 3] = bgColor.a;
				}
			}
			const int srcW = sourceImage->GetWidth();
			const int srcH = sourceImage->GetHeight();
			const bool srcPaletized = sourceImage->IsPaletized();
			if (!srcPaletized) {
				//
				// RGBA32 source: fast row memcpy directly into destination
				//
				auto srcAccess = sourceImage->LockPixels();
				const uint8_t* src = srcAccess.Pixels;
				const int srcPitch = srcAccess.Pitch;
				for (int i = 0; i < N; i++) {
					const TilePackRegion& r = regions[i];
					int col = i % cols;
					int row = i / cols;
					int dstOriginX = col * (cellW + pad);
					int dstOriginY = row * (cellH + pad);
					int offX = (cellW - r.w) / 2;
					int offY = (cellH - r.h) / 2;
					for (int py = 0; py < r.h; py++) {
						int sy = r.y + py;
						if (sy >= srcH)
							continue;
						int dy = dstOriginY + offY + py;
						if (dy < 0 || dy >= outH)
							continue;
						const uint8_t* srcRow = src + sy * srcPitch + r.x * 4;
						uint8_t* dstRow = dst + dy * dstPitch + (dstOriginX + offX) * 4;
						int copyW = r.w;
						if (r.x + copyW > srcW)
							copyW = srcW - r.x;
						if (dstOriginX + offX + copyW > outW)
							copyW = outW - dstOriginX - offX;
						if (copyW > 0)
							std::memcpy(dstRow, srcRow, (size_t)copyW * 4);
					}
				}
			} else {
				//
				// INDEX8 source: resolve each pixel through the palette
				//
				for (int i = 0; i < N; i++) {
					const TilePackRegion& r = regions[i];
					int col = i % cols;
					int row = i / cols;
					int dstOriginX = col * (cellW + pad);
					int dstOriginY = row * (cellH + pad);
					int offX = (cellW - r.w) / 2;
					int offY = (cellH - r.h) / 2;
					for (int py = 0; py < r.h; py++) {
						int sy = r.y + py;
						if (sy >= srcH)
							continue;
						int dy = dstOriginY + offY + py;
						if (dy < 0 || dy >= outH)
							continue;
						uint8_t* dstRow = dst + dy * dstPitch + (dstOriginX + offX) * 4;
						for (int px = 0; px < r.w; px++) {
							int sx = r.x + px;
							if (sx >= srcW)
								break;
							RgbColor c = sourceImage->GetPixelColor(sx, sy);
							dstRow[px * 4 + 0] = c.r;
							dstRow[px * 4 + 1] = c.g;
							dstRow[px * 4 + 2] = c.b;
							dstRow[px * 4 + 3] = c.a;
						}
					}
				}
			}
		}
		//
		// Mark the output image modified so GetTexture uploads it on first use
		//
		out->MarkModified();
		//
		// Fill result
		//
		result.packedImage = out;
		result.cellWidth = cellW;
		result.cellHeight = cellH;
		result.cellPadding = pad;
		result.regionCount = N;
		Log::Info("[TilePack] Packed %d region(s) into grid cols=%d rows=%d, cell %dx%d, output %dx%d.", N, cols, rows, cellW, cellH, outW, outH);
		return result;
	}
}
