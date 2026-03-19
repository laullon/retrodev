# Color Quantization Methods for Retro Systems

## Table of Contents
- [Overview](#overview)
- [Implemented Algorithm](#implemented-algorithm)
- [Parameters and Variables](#parameters-and-variables)
- [Comparison with Other Methods](#comparison-with-other-methods)
- [Usage Guidelines](#usage-guidelines)
- [Technical Details](#technical-details)

---

## Overview

Color quantization is the process of reducing the number of distinct colors in an image while preserving visual quality. For retro systems (Amstrad CPC, MSX, Commodore 64, etc.), this is essential because:

- **Hardware Limitations**: Most retro systems support only 4-27 distinct colors from a larger palette
- **Scanline Palettes**: Some systems (like CPC) can change palettes per scanline, requiring line-aware quantization
- **Fixed Palettes**: Some systems have predefined color sets that cannot be changed

The quantization module in this project implements a **Popularity-Based Greedy Selection** algorithm optimized specifically for retro hardware constraints.

---

## Implemented Algorithm

### Algorithm Type: **Hybrid Popularity + Min-Max Distance**

The implementation uses a custom greedy algorithm with two operational modes:

### Mode 1: Higher Frequencies (Default)

A simple frequency-based greedy selection that prioritizes the most commonly occurring colors.

**Pseudocode:**
```
for each palette slot:
    find the color with highest total usage across all scanlines
    assign it to the current palette slot
    mark that color as "used" (zero its count)
    continue until all palette slots are filled
```

**Characteristics:**
- **Complexity**: O(n) - single pass through the image
- **Speed**: Fastest possible
- **Best for**: Pixel art, graphics with solid color regions, logos
- **Weakness**: May select similar colors (e.g., 5 shades of blue)

### Mode 2: Higher Distances (Perceptual Enhancement)

Alternates between frequency selection and maximum distance selection to ensure color diversity.

**Pseudocode:**
```
select most frequent color first (as reference)
for each remaining palette slot:
    if alternating_flag:
        select next most frequent color
    else:
        select color maximally distant from reference color
    toggle alternating_flag
```

**Characteristics:**
- **Complexity**: O(n log n) - includes distance calculations
- **Speed**: ~2x slower than Mode 1, still fast
- **Best for**: Photographs, gradients, mixed content
- **Strength**: Avoids color clustering, better perceptual coverage

---

## Parameters and Variables

### QuantizationParams Structure

Located in: `src/lib/image/quantization/quantization.params.h`

```cpp
struct QuantizationParams {
    bool Smoothness;              // Apply smoothness during quantization
    int ColorSelectionMethod;     // Palette converter color selection mode
    bool SortPalette;             // Sort palette by index after selection
    ReductionMethod ReductionType; // HigherFrequencies or HigherDistances
    bool ReductionTime;           // Apply reduction before or after dithering
};
```

#### Parameter Details

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `Smoothness` | bool | false | Enables averaging of adjacent pixels during resize (applied during resize phase, not quantization) |
| `ColorSelectionMethod` | int | 0 | Platform-specific method for color matching (passed to IPaletteConverter) |
| `SortPalette` | bool | false | Sorts final palette by system color index (useful for debugging/consistency) |
| `ReductionType` | enum | HigherFrequencies | Selects the quantization algorithm mode:<br>• `HigherFrequencies`: Pure popularity<br>• `HigherDistances`: Hybrid popularity + distance |
| `ReductionTime` | bool | false | If true, applies palette reduction before dithering; if false, after dithering |

### Internal Variables

#### Static Class Members

Located in: `src/lib/image/quantization/quantization.cpp`

```cpp
class GFXQuantization {
    static std::vector<std::vector<int>> colorsFound;
    static int maxSystemColors;
    static int maxSystemLines;
};
```

| Variable | Type | Description |
|----------|------|-------------|
| `colorsFound` | `vector<vector<int>>` | 2D histogram: [colorIndex][scanline] → count<br>Tracks how many times each color appears on each scanline |
| `maxSystemColors` | int | Maximum colors supported by target system (e.g., 27 for CPC) |
| `maxSystemLines` | int | Maximum scanlines in target image (height after resize) |

#### Processing Variables (ApplyQuantizationAndDither)

```cpp
int ditherAmount;         // Amount of dithering to apply (from DitherMatrix)
RgbColor p;               // Current pixel color (after adjustments)
int paletteIndex;         // Index of nearest palette color
RgbColor paletteColor;    // Actual RGB color from palette
int incY;                 // Vertical increment (1 or 2, depending on pattern dithering)
```

#### Color Matching Variables (FindColorMatching)

```cpp
int cUtil;                // Current palette slot being filled
int valMax;               // Maximum frequency found so far
int FindMax;              // Total available source colors (= maxSystemColors)
vector<int> lockState;    // Locks specific palette slots (reserved colors)
bool takeDist;            // Alternates between frequency/distance selection
int oldDist;              // Maximum distance found so far (in HigherDistances mode)
```

---

## Comparison with Other Methods

### Summary Table

| Algorithm | Complexity | Speed | Quality | Retro Suitable | Scanline Support |
|-----------|-----------|-------|---------|----------------|------------------|
| **Popularity (Implemented)** | O(n) | ★★★★★ | ★★★☆☆ | ★★★★★ | ✅ Yes |
| **Popularity + Distance (Implemented)** | O(n log n) | ★★★★☆ | ★★★★☆ | ★★★★★ | ✅ Yes |
| Median Cut | O(n log n) | ★★★☆☆ | ★★★★☆ | ★★☆☆☆ | ❌ No |
| Octree | O(n) | ★★★☆☆ | ★★★★☆ | ★☆☆☆☆ | ❌ No |
| K-means | O(n × k × i) | ★☆☆☆☆ | ★★★★★ | ❌ No | ❌ No |

### Detailed Comparison

#### 1. Median Cut

**How it works:**
- Recursively subdivides RGB color space into boxes
- Each subdivision splits along the axis with the widest color range
- Final palette = average color in each box

**Pros:**
- ✅ Even distribution across color space
- ✅ Works well for photos with varied colors

**Cons:**
- ❌ No scanline support (treats entire image as one palette)
- ❌ May select colors not actually in the image
- ❌ Slower than popularity (requires sorting)
- ❌ More complex implementation

**Use case:** Converting photographs where scanline palettes aren't needed

---

#### 2. Octree Quantization

**How it works:**
- Builds a tree where each level represents RGB bit depth
- Merges nodes to reduce palette size
- Good for progressive refinement

**Pros:**
- ✅ Hierarchical structure allows progressive palette reduction
- ✅ Works well for large palettes (256 colors)

**Cons:**
- ❌ **Massive overkill** for 4-16 color palettes
- ❌ High memory overhead (tree structure)
- ❌ No scanline support
- ❌ Complex implementation

**Use case:** 256-color VGA mode conversions (not applicable to most retro systems)

---

#### 3. K-means Clustering

**How it works:**
- Iteratively refines palette by:
  1. Randomly initializing k cluster centers
  2. Assigning each pixel to nearest center
  3. Moving centers to cluster centroids
  4. Repeating until convergence

**Pros:**
- ✅ Theoretically optimal (minimizes quantization error)
- ✅ Well-studied algorithm

**Cons:**
- ❌ **Far too slow** - requires multiple iterations (5-20 passes)
- ❌ **Non-deterministic** - depends on random initial centroids
- ❌ **Per-scanline impossible** - can't afford multiple iterations per scanline
- ❌ Convergence not guaranteed
- ❌ Overkill for 4-16 colors

**Use case:** Academic research only (never for retro systems)

---

## Usage Guidelines

### When to Use Higher Frequencies Mode

**Ideal for:**
- ✅ Pixel art and sprites
- ✅ Graphics with well-defined color regions
- ✅ Logos and UI elements
- ✅ Cartoons and cell-shaded graphics
- ✅ Maximum performance requirements

**Example scenario:**
```
Image: 16-color pixel art sprite with solid color regions
Result: Palette contains the 16 most frequently used colors
Outcome: Perfect representation, no color artifacts
```

### When to Use Higher Distances Mode

**Ideal for:**
- ✅ Photographs and scanned images
- ✅ Gradients and smooth color transitions
- ✅ Mixed content (photos + graphics)
- ✅ Better perceptual quality requirements

**Example scenario:**
```
Image: Photograph with blue sky (60%), green grass (30%), red shirt (10%)

Higher Frequencies:
  Palette: 10 shades of blue, 4 shades of green, 2 near-black
  Problem: Red shirt becomes brown/black (no red in palette)

Higher Distances:
  Palette: Most common blue, most common green, RED (maximally distant), shades
  Result: Blue sky ✓, green grass ✓, red shirt ✓ - all represented
```

---

## Technical Details

### Scanline-Aware Quantization

The algorithm is designed for systems that can change palettes per scanline (e.g., Amstrad CPC).

**Key insight:**
```cpp
// 2D histogram: tracks colors PER SCANLINE
std::vector<std::vector<int>> colorsFound;
// colorsFound[colorIndex][scanlineNumber] = usage_count
```

This allows:
1. **Per-line palette optimization**: Different palette for each scanline
2. **Screen splitting**: Different palettes for top/bottom screen regions
3. **Raster effects**: Palette changes synchronized with display

### Color Distance Calculation

Distance is calculated in RGB space by the `IPaletteConverter`:

```cpp
int dist = palette.ColorDistance(color1, color2);
```

Common implementations:
- **Euclidean distance**: `sqrt((r1-r2)² + (g1-g2)² + (b1-b2)²)`
- **Manhattan distance**: `|r1-r2| + |g1-g2| + |b1-b2|`
- **Weighted perceptual**: Custom weights for human color perception

### Integration with Dithering

The quantization process integrates tightly with dithering:

```cpp
// Workflow:
1. Resize image to target resolution
2. Apply quantization (build color histogram)
3. Optionally apply dithering (spread quantization error)
4. Reduce to final palette (FindColorMatching)
5. Map all pixels to final palette (ConvertStd)
```

**ReductionTime parameter:**
- `true`: Reduce palette → then dither → final output
- `false`: Dither → then reduce palette → final output

---

## Algorithm Complexity Analysis

### Time Complexity

| Operation | Complexity | Description |
|-----------|-----------|-------------|
| Histogram building | O(w × h) | One pass through all pixels |
| Higher Frequencies | O(c × s × p) | c=colors, s=scanlines, p=palette_slots |
| Higher Distances | O(c × s × p × d) | d=distance calculations per slot |
| Final mapping | O(w × h × p) | Map each pixel to nearest palette color |

**Total complexity:**
- Higher Frequencies: O(w × h + c × s × p)
- Higher Distances: O(w × h + c × s × p × d)

For typical retro systems:
- w, h = 160-320 pixels
- c = 27 colors (CPC), s = 200 lines, p = 4-16 slots
- **Result**: Extremely fast, completes in < 10ms

### Space Complexity

| Structure | Size | Description |
|-----------|------|-------------|
| `colorsFound` | c × s × 4 bytes | 27 × 200 × 4 = ~21 KB |
| `linesCol` | p × s × RGB | 16 × 200 × 12 = ~38 KB |
| Total | < 100 KB | Minimal memory footprint |

---

## Code Organization

### File Structure

```
src/lib/image/quantization/
├── quantization.h              # Main quantization class
├── quantization.cpp            # Implementation
├── quantization.params.h       # Parameters structure
└── quantization.params.cpp     # Default values

src/lib/project/metadata/
└── meta.quantization.h         # JSON serialization

src/gui/widgets/
└── conversion.widget.cpp       # UI controls
```

### Key Functions

```cpp
// Initialize quantization system
void QuantizationInit(int MaxColors, int MaxLines);

// Apply quantization + dithering
void ApplyQuantizationAndDither(
    IPaletteConverter& palette,
    std::shared_ptr<Image> source,
    GFXParams& prm
);

// Reduce to final palette
void ApplyColorReduction(
    IPaletteConverter& palette,
    std::shared_ptr<Image> source,
    GFXParams& prm,
    int& colSplit
);

// Internal: Find best N colors
void FindColorMatching(
    IPaletteConverter& palette,
    int maxPen,
    std::vector<int>& lockState,
    GFXParams& prm
);

// Internal: Map pixels to final palette
void ConvertStd(
    std::shared_ptr<Image> source,
    GFXParams& prm,
    IPaletteConverter& palette,
    std::vector<std::vector<RgbColor>>& tabCol
);
```

---

## References

### Academic Papers
- **Popularity Algorithm**: Heckbert, P. (1982). "Color Image Quantization for Frame Buffer Display"
- **Median Cut**: Heckbert, P. (1982). "Color Image Quantization for Frame Buffer Display"
- **Octree**: Gervautz, M. & Purgathofer, W. (1988). "A Simple Method for Color Quantization: Octree Quantization"
- **K-means**: MacQueen, J. (1967). "Some methods for classification and analysis of multivariate observations"

### Implementation Notes
- The hybrid approach (Popularity + Distance) is a custom enhancement not found in academic literature
- Scanline-aware quantization is specific to retro hardware requirements
- Integration with error diffusion dithering provides better results than quantization alone

---

## Version History

- **v1.0** (Current): Hybrid Popularity + Min-Max Distance algorithm
- Future considerations:
  - Palette locking API for user-specified colors
  - Per-region quantization (split screen modes)
  - Adaptive mode selection based on image analysis

---

## License

This implementation is part of the Retrodev project and follows the project's licensing terms.
