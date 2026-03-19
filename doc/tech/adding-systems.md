# Adding Support for a New Target System

Retrodev is designed for system support to be added incrementally. Only the Amstrad CPC has a full converter implementation today; the ZX Spectrum, Commodore 64 and MSX slots exist in the dispatcher but return `nullptr`.

The converter layer is built around four interfaces, all declared in `src/lib/convert/`:

| Interface | Header | Role |
|---|---|---|
| `IBitmapConverter` | `convert.bitmap.h` | Central entry point: conversion, preview, native size, and factory for the three sub-objects below |
| `IPaletteConverter` | `convert.palette.h` | System palette + per-pen colour assignment, locking, and quantization |
| `ITileExtractor` | `convert.tileset.h` | Extracts individual tiles from a converted image |
| `ISpriteExtractor` | `convert.sprites.h` | Extracts individual sprites from a converted image using sprite definitions |

`IBitmapConverter::GetPalette()`, `GetTileExtractor()`, and `GetSpriteExtractor()` each return their respective interface; the extractors are created lazily on first call and receive a back-pointer to the parent bitmap converter so they can query conversion parameters.

The CPC implementation (`src/lib/convert/amstrad.cpc/`) is the reference for all four interfaces.

## Steps

### 1. Add the system identifier

In `src/lib/convert/converters.cpp`, add a new string constant to `SupportedSystems` and append it to `SupportedSystemsNames`:

```cpp
const std::string MySystem = "My System";
```

```cpp
static const std::vector<std::string> SupportedSystemsNames = {
    SupportedSystems::AmstradCPC, ..., SupportedSystems::MySystem
};
```

Add a corresponding `GetSystemId` mapping (used by export scripts to match the `@target` tag):

```cpp
if (displayName == SupportedSystems::MySystem)
    return "mysystem";
```

### 2. Create the system folder and constants header

Create `src/lib/convert/my.system/` and add `my.system.h` inside a namespace `RetrodevLib::ConverterMySystem`. Define nested namespaces for modes, resolutions, and palette types, following the CPC pattern in `src/lib/convert/amstrad.cpc/amstrad.cpc.h`:

```cpp
namespace RetrodevLib::ConverterMySystem {
    namespace MySystemModes   { const std::string Mode0 = "Mode 0"; }
    namespace MySystemResolutions { const std::string Normal = "Normal"; }
    namespace MySystemPaletteTypes { const std::string Hardware = "Hardware"; }
    static const std::vector<std::string> MySystemModesList = { MySystemModes::Mode0 };
    // ...
}
```

### 3. Implement IPaletteConverter

Create `my.system.palette.h` / `my.system.palette.cpp` implementing `IPaletteConverter`. The full interface is declared in `src/lib/convert/convert.palette.h`:

```cpp
int GetSystemMaxColors() override;
RgbColor GetSystemColorByIndex(int index) override;
int GetSystemIndexByColor(const RgbColor& color, const std::string& colorSelectionMode) override;
int PaletteMaxColors() override;
int PaletteMaxColorsByLine(int line) override;
RgbColor PenGetColor(int index) override;
int PenGetIndex(const RgbColor& color) override;
void PenSetColorIndex(int pen, int index) override;
int PenGetColorIndex(int pen) override;
void PenLock(int pen, bool lock) override;
void PenEnable(int pen, bool enable) override;
bool PenGetLock(int pen) override;
bool PenGetEnabled(int pen) override;
bool PenGetUsed(int pen) override;
std::vector<std::string> GetColorSelectionModes() override;
int ColorDistance(const RgbColor& c1, const RgbColor& c2) override;
void PenLockAll(bool lock) override;
void PenEnableAll(bool enable) override;
```

`GetSystemMaxColors` returns the total number of colours the hardware can produce. `PaletteMaxColors` returns how many pens are available in the current mode.

### 4. Implement ITileExtractor

Create `my.system.tileset.h` / `my.system.tileset.cpp` implementing `ITileExtractor`. The full interface is in `src/lib/convert/convert.tileset.h`. The constructor receives an `IBitmapConverter*` back-pointer so the extractor can query the active conversion parameters:

```cpp
bool Extract(std::shared_ptr<Image> sourceImage, const TileExtractionParams* params) override;
int GetTileCount() const override;
std::shared_ptr<Image> GetTile(int index) const override;
```

See `src/lib/convert/amstrad.cpc/cpc.tileset.cpp` for the reference extraction loop.

### 5. Implement ISpriteExtractor

Create `my.system.sprites.h` / `my.system.sprites.cpp` implementing `ISpriteExtractor`. The full interface is in `src/lib/convert/convert.sprites.h`. Like the tile extractor it takes an `IBitmapConverter*` back-pointer:

```cpp
bool Extract(std::shared_ptr<Image> sourceImage, const SpriteExtractionParams* params) override;
int GetSpriteCount() const override;
std::shared_ptr<Image> GetSprite(int index) const override;
const SpriteDefinition* GetSpriteDefinition(int index) const override;
```

### 6. Use the process/image layer inside your converter

The `src/lib/process/image/` classes are the building blocks that `IBitmapConverter::Convert()` should call. The CPC bitmap converter (`cpc.bitmap.cpp`) uses all of them directly. Each class is stateless (all methods are `static`) and operates on `std::shared_ptr<Image>` and the shared `GFXParams` / params structs:

| Class | Header | What it does |
|---|---|---|
| `GFXResize` | `process/image/resize.h` | Resizes an image to the target resolution. Scale mode (`Fit`, `Largest`, `Smaller`, `Custom`, `Original`) and interpolation mode (`NearestNeighbor`, `Bilinear`, `Bicubic`, `High`, …) are controlled via `ResizeParams`. Also provides `MarkColorAsTransparent()` for chroma-key removal. |
| `GFXColor` | `process/image/color.correction.h` | Applies per-channel correction (R/G/B offset), contrast, brightness, saturation, colour bit depth reduction, and palette range clamping (lower/upper OR/AND masks). Call `ContrastInitialize()` once with the params, then `ApplyColorCorrection()` per pixel. |
| `GFXQuantization` | `process/image/quantization.h` | Maps image colours to the system palette. `QuantizationInit(maxColors, maxLines)` prepares internal tables; `ApplyQuantizationAndDither()` quantizes and applies the chosen dithering pattern in one pass; `ApplyColorReduction()` enforces a colour count limit. The palette object (`IPaletteConverter`) is passed in so the quantizer can read/write pen assignments. |
| `GFXDithering` | `process/image/dithering.h` | Dithering matrix library used internally by `GFXQuantization`. Available patterns are named in `DitheringMethods` (e.g. `FloydSteinberg`, `Bayer1..3`, `Ordered1..4`, `ZigZag1..3`, `CheckerboardHeavy/Light/Alternate`, `DiagonalWave`, `SparseVertical/Horizontal`, `CrossPattern`, `ClusterDots`, `GradientHorizontal/Diagonal`). |

A typical `Convert()` implementation follows this order:

1. `GFXResize::GetResizeBitmap()` — scale the source to the native target resolution.
2. `GFXColor::ContrastInitialize()` + `ApplyColorCorrection()` — apply colour correction to each pixel.
3. `GFXQuantization::QuantizationInit()` + `ApplyQuantizationAndDither()` — quantize to the palette.
4. `GFXQuantization::ApplyColorReduction()` — enforce the mode's pen count limit if needed.

The `ColorCorrectionParams` struct (in `process/image/color.correction.params.h`) and `ResizeParams` struct (in `process/image/resize.params.h`) carry the user-facing settings from the UI into each pass. Both are embedded inside `GFXParams` so the converter receives them through the single `params` argument.

### 7. Implement IBitmapConverter

Create `my.system.bitmap.h`

```cpp
std::shared_ptr<IPaletteConverter> GetPalette() override;       // return palette instance
std::shared_ptr<ITileExtractor> GetTileExtractor() override;    // create lazily, pass this
std::shared_ptr<ISpriteExtractor> GetSpriteExtractor() override; // create lazily, pass this
```

The remaining methods cover mode/resolution/palette-type queries, conversion itself, preview generation, native size reporting, and estimated output size — see the full interface declaration in `convert.bitmap.h` and the CPC reference in `cpc.bitmap.cpp`.

### 8. Register the converter

In `Converters::GetBitmapConverter()` in `src/lib/convert/converters.cpp`, add the new system branch before the fallthrough warning:

```cpp
else if (params->SParams.TargetSystem == SupportedSystems::MySystem)
    return std::make_shared<RetrodevLib::ConverterMySystem::MySystemBitmap>();
```

### 9. Add aspect ratio data

Create `src/lib/system/my.system/devices/mysystem.screen.h` and `mysystem.screen.cpp` with a `GetPixelAspectRatio(mode, hScale, vScale)` static method, following the `CPCScreen` pattern in `src/lib/system/amstrad.cpc/devices/cpc.screen.h`. Use the mode string constants from your system header (step 2) rather than bare string literals to avoid a second point of failure if mode names change:

```cpp
void MySystemScreen::GetPixelAspectRatio(const std::string& mode, float& hScale, float& vScale) {
    if (mode == ConverterMySystem::MySystemModes::Mode0) {
        hScale = ...;
        vScale = ...;
    } else {
        hScale = 1.0f;
        vScale = 1.0f;
    }
}
```

Then register the system in `src/lib/convert/converters.cpp`:

- In `GetAspectSystems()`, add `result.push_back(SupportedSystems::MySystem);` after the existing CPC entry.
- In `GetAspectData()`, add an `else if` branch that populates `modes` from `ConverterMySystem::MySystemModesList` and calls `MySystemScreen::GetPixelAspectRatio` with inline normalisation (divide both raw values by their minimum so the smaller axis is 1.0), following the existing AmstradCPC branch.

The map canvas toolbar reads `Converters::GetAspectSystems()` dynamically every frame, so no changes are needed in `document.map.cpp` — the new system will appear in the System combobox automatically.

### 10. Implement screen emulation (preview)

This step is optional but required if you want the converter to produce a display-correct preview image — i.e. pixels stretched to simulate how the system actually looks on a real monitor.

Add `ApplyAspectCorrection` and `ApplyScanlines` to your screen class (alongside `GetPixelAspectRatio` from step 9). Follow the `CPCScreen` pattern in `src/lib/system/amstrad.cpc/devices/cpc.screen.h` and `cpc.screen.cpp`:

```cpp
struct ScalingParams {
    std::string Mode;       // screen mode (uses your mode string constants)
    int         ScaleFactor; // integer multiplier applied on top of aspect correction
    bool        Scanlines;
    float       ScanlineIntensity; // 0.0 = no effect, 1.0 = black lines
    ScalingParams() : Mode(MySystemModes::Mode0), ScaleFactor(2), Scanlines(false), ScanlineIntensity(0.3f) {}
};

static std::shared_ptr<Image> ApplyAspectCorrection(std::shared_ptr<Image> source, const ScalingParams& params);
```

`ApplyAspectCorrection` calls `GetPixelAspectRatio` to obtain the raw `hScale`/`vScale` factors, multiplies both by `ScaleFactor`, allocates a destination `Image` via `Image::ImageCreate(dstWidth, dstHeight)`, then performs nearest-neighbour upscaling with direct RGBA32 pixel copies using `LockPixels()`. After scaling it optionally calls `ApplyScanlines` which darkens every other row by `(1.0 - intensity)`.

The converter's `IBitmapConverter` preview method should call `ApplyAspectCorrection` on the quantized output image and return the result so the UI can display it at the correct pixel shape.

### 11. Write export scripts

Add example export scripts to `sdk/export/my.system/` so users have a starting point. See [Export Scripts](../usage/export-scripts.md) and [SDK](sdk.md) for the required `// @` metadata tags and folder conventions.
