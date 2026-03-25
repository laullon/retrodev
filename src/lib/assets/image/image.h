// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Image asset -- pixel buffer and colour access.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <assets/image/image.color.h>
#include <SDL3/SDL.h>
#include <string>
#include <memory>

namespace RetrodevLib {

	//
	// Image class for loading and manipulating image data
	// Manages both SDL_Surface for pixel manipulation and SDL_Texture for rendering
	// Supports RGBA32 format for general images and INDEX8 (paletized) format for 256-colour images
	//
	class Image {
	public:
		struct Size {
			int Width, Height;
		};
		Image();
		~Image();

		//
		// Load an image from a file and return a shared pointer
		// Supports all formats handled by SDL_image (PNG, JPG, BMP, TGA, etc.)
		// Returns nullptr on failure
		//
		static std::shared_ptr<Image> ImageLoad(const std::string& filePath);

		//
		// Create a blank image with the specified dimensions and return a shared pointer.
		// When paletized is false (default), creates an RGBA32 surface initialised to transparent black.
		// When paletized is true, creates an INDEX8 (256-colour) surface with all pixels at index 0.
		// Returns nullptr on failure (invalid dimensions or SDL error).
		//
		static std::shared_ptr<Image> ImageCreate(int width, int height, bool paletized = false);

		//
		// Save the image to a PNG file at the given path.
		// Works for both RGBA32 and INDEX8 (paletized) surfaces.
		// Returns true on success, false on failure.
		//
		bool Save(const std::string& filePath) const;

		//
		// Get image width in pixels
		//
		int GetWidth() const { return m_width; }
		//
		// Get image height in pixels
		//
		int GetHeight() const { return m_height; }
		//
		// Get the SDL surface for direct pixel manipulation
		// Returns nullptr if no image is loaded
		//
		SDL_Surface* GetSurface();
		//
		// Get or create the SDL texture for rendering
		// Creates texture from surface if it doesn't exist
		// Updates texture if image has been modified since last call
		// Returns nullptr if no renderer is available or on error
		//
		SDL_Texture* GetTexture(SDL_Renderer* renderer);
		//
		// Update a specific region of the texture
		// More efficient than updating the entire texture when only a small area changed
		// rect: Region to update (nullptr for entire texture)
		// Returns false on error
		//
		bool UpdateTextureRegion(SDL_Renderer* renderer, const SDL_Rect* rect = nullptr);
		//
		// Get the color of a pixel at the specified coordinates
		// Returns black (0,0,0) if coordinates are out of bounds
		//
		RgbColor GetPixelColor(int x, int y) const;
		//
		// Set the color of a pixel at the specified coordinates
		// Marks the image as modified for texture updates
		// Does nothing if coordinates are out of bounds
		//
		void SetPixelColor(int x, int y, RgbColor c);
		//
		// Scoped pixel access -- returned by LockPixels().
		// Locks the surface on construction and unlocks automatically on destruction.
		// Exposes Pixels (raw pointer) and Pitch (row stride in bytes) together so
		// callers never compute stride independently and cannot forget to unlock.
		// Non-copyable; must be used in the same scope as the LockPixels() call.
		//
		struct PixelAccess {
			uint8_t* Pixels;
			int Pitch;
			~PixelAccess() {
				if (m_surface)
					SDL_UnlockSurface(m_surface);
			}
			PixelAccess(const PixelAccess&) = delete;
			PixelAccess& operator=(const PixelAccess&) = delete;

		private:
			friend class Image;
			PixelAccess(SDL_Surface* surface, uint8_t* pixels, int pitch) : Pixels(pixels), Pitch(pitch), m_surface(surface) {}
			SDL_Surface* m_surface;
		};
		//
		// Lock the surface and return a scoped PixelAccess bundling Pixels and Pitch.
		// The surface is automatically unlocked when the returned object is destroyed.
		//
		PixelAccess LockPixels();
		//
		// Mark the image as modified to trigger texture update on next GetTexture call
		//
		void MarkModified() { m_modified = true; }
		//
		// Returns true if the underlying surface is an 8-bit paletized (INDEX8) image
		//
		bool IsPaletized() const;
		//
		// Get the color assigned to the given palette index (0-255).
		// Returns black if the surface is not paletized, has no palette, or index is out of range.
		//
		RgbColor GetPaletteColor(int index) const;
		//
		// Set the color for the given palette index (0-255).
		// Does nothing if the surface is not paletized, has no palette, or index is out of range.
		// Marks the image as modified so the texture is refreshed on the next GetTexture call.
		//
		void SetPaletteColor(int index, RgbColor color);
		//
		// Return the number of palette entries (256 for INDEX8, 0 for non-paletized images).
		//
		int GetPaletteSize() const;
		//
		// Get the raw palette index stored at the given pixel coordinates.
		// Returns 0 if the surface is not paletized or coordinates are out of bounds.
		//
		int GetPixelIndex(int x, int y) const;
		//
		// Set the raw palette index at the given pixel coordinates.
		// Does nothing if the surface is not paletized or coordinates are out of bounds.
		// Marks the image as modified so the texture is refreshed on the next GetTexture call.
		//
		void SetPixelIndex(int x, int y, int index);
		//
		// Count how many distinct palette indices are actually referenced by pixels.
		// Returns 0 for non-paletized images.
		//
		int CountPensUsed() const;

	private:
		//
		// SDL surface for pixel data storage and manipulation
		//
		SDL_Surface* m_surface;
		//
		// SDL texture for GPU-accelerated rendering (lazily created)
		//
		SDL_Texture* m_texture;
		//
		// Image width in pixels
		//
		int m_width;
		//
		// Image height in pixels
		//
		int m_height;
		//
		// Flag indicating if image has been modified since last texture update
		//
		bool m_modified;
	};

}
