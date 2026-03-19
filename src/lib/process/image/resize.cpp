//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#include <utils/utils.h>
#include <SDL3/SDL.h>
#include <log/log.h>
#include <cmath>
#include <algorithm>
#include "resize.h"

namespace RetrodevLib {

	//
	// Mitchell-Netravali cubic kernel (B=1/3, C=1/3)
	//
	static float CubicWeight(float x) {
		const float B = 1.0f / 3.0f;
		const float C = 1.0f / 3.0f;
		x = std::fabs(x);
		if (x < 1.0f) {
			return ((12.0f - 9.0f * B - 6.0f * C) * x * x * x + (-18.0f + 12.0f * B + 6.0f * C) * x * x + (6.0f - 2.0f * B)) / 6.0f;
		} else if (x < 2.0f) {
			return ((-B - 6.0f * C) * x * x * x + (6.0f * B + 30.0f * C) * x * x + (-12.0f * B - 48.0f * C) * x + (8.0f * B + 24.0f * C)) / 6.0f;
		}
		return 0.0f;
	}

	//
	// Mark a specific color as transparent (alpha = 0)
	// Useful for sprites/tiles that use a "chroma key" or "magic pink" instead of true alpha
	// tolerance: RGB distance threshold (0 = exact match, higher = more permissive)
	//
	void GFXResize::MarkColorAsTransparent(std::shared_ptr<Image> image, const RgbColor& keyColor, int tolerance) {
		if (!image || !image->GetSurface())
			return;
		int width = image->GetWidth();
		int height = image->GetHeight();
		//
		// Process each pixel
		//
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				RgbColor pixel = image->GetPixelColor(x, y);
				//
				// Skip pixels that are already transparent
				//
				if (pixel.IsTransparent())
					continue;
				//
				// Calculate RGB distance from key color
				//
				int dr = pixel.r - keyColor.r;
				int dg = pixel.g - keyColor.g;
				int db = pixel.b - keyColor.b;
				int distance = dr * dr + dg * dg + db * db;
				int threshold = tolerance * tolerance;
				//
				// If color matches within tolerance, set alpha to 0 (transparent)
				//
				if (distance <= threshold) {
					pixel.a = 0;
					image->SetPixelColor(x, y, pixel);
				}
			}
		}
		image->MarkModified();
	}

	//
	// Nearest-neighbor resize
	// Fast, preserves hard pixel edges (best for pixel art)
	//
	void ResizeNearest(const uint8_t* src, int srcW, int srcH, int srcStride, uint8_t* dst, int dstW, int dstH, int dstStride) {
		//
		// Validate input parameters
		//
		if (!src || !dst || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0)
			return;
		for (int y = 0; y < dstH; y++) {
			int sy = (int)(((float)y + 0.5f) * srcH / dstH);
			sy = Utils::Clamp(sy, srcH - 1);
			for (int x = 0; x < dstW; x++) {
				int sx = (int)(((float)x + 0.5f) * srcW / dstW);
				sx = Utils::Clamp(sx, srcW - 1);
				const uint8_t* sp = src + sy * srcStride + sx * 4;
				uint8_t* dp = dst + y * dstStride + x * 4;
				dp[0] = sp[0];
				dp[1] = sp[1];
				dp[2] = sp[2];
				dp[3] = sp[3];
			}
		}
	}

	//
	// Bilinear resize
	// Smooth interpolation, good balance between quality and performance
	//
	void ResizeBilinear(const uint8_t* src, int srcW, int srcH, int srcStride, uint8_t* dst, int dstW, int dstH, int dstStride) {
		for (int y = 0; y < dstH; y++) {
			float fy = ((float)y + 0.5f) * srcH / dstH - 0.5f;
			int iy = (int)std::floor(fy);
			float dy = fy - iy;
			for (int x = 0; x < dstW; x++) {
				float fx = ((float)x + 0.5f) * srcW / dstW - 0.5f;
				int ix = (int)std::floor(fx);
				float dx = fx - ix;

				const uint8_t* p00 = Utils::SamplePixel(src, ix, iy, srcW, srcH, srcStride);
				const uint8_t* p10 = Utils::SamplePixel(src, ix + 1, iy, srcW, srcH, srcStride);
				const uint8_t* p01 = Utils::SamplePixel(src, ix, iy + 1, srcW, srcH, srcStride);
				const uint8_t* p11 = Utils::SamplePixel(src, ix + 1, iy + 1, srcW, srcH, srcStride);

				uint8_t* dp = dst + y * dstStride + x * 4;
				for (int c = 0; c < 4; c++) {
					float v = p00[c] * (1 - dx) * (1 - dy) + p10[c] * dx * (1 - dy) + p01[c] * (1 - dx) * dy + p11[c] * dx * dy;
					dp[c] = (uint8_t)Utils::Clampf(v, 0.0f, 255.0f);
				}
			}
		}
	}

	//
	// Bicubic resize (Mitchell-Netravali)
	// Highest quality, smooth results with detail preservation
	//
	void ResizeBicubic(const uint8_t* src, int srcW, int srcH, int srcStride, uint8_t* dst, int dstW, int dstH, int dstStride) {
		//
		// Validate input parameters
		//
		if (!src || !dst || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0)
			return;
		for (int y = 0; y < dstH; y++) {
			float fy = ((float)y + 0.5f) * srcH / dstH - 0.5f;
			int iy = (int)std::floor(fy);
			float dy = fy - iy;
			for (int x = 0; x < dstW; x++) {
				float fx = ((float)x + 0.5f) * srcW / dstW - 0.5f;
				int ix = (int)std::floor(fx);
				float dx = fx - ix;

				uint8_t* dp = dst + y * dstStride + x * 4;
				for (int c = 0; c < 4; c++) {
					float val = 0.0f;
					for (int ky = -1; ky <= 2; ky++) {
						float wy = CubicWeight(dy - ky);
						for (int kx = -1; kx <= 2; kx++) {
							float wx = CubicWeight(dx - kx);
							const uint8_t* sp = Utils::SamplePixel(src, ix + kx, iy + ky, srcW, srcH, srcStride);
							val += sp[c] * wx * wy;
						}
					}
					dp[c] = (uint8_t)Utils::Clampf(val, 0.0f, 255.0f);
				}
			}
		}
	}

	//
	// Main resize dispatcher - routes to the appropriate resize function
	//
	void GFXResize::ResizeRGBA(const uint8_t* src, int srcW, int srcH, int srcStride, uint8_t* dst, int dstW, int dstH, int dstStride, ResizeParams::InterpolationMode mode) {
		switch (mode) {
			case ResizeParams::InterpolationMode::NearestNeighbor:
				ResizeNearest(src, srcW, srcH, srcStride, dst, dstW, dstH, dstStride);
				break;
			case ResizeParams::InterpolationMode::Bilinear:
			case ResizeParams::InterpolationMode::Low:
			case ResizeParams::InterpolationMode::HighBilinear:
				ResizeBilinear(src, srcW, srcH, srcStride, dst, dstW, dstH, dstStride);
				break;
			case ResizeParams::InterpolationMode::Bicubic:
			case ResizeParams::InterpolationMode::High:
			case ResizeParams::InterpolationMode::HighBicubic:
				ResizeBicubic(src, srcW, srcH, srcStride, dst, dstW, dstH, dstStride);
				break;
			default:
				ResizeNearest(src, srcW, srcH, srcStride, dst, dstW, dstH, dstStride);
				break;
		}
	}

	//
	// Resizes one Image passed as source into a newly created Image with the given values
	// All the resizing parameters are passed into the ResizeParams struct where the dimensions,
	// resizing mode and interpolation modes are indicated
	//
	std::shared_ptr<Image> GFXResize::GetResizeBitmap(std::shared_ptr<Image> src, const ResizeParams& values) {
		//
		// Create the target image with the specified dimensions
		//
		auto tmp = Image::ImageCreate(values.TargetWidth, values.TargetHeight);
		if (!tmp || !tmp->GetSurface() || !src || !src->GetSurface()) {
			Log::Error(LogChannel::General, "[Resize] Failed to create target image (%dx%d) or source is null.",
				values.TargetWidth, values.TargetHeight);
			return nullptr;
		}
		Log::Info(LogChannel::General, "[Resize] %dx%d -> %dx%d.",
			src->GetWidth(), src->GetHeight(), values.TargetWidth, values.TargetHeight);
		//
		// Resolve the source to RGBA32 before locking for raw pixel access.
		// Paletized (INDEX8) images are blitted through their palette into a
		// temporary RGBA32 surface; the resize algorithms assume 4 bytes per pixel.
		// For the Original-mode SDL_BlitSurface path SDL handles the conversion
		// itself so both branches are covered by using srcRgba as the lock target.
		//
		std::shared_ptr<Image> srcRgba;
		if (src->IsPaletized()) {
			srcRgba = Image::ImageCreate(src->GetWidth(), src->GetHeight());
			if (!srcRgba || !srcRgba->GetSurface())
				return nullptr;
			SDL_BlitSurface(src->GetSurface(), nullptr, srcRgba->GetSurface(), nullptr);
		} else {
			srcRgba = src;
		}
		//
		// Lock both surfaces for raw pixel access; Pitch is taken from the accessor
		//
		auto srcAccess = srcRgba->LockPixels();
		auto tmpAccess = tmp->LockPixels();
		//
		// Fill background with gray (for debugging letterbox/pillarbox areas)
		//
		SDL_FillSurfaceRect(tmp->GetSurface(), NULL, SDL_MapRGBA(SDL_GetPixelFormatDetails(tmp->GetSurface()->format), NULL, 128, 128, 0, 255));
		//
		// Initialize source and destination regions
		//
		int srcX = 0, srcY = 0, srcW = src->GetWidth(), srcH = src->GetHeight();
		int dstX = 0, dstY = 0, dstW = values.TargetWidth, dstH = values.TargetHeight;
		//
		// Adjust regions based on scale mode
		//
		switch (values.ResMode) {
			case ResizeParams::ScaleMode::Smaller:
				//
				// Fit inside: scale so entire source fits in destination, may have letterbox/pillarbox
				// Use the smaller scale factor to ensure the whole image is visible
				//
				{
					float scaleW = (float)values.TargetWidth / (float)src->GetWidth();
					float scaleH = (float)values.TargetHeight / (float)src->GetHeight();
					float scale = scaleW < scaleH ? scaleW : scaleH;
					dstW = (int)(src->GetWidth() * scale);
					dstH = (int)(src->GetHeight() * scale);
					dstX = (values.TargetWidth - dstW) / 2;
					dstY = (values.TargetHeight - dstH) / 2;
				}
				break;
			case ResizeParams::ScaleMode::Largest:
				//
				// Cover: scale so source covers entire destination, may crop source
				// Use the larger scale factor to ensure the destination is fully covered
				//
				{
					float scaleW = (float)values.TargetWidth / (float)src->GetWidth();
					float scaleH = (float)values.TargetHeight / (float)src->GetHeight();
					float scale = scaleW > scaleH ? scaleW : scaleH;
					//
					// Calculate virtual scaled size
					//
					int virtualW = (int)(src->GetWidth() * scale);
					int virtualH = (int)(src->GetHeight() * scale);
					//
					// Destination fills the entire target
					//
					dstX = 0;
					dstY = 0;
					dstW = values.TargetWidth;
					dstH = values.TargetHeight;
					//
					// Calculate which portion of the source to read to get centered crop
					// If virtual size is larger than destination, we need to crop the source
					//
					if (virtualW > values.TargetWidth) {
						//
						// Width overflows: read only the center portion of source width
						//
						srcW = (int)((float)src->GetWidth() * values.TargetWidth / virtualW);
						srcX = (src->GetWidth() - srcW) / 2;
					} else {
						//
						// Width fits: use full source width
						//
						srcX = 0;
						srcW = src->GetWidth();
					}
					if (virtualH > values.TargetHeight) {
						//
						// Height overflows: read only the center portion of source height
						//
						srcH = (int)((float)src->GetHeight() * values.TargetHeight / virtualH);
						srcY = (src->GetHeight() - srcH) / 2;
					} else {
						//
						// Height fits: use full source height
						//
						srcY = 0;
						srcH = src->GetHeight();
					}
				}
				break;
			case ResizeParams::ScaleMode::Fit:
				//
				// Stretch to fill target, ignoring aspect ratio
				//
				break;
			case ResizeParams::ScaleMode::Custom:
				//
				// Use custom source rectangle, stretch to destination
				//
				srcX = values.SourceRect.X;
				srcY = values.SourceRect.Y;
				srcW = values.SourceRect.Width;
				srcH = values.SourceRect.Height;
				break;
			case ResizeParams::ScaleMode::Original:
				//
				// Copy original starting at (0,0), show only what fits
				// If source is larger than destination, crop it
				// If source is smaller, only use source dimensions
				//
				dstX = 0;
				dstY = 0;
				dstW = src->GetWidth() < values.TargetWidth ? src->GetWidth() : values.TargetWidth;
				dstH = src->GetHeight() < values.TargetHeight ? src->GetHeight() : values.TargetHeight;
				srcW = dstW;
				srcH = dstH;
				break;
		}
		//
		// Clamp destination region to ensure we don't write outside the allocated buffer
		// This prevents buffer overruns when centering images (letterbox/pillarbox modes)
		//
		if (dstX < 0) {
			dstW += dstX;
			srcX -= dstX;
			srcW += dstX;
			dstX = 0;
		}
		if (dstY < 0) {
			dstH += dstY;
			srcY -= dstY;
			srcH += dstY;
			dstY = 0;
		}
		if (dstX + dstW > values.TargetWidth) {
			int overflow = (dstX + dstW) - values.TargetWidth;
			dstW -= overflow;
			srcW -= overflow;
		}
		if (dstY + dstH > values.TargetHeight) {
			int overflow = (dstY + dstH) - values.TargetHeight;
			dstH -= overflow;
			srcH -= overflow;
		}
		//
		// Clamp to ensure dimensions are at least 1x1 (required for resize functions)
		//
		if (dstW < 1)
			dstW = 1;
		if (dstH < 1)
			dstH = 1;
		if (srcW < 1)
			srcW = 1;
		if (srcH < 1)
			srcH = 1;
		//
		// Perform the resize operation
		// For Original mode with no scaling, use direct blit for efficiency
		//
		if (values.ResMode == ResizeParams::ScaleMode::Original && srcW == src->GetWidth() && srcH == src->GetHeight() && dstW == srcW && dstH == srcH) {
			//
			// Original mode: direct copy without scaling
			//
			SDL_Rect srcRect = {0, 0, srcW, srcH};
			SDL_Rect dstRect = {dstX, dstY, dstW, dstH};
			SDL_BlitSurface(src->GetSurface(), &srcRect, tmp->GetSurface(), &dstRect);
		} else {
			//
			// Calculate offset for centering in letterbox/pillarbox modes
			//
			int offset = dstY * tmpAccess.Pitch + dstX * 4;
			//
			// Perform the resize with the selected interpolation method
			//
			ResizeRGBA(srcAccess.Pixels + srcY * srcAccess.Pitch + srcX * 4, srcW, srcH, srcAccess.Pitch, tmpAccess.Pixels + offset, dstW, dstH, tmpAccess.Pitch,
					   values.InterpMode);
		}
		//
		// Mark the image as modified for texture updates
		//
		tmp->MarkModified();
		return tmp;
	}
	//
	// Returns the list of available scale modes
	//
	std::vector<std::string> GFXResize::GetScaleModes() {
		return ScaleModeNames;
	}
	//
	// Returns the list of available interpolation modes
	//
	std::vector<std::string> GFXResize::GetInterpolationModes() {
		return InterpolationModeNames;
	}

} // namespace RetrodevLib