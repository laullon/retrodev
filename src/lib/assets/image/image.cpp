//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#include <assets/image/image.h>
#include <log/log.h>
#include <SDL3_image/SDL_image.h>

namespace RetrodevLib {

	//
	// Constructor - Initialize all members to safe defaults
	//
	Image::Image() {
		m_texture = nullptr;
		m_surface = nullptr;
		m_width = 0;
		m_height = 0;
		m_modified = false;
	}
	//
	// Destructor - Clean up SDL resources
	//
	Image::~Image() {
		if (m_texture) {
			SDL_DestroyTexture(m_texture);
			m_texture = nullptr;
		}
		if (m_surface) {
			SDL_DestroySurface(m_surface);
			m_surface = nullptr;
		}
		m_width = 0;
		m_height = 0;
	}

	//
	// Load an image from a file and return a shared pointer.
	// Supports all formats handled by SDL_image (PNG, JPG, BMP, TGA, etc.).
	// INDEX8 (paletized) surfaces are kept as-is so palette data is preserved.
	// All other formats are converted to RGBA32 for consistent pixel access.
	//
	std::shared_ptr<Image> Image::ImageLoad(const std::string& filePath) {
		//
		// Load image using SDL_image library
		//
		SDL_Surface* surf = IMG_Load(filePath.c_str());
		if (!surf) {
			Log::Error("Image::ImageLoad failed for '%s': %s", filePath.c_str(), SDL_GetError());
			return nullptr;
		}
		//
		// Keep INDEX8 surfaces as-is; ensure a palette is attached so all
		// palette accessors work correctly even if the file had none.
		// Convert everything else to RGBA32.
		//
		if (surf->format == SDL_PIXELFORMAT_INDEX8) {
			if (!SDL_GetSurfacePalette(surf))
				SDL_CreateSurfacePalette(surf);
		} else {
			SDL_Surface* converted = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);
			SDL_DestroySurface(surf);
			if (!converted) {
				Log::Error("Image::ImageLoad SDL_ConvertSurface failed for '%s': %s", filePath.c_str(), SDL_GetError());
				return nullptr;
			}
			surf = converted;
		}
		//
		// Create the Image object and assign the surface
		//
		auto image = std::make_shared<Image>();
		image->m_surface = surf;
		image->m_width = surf->w;
		image->m_height = surf->h;
		image->m_modified = true;
		return image;
	}

	//
	// Create a blank image with the specified dimensions.
	// When paletized is false, creates an RGBA32 surface initialised to transparent black.
	// When paletized is true, creates an INDEX8 (256-colour) surface with all pixels at index 0.
	//
	std::shared_ptr<Image> Image::ImageCreate(int width, int height, bool paletized) {
		//
		// Validate dimensions
		//
		if (width <= 0 || height <= 0) {
			Log::Error("Image::ImageCreate invalid dimensions %dx%d", width, height);
			return nullptr;
		}
		//
		// Create surface in the requested pixel format
		//
		SDL_PixelFormat fmt = paletized ? SDL_PIXELFORMAT_INDEX8 : SDL_PIXELFORMAT_RGBA32;
		SDL_Surface* surf = SDL_CreateSurface(width, height, fmt);
		if (!surf) {
			Log::Error("Image::ImageCreate SDL_CreateSurface(%dx%d) failed: %s", width, height, SDL_GetError());
			return nullptr;
		}
		//
		// For paletized surfaces attach a default 256-entry palette and zero all pixels
		//
		if (paletized) {
			SDL_CreateSurfacePalette(surf);
			SDL_memset(surf->pixels, 0, (size_t)surf->h * (size_t)surf->pitch);
		}
		//
		// Create the Image object and assign the surface
		//
		auto image = std::make_shared<Image>();
		image->m_surface = surf;
		image->m_width = width;
		image->m_height = height;
		image->m_modified = true;
		return image;
	}
	//
	// Save the image surface to a PNG file at the given path
	// Works for both RGBA32 and INDEX8 surfaces
	//
	bool Image::Save(const std::string& filePath) const {
		if (!m_surface) {
			Log::Error("Image::Save called on empty image for '%s'", filePath.c_str());
			return false;
		}
		bool ok = IMG_SavePNG(m_surface, filePath.c_str());
		if (!ok)
			Log::Error("Image::Save IMG_SavePNG failed for '%s': %s", filePath.c_str(), SDL_GetError());
		return ok;
	}
	//
	// Get the SDL surface for direct pixel access
	//
	SDL_Surface* Image::GetSurface() {
		return m_surface;
	}
	//
	// Get or create the SDL texture for rendering
	// Creates texture from surface if needed, updates if modified
	//
	SDL_Texture* Image::GetTexture(SDL_Renderer* renderer) {
		if (!renderer || !m_surface)
			return nullptr;
		// Create texture if it doesn't exist
		if (!m_texture) {
			m_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, m_width, m_height);
			if (!m_texture) {
				Log::Error("Image::GetTexture SDL_CreateTexture(%dx%d) failed: %s", m_width, m_height, SDL_GetError());
				return nullptr;
			}
			m_modified = true;
		}
		// Update texture if image has been modified
		if (m_modified) {
			UpdateTextureRegion(renderer, nullptr);
			m_modified = false;
		}
		return m_texture;
	}
	//
	// Update a specific region of the texture from the surface.
	// For INDEX8 surfaces SDL_BlitSurface is used so the palette lookup is done by SDL.
	// For RGBA32 surfaces pixels are copied directly into the locked texture.
	//
	bool Image::UpdateTextureRegion(SDL_Renderer* renderer, const SDL_Rect* rect) {
		if (!renderer || !m_surface || !m_texture)
			return false;
		SDL_Rect copyRect;
		if (rect) {
			copyRect = *rect;
		} else {
			copyRect.x = 0;
			copyRect.y = 0;
			copyRect.w = m_width;
			copyRect.h = m_height;
		}
		if (m_surface->format == SDL_PIXELFORMAT_INDEX8) {
			//
			// Blit into a temporary RGBA32 surface so SDL applies the palette,
			// then upload from that surface into the locked texture region.
			//
			SDL_Surface* tmp = SDL_CreateSurface(copyRect.w, copyRect.h, SDL_PIXELFORMAT_RGBA32);
			if (!tmp) {
				Log::Error("Image::UpdateTextureRegion SDL_CreateSurface(tmp) failed: %s", SDL_GetError());
				return false;
			}
			SDL_BlitSurface(m_surface, &copyRect, tmp, nullptr);
			void* texPixels;
			int texPitch;
			if (!SDL_LockTexture(m_texture, &copyRect, &texPixels, &texPitch)) {
				Log::Error("Image::UpdateTextureRegion SDL_LockTexture failed: %s", SDL_GetError());
				SDL_DestroySurface(tmp);
				return false;
			}
			SDL_LockSurface(tmp);
			const uint8_t* src = (const uint8_t*)tmp->pixels;
			uint8_t* dst = (uint8_t*)texPixels;
			for (int y = 0; y < copyRect.h; y++)
				SDL_memcpy(dst + y * texPitch, src + y * tmp->pitch, (size_t)copyRect.w * 4);
			SDL_UnlockSurface(tmp);
			SDL_UnlockTexture(m_texture);
			SDL_DestroySurface(tmp);
			return true;
		}
		//
		// RGBA32: lock texture and copy rows directly
		//
		void* pixels;
		int pitch;
		if (!SDL_LockTexture(m_texture, rect, &pixels, &pitch)) {
			Log::Error("Image::UpdateTextureRegion SDL_LockTexture failed: %s", SDL_GetError());
			return false;
		}
		if (!SDL_LockSurface(m_surface)) {
			Log::Error("Image::UpdateTextureRegion SDL_LockSurface failed: %s", SDL_GetError());
			SDL_UnlockTexture(m_texture);
			return false;
		}
		const uint8_t* srcPixels = (const uint8_t*)m_surface->pixels;
		uint8_t* dstPixels = (uint8_t*)pixels;
		int srcPitch = m_surface->pitch;
		for (int y = 0; y < copyRect.h; y++) {
			int srcOffset = (copyRect.y + y) * srcPitch + copyRect.x * 4;
			SDL_memcpy(dstPixels + y * pitch, srcPixels + srcOffset, (size_t)copyRect.w * 4);
		}
		SDL_UnlockSurface(m_surface);
		SDL_UnlockTexture(m_texture);
		return true;
	}
	//
	// Get the color of a pixel at specified coordinates.
	// For paletized surfaces the palette entry for the stored index is returned.
	//
	RgbColor Image::GetPixelColor(int x, int y) const {
		if (!m_surface || x < 0 || y < 0 || x >= m_width || y >= m_height)
			return RgbColor(0, 0, 0);
		SDL_LockSurface(m_surface);
		const uint8_t* pixels = (const uint8_t*)m_surface->pixels;
		int pitch = m_surface->pitch;
		RgbColor result(0, 0, 0);
		if (m_surface->format == SDL_PIXELFORMAT_INDEX8) {
			//
			// INDEX8: one byte per pixel — resolve through the palette
			//
			uint8_t idx = pixels[y * pitch + x];
			SDL_Palette* pal = SDL_GetSurfacePalette(m_surface);
			if (pal && (int)idx < pal->ncolors) {
				const SDL_Color& e = pal->colors[idx];
				result = RgbColor(e.r, e.g, e.b, e.a);
			}
		} else {
			//
			// RGBA32: four bytes per pixel
			//
			int offset = (y * pitch) + (x * 4);
			result = RgbColor(pixels[offset], pixels[offset + 1], pixels[offset + 2], pixels[offset + 3]);
		}
		SDL_UnlockSurface(m_surface);
		return result;
	}
	//
	// Set the color of a pixel at specified coordinates.
	// For paletized surfaces the first palette entry matching the color is written as the index.
	// Use SetPixelIndex to write palette indices directly.
	//
	void Image::SetPixelColor(int x, int y, RgbColor c) {
		if (!m_surface || x < 0 || y < 0 || x >= m_width || y >= m_height)
			return;
		SDL_LockSurface(m_surface);
		uint8_t* pixels = (uint8_t*)m_surface->pixels;
		int pitch = m_surface->pitch;
		if (m_surface->format == SDL_PIXELFORMAT_INDEX8) {
			//
			// Find the first palette entry that matches and write its index
			//
			SDL_Palette* pal = SDL_GetSurfacePalette(m_surface);
			if (pal) {
				for (int i = 0; i < pal->ncolors; i++) {
					const SDL_Color& e = pal->colors[i];
					if (e.r == c.r && e.g == c.g && e.b == c.b && e.a == c.a) {
						pixels[y * pitch + x] = static_cast<uint8_t>(i);
						break;
					}
				}
			}
		} else {
			int offset = y * pitch + x * 4;
			pixels[offset + 0] = c.r;
			pixels[offset + 1] = c.g;
			pixels[offset + 2] = c.b;
			pixels[offset + 3] = c.a;
		}
		SDL_UnlockSurface(m_surface);
		m_modified = true;
	}
	//
	// Lock the surface and return a scoped PixelAccess with Pixels and Pitch
	//
	Image::PixelAccess Image::LockPixels() {
		if (m_surface)
			SDL_LockSurface(m_surface);
		return PixelAccess(m_surface, m_surface ? (uint8_t*)m_surface->pixels : nullptr, m_surface ? m_surface->pitch : 0);
	}
	//
	// Returns true if the surface uses 8-bit paletized (INDEX8) format
	//
	bool Image::IsPaletized() const {
		return m_surface != nullptr && m_surface->format == SDL_PIXELFORMAT_INDEX8;
	}
	//
	// Get the color assigned to the given palette index
	//
	RgbColor Image::GetPaletteColor(int index) const {
		if (!IsPaletized())
			return RgbColor(0, 0, 0);
		SDL_Palette* pal = SDL_GetSurfacePalette(m_surface);
		if (!pal || index < 0 || index >= pal->ncolors)
			return RgbColor(0, 0, 0);
		const SDL_Color& c = pal->colors[index];
		return RgbColor(c.r, c.g, c.b, c.a);
	}
	//
	// Set the color for the given palette index
	//
	void Image::SetPaletteColor(int index, RgbColor color) {
		if (!IsPaletized())
			return;
		SDL_Palette* pal = SDL_GetSurfacePalette(m_surface);
		if (!pal || index < 0 || index >= pal->ncolors)
			return;
		SDL_Color c;
		c.r = color.r;
		c.g = color.g;
		c.b = color.b;
		c.a = color.a;
		SDL_SetPaletteColors(pal, &c, index, 1);
		m_modified = true;
	}
	//
	// Return the number of palette entries (256 for INDEX8, 0 otherwise)
	//
	int Image::GetPaletteSize() const {
		if (!IsPaletized())
			return 0;
		SDL_Palette* pal = SDL_GetSurfacePalette(m_surface);
		return pal ? pal->ncolors : 0;
	}
	//
	// Get the raw palette index stored at the given pixel coordinates
	//
	int Image::GetPixelIndex(int x, int y) const {
		if (!IsPaletized() || x < 0 || y < 0 || x >= m_width || y >= m_height)
			return 0;
		SDL_LockSurface(m_surface);
		uint8_t idx = ((const uint8_t*)m_surface->pixels)[y * m_surface->pitch + x];
		SDL_UnlockSurface(m_surface);
		return static_cast<int>(idx);
	}
	//
	// Set the raw palette index at the given pixel coordinates
	//
	void Image::SetPixelIndex(int x, int y, int index) {
		if (!IsPaletized() || x < 0 || y < 0 || x >= m_width || y >= m_height)
			return;
		SDL_LockSurface(m_surface);
		((uint8_t*)m_surface->pixels)[y * m_surface->pitch + x] = static_cast<uint8_t>(index & 0xFF);
		SDL_UnlockSurface(m_surface);
		m_modified = true;
	}
	//
	// Count how many distinct palette indices are actually referenced by pixels
	//
	int Image::CountPensUsed() const {
		if (!IsPaletized())
			return 0;
		bool used[256] = {};
		SDL_LockSurface(m_surface);
		const uint8_t* pixels = (const uint8_t*)m_surface->pixels;
		int pitch = m_surface->pitch;
		for (int y = 0; y < m_height; y++) {
			const uint8_t* row = pixels + y * pitch;
			for (int x = 0; x < m_width; x++)
				used[row[x]] = true;
		}
		SDL_UnlockSurface(m_surface);
		int count = 0;
		for (int i = 0; i < 256; i++)
			if (used[i])
				count++;
		return count;
	}

} // namespace RetrodevLib
