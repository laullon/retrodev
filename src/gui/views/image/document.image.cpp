// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#include <app/app.h>
#include <retrodev.lib.h>
#include "document.image.h"
#include <widgets/image.palette.widget.h>
#include <widgets/painting.widget.h>
#include <vector>
#include <cmath>

namespace RetrodevGui {

	DocumentImage::DocumentImage(const std::string& name, const std::string& filepath) : DocumentView(name, filepath) {
		m_image = RetrodevLib::Image::ImageLoad(filepath);
		if (m_image) {
			m_zoomState.textureSize = ImVec2((float)m_image->GetWidth(), (float)m_image->GetHeight());
			m_zoomState.maintainAspectRatio = true;
			m_zoomState.showInfo = true;
			m_zoomState.name = name;
			m_zoomState.showPixelGrid = true;
		}
	}

	DocumentImage::~DocumentImage() {}

	bool DocumentImage::Save() {
		if (!m_image)
			return false;
		bool ok = m_image->Save(m_filePath);
		if (ok)
			SetModified(false);
		return ok;
	}

	void DocumentImage::CaptureSnapshot() {
		if (!m_image)
			return;
		int w = m_image->GetWidth();
		int h = m_image->GetHeight();
		std::vector<int> snapshot(w * h);
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				if (m_image->IsPaletized()) {
					snapshot[y * w + x] = m_image->GetPixelIndex(x, y);
				} else {
					RetrodevLib::RgbColor c = m_image->GetPixelColor(x, y);
					snapshot[y * w + x] = (c.r << 16) | (c.g << 8) | c.b;
				}
			}
		}
		//
		// Trim undo stack to max steps before pushing
		//
		if ((int)m_undoStack.size() >= k_maxUndoSteps)
			m_undoStack.erase(m_undoStack.begin());
		m_undoStack.push_back(std::move(snapshot));
		//
		// Any new action clears the redo stack
		//
		m_redoStack.clear();
	}

	void DocumentImage::ApplySnapshot(const std::vector<int>& snapshot) {
		if (!m_image)
			return;
		int w = m_image->GetWidth();
		int h = m_image->GetHeight();
		if ((int)snapshot.size() != w * h)
			return;
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				int packed = snapshot[y * w + x];
				if (m_image->IsPaletized()) {
					m_image->SetPixelIndex(x, y, packed);
				} else {
					RetrodevLib::RgbColor c;
					c.r = (packed >> 16) & 0xFF;
					c.g = (packed >> 8) & 0xFF;
					c.b = packed & 0xFF;
					m_image->SetPixelColor(x, y, c);
				}
			}
		}
		m_image->GetTexture(Application::GetRenderer());
	}

	void DocumentImage::Perform() {
		if (!m_image) {
			AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed to load image: %s", m_filePath.c_str());
			return;
		}
		// Get the texture to be painted
		SDL_Texture* m_texture = m_image->GetTexture(Application::GetRenderer());
		// Check texture was created successfully
		if (!m_texture) {
			AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed to create texture: %s", m_filePath.c_str());
			return;
		}
		// Initialize splitter sizes
		float fontSize = ImGui::GetFontSize();
		ImVec2 avail = ImGui::GetContentRegionAvail();
		bool isPaletized = m_image->IsPaletized();
		if (!m_sizesInitialized) {
			m_hSizeRight = fontSize * 15;
			m_hSizeLeft = avail.x - (m_hSizeRight + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.x);
			m_vSizeTop = avail.y * 0.7f;
			m_vSizeBottom = avail.y - (m_vSizeTop + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.y);
			m_sizesInitialized = true;
		}
		// Recalculate horizontal sizes when window resizes
		m_hSizeLeft = avail.x - (m_hSizeRight + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.x);
		// Minimum sizes for splitter constraints
		float hMinLeft = fontSize * 20;
		float hMinRight = fontSize * 10;
		float vMinTop = fontSize * 8;
		float vMinBottom = fontSize * 5;
		// Horizontal splitter: left area | right tooling panel (paletized only)
		if (isPaletized)
			ImGui::DrawSplitter(false, Application::splitterThickness, &m_hSizeLeft, &m_hSizeRight, hMinLeft, hMinRight);
		// Left area width: full width for RGBA, split width for paletized
		float leftPanelW = isPaletized ? m_hSizeLeft : avail.x;
		// Left area (image viewer + operations)
		if (ImGui::BeginChild("ImageLeftPanel", ImVec2(leftPanelW, 0), false)) {
			ImVec2 leftAvail = ImGui::GetContentRegionAvail();
			m_vSizeBottom = leftAvail.y - (m_vSizeTop + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.y);
			// Vertical splitter: top (image) | bottom palette panel (paletized only)
			if (isPaletized)
				ImGui::DrawSplitter(true, Application::splitterThickness, &m_vSizeTop, &m_vSizeBottom, vMinTop, vMinBottom);
			// Image viewer height: full height for RGBA, split height for paletized
			float imageViewH = isPaletized ? m_vSizeTop : -1.0f;
			// Top: zoomable image viewer
			//
			if (ImGui::BeginChild("ImageViewPanel", ImVec2(-1, imageViewH), true)) {
				ImVec2 displaySize = ImGui::GetContentRegionAvail();
				//
				// Set zoom mode based on active tool
				//
				PaintingTool tool = PaintingWidget::GetSelectedTool();
				//
				// Clear stamp buffer when the user switches away from RegionSelect
				//
				if (tool != PaintingTool::RegionSelect && m_hasStamp) {
					m_hasStamp = false;
					m_stampBuffer.clear();
					m_zoomState.selectionActive = false;
				}
				if (tool == PaintingTool::None)
					m_zoomState.mode = ImGui::ZoomableMode::Panning;
				else if (tool == PaintingTool::RegionSelect && !m_hasStamp)
					m_zoomState.mode = ImGui::ZoomableMode::Selection;
				else
					m_zoomState.mode = ImGui::ZoomableMode::Picking;
				ImGui::Zoomable(m_texture, displaySize, &m_zoomState);
				//
				// Undo / Redo keyboard shortcuts (Ctrl+Z / Ctrl+Y)
				//
				{
					auto& io = ImGui::GetIO();
					bool ctrl = io.KeyCtrl;
					//
					// Helper to snapshot the current image pixels without affecting the redo stack
					//
					auto SnapshotCurrent = [&]() -> std::vector<int> {
						int w = m_image->GetWidth();
						int h = m_image->GetHeight();
						std::vector<int> snap(w * h);
						for (int y = 0; y < h; y++) {
							for (int x = 0; x < w; x++) {
								if (m_image->IsPaletized()) {
									snap[y * w + x] = m_image->GetPixelIndex(x, y);
								} else {
									RetrodevLib::RgbColor c = m_image->GetPixelColor(x, y);
									snap[y * w + x] = (c.r << 16) | (c.g << 8) | c.b;
								}
							}
						}
						return snap;
					};
					if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z, false) && !m_undoStack.empty()) {
						//
						// Save current state to redo, restore top of undo stack
						//
						m_redoStack.push_back(SnapshotCurrent());
						ApplySnapshot(m_undoStack.back());
						m_undoStack.pop_back();
						SetModified(true);
					} else if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y, false) && !m_redoStack.empty()) {
						//
						// Save current state to undo, restore top of redo stack
						//
						m_undoStack.push_back(SnapshotCurrent());
						ApplySnapshot(m_redoStack.back());
						m_redoStack.pop_back();
						SetModified(true);
					}
				}
				//
				// Resolve pen index from active slot
				//
				int penIndex = (PaintingWidget::GetActiveSlot() == PaintSlot::Erase) ? PaintingWidget::GetEraseColor() : m_activePen;
				bool hasPos = !std::isnan(m_zoomState.mousePosition.x) && !std::isnan(m_zoomState.mousePosition.y);
				int mx = hasPos ? (int)m_zoomState.mousePosition.x : 0;
				int my = hasPos ? (int)m_zoomState.mousePosition.y : 0;
				//
				// Helper: paint one pixel safely
				//
				auto PaintPixel = [&](int px, int py) {
					if (m_image->IsPaletized())
						m_image->SetPixelIndex(px, py, penIndex);
					else
						m_image->SetPixelColor(px, py, m_image->GetPaletteColor(penIndex));
				};
				//
				// Helper: paint one brush stamp at (cx, cy)
				//
				auto PaintBrush = [&](int cx, int cy) {
					int brushSize = PaintingWidget::GetBrushSize();
					int half = brushSize / 2;
					BrushShape shape = PaintingWidget::GetBrushShape();
					for (int dy = -half; dy < brushSize - half; dy++) {
						for (int dx = -half; dx < brushSize - half; dx++) {
							if (shape == BrushShape::Circle) {
								float fx = dx + 0.5f;
								float fy = dy + 0.5f;
								float r = brushSize * 0.5f;
								if (fx * fx + fy * fy > r * r)
									continue;
							} else if (shape == BrushShape::Diamond) {
								if (std::abs(dx) + std::abs(dy) >= half + 1)
									continue;
							}
							PaintPixel(cx + dx, cy + dy);
						}
					}
				};
				//
				// Helper: draw a line between two points using Bresenham
				//
				auto DrawLine = [&](int x0, int y0, int x1, int y1) {
					int dx = std::abs(x1 - x0);
					int dy = std::abs(y1 - y0);
					int sx = x0 < x1 ? 1 : -1;
					int sy = y0 < y1 ? 1 : -1;
					int err = dx - dy;
					while (true) {
						PaintBrush(x0, y0);
						if (x0 == x1 && y0 == y1)
							break;
						int e2 = 2 * err;
						if (e2 > -dy) {
							err -= dy;
							x0 += sx;
						}
						if (e2 < dx) {
							err += dx;
							y0 += sy;
						}
					}
				};
				//
				// Helper: draw a rectangle outline
				//
				auto DrawRectOutline = [&](int x0, int y0, int x1, int y1) {
					int minX = std::min(x0, x1);
					int maxX = std::max(x0, x1);
					int minY = std::min(y0, y1);
					int maxY = std::max(y0, y1);
					for (int x = minX; x <= maxX; x++) {
						PaintPixel(x, minY);
						PaintPixel(x, maxY);
					}
					for (int y = minY + 1; y < maxY; y++) {
						PaintPixel(minX, y);
						PaintPixel(maxX, y);
					}
				};
				//
				// Helper: draw a filled rectangle
				//
				auto DrawRectFill = [&](int x0, int y0, int x1, int y1) {
					int minX = std::min(x0, x1);
					int maxX = std::max(x0, x1);
					int minY = std::min(y0, y1);
					int maxY = std::max(y0, y1);
					for (int y = minY; y <= maxY; y++)
						for (int x = minX; x <= maxX; x++)
							PaintPixel(x, y);
				};
				//
				// Helper: draw an ellipse outline using midpoint algorithm
				//
				auto DrawEllipseOutline = [&](int cx, int cy, int rx, int ry) {
					if (rx < 0)
						rx = -rx;
					if (ry < 0)
						ry = -ry;
					int x = 0;
					int y = ry;
					long long rx2 = (long long)rx * rx;
					long long ry2 = (long long)ry * ry;
					long long d1 = ry2 - rx2 * ry + rx2 / 4;
					long long dx = 2 * ry2 * x;
					long long dy = 2 * rx2 * y;
					while (dx < dy) {
						PaintPixel(cx + x, cy + y);
						PaintPixel(cx - x, cy + y);
						PaintPixel(cx + x, cy - y);
						PaintPixel(cx - x, cy - y);
						if (d1 < 0) {
							x++;
							dx += 2 * ry2;
							d1 += dx + ry2;
						} else {
							x++;
							y--;
							dx += 2 * ry2;
							dy -= 2 * rx2;
							d1 += dx - dy + ry2;
						}
					}
					long long d2 = ry2 * ((long long)(x)*x + x) + rx2 * ((long long)(y - 1) * (y - 1)) - (long long)rx2 * ry2;
					while (y >= 0) {
						PaintPixel(cx + x, cy + y);
						PaintPixel(cx - x, cy + y);
						PaintPixel(cx + x, cy - y);
						PaintPixel(cx - x, cy - y);
						if (d2 > 0) {
							y--;
							dy -= 2 * rx2;
							d2 += rx2 - dy;
						} else {
							y--;
							x++;
							dx += 2 * ry2;
							dy -= 2 * rx2;
							d2 += dx - dy + rx2;
						}
					}
				};
				//
				// Helper: draw a filled ellipse
				//
				auto DrawEllipseFill = [&](int cx, int cy, int rx, int ry) {
					if (rx < 0)
						rx = -rx;
					if (ry < 0)
						ry = -ry;
					long long rx2 = (long long)rx * rx;
					long long ry2 = (long long)ry * ry;
					for (int y = -ry; y <= ry; y++) {
						long long xMax2 = rx2 * (ry2 - (long long)y * y) / (ry2 > 0 ? ry2 : 1);
						int xMax = (int)std::sqrt((double)xMax2);
						for (int x = -xMax; x <= xMax; x++)
							PaintPixel(cx + x, cy + y);
					}
				};
				//
				// Helper: flood fill with tolerance
				//
				auto FloodFill = [&](int sx, int sy) {
					int w = m_image->GetWidth();
					int h = m_image->GetHeight();
					if (sx < 0 || sy < 0 || sx >= w || sy >= h)
						return;
					int tolerance = PaintingWidget::GetFillTolerance();
					int targetIndex = m_image->IsPaletized() ? m_image->GetPixelIndex(sx, sy) : 0;
					RetrodevLib::RgbColor targetColor = m_image->GetPixelColor(sx, sy);
					//
					// Visited bitmap
					//
					std::vector<bool> visited(w * h, false);
					std::vector<std::pair<int, int>> stack;
					stack.reserve(w * h / 4);
					stack.push_back({sx, sy});
					visited[sy * w + sx] = true;
					while (!stack.empty()) {
						auto [fx, fy] = stack.back();
						stack.pop_back();
						//
						// Check if this pixel matches the target within tolerance
						//
						bool matches = false;
						if (m_image->IsPaletized()) {
							if (tolerance == 0) {
								matches = (m_image->GetPixelIndex(fx, fy) == targetIndex);
							} else {
								RetrodevLib::RgbColor pc = m_image->GetPixelColor(fx, fy);
								int dr = std::abs((int)pc.r - (int)targetColor.r);
								int dg = std::abs((int)pc.g - (int)targetColor.g);
								int db = std::abs((int)pc.b - (int)targetColor.b);
								matches = (dr <= tolerance && dg <= tolerance && db <= tolerance);
							}
						} else {
							RetrodevLib::RgbColor pc = m_image->GetPixelColor(fx, fy);
							int dr = std::abs((int)pc.r - (int)targetColor.r);
							int dg = std::abs((int)pc.g - (int)targetColor.g);
							int db = std::abs((int)pc.b - (int)targetColor.b);
							matches = (dr <= tolerance && dg <= tolerance && db <= tolerance);
						}
						if (!matches)
							continue;
						PaintPixel(fx, fy);
						//
						// Push 4-connected neighbours
						//
						static const int nx[] = {1, -1, 0, 0};
						static const int ny[] = {0, 0, 1, -1};
						for (int d = 0; d < 4; d++) {
							int nx2 = fx + nx[d];
							int ny2 = fy + ny[d];
							if (nx2 >= 0 && ny2 >= 0 && nx2 < w && ny2 < h && !visited[ny2 * w + nx2]) {
								visited[ny2 * w + nx2] = true;
								stack.push_back({nx2, ny2});
							}
						}
					}
				};
				//
				// Pencil / Eraser: paint continuously while held
				//
				if ((tool == PaintingTool::Pencil || tool == PaintingTool::Eraser) && hasPos && (m_zoomState.clicked || m_zoomState.pressed)) {
					if (m_zoomState.clicked)
						CaptureSnapshot();
					PaintBrush(mx, my);
					SetModified(true);
					m_image->GetTexture(Application::GetRenderer());
				}
				//
				// Fill: single click flood fill
				//
				if (tool == PaintingTool::Fill && hasPos && m_zoomState.clicked) {
					CaptureSnapshot();
					FloodFill(mx, my);
					SetModified(true);
					m_image->GetTexture(Application::GetRenderer());
				}
				//
				// Shape tools (Line, Rect, Ellipse): drag to define, commit on release
				// On press: record start point
				// While dragging: show live preview overlay
				// On release: commit to image
				//
				bool isShapeTool = (tool == PaintingTool::Line || tool == PaintingTool::RectOutline || tool == PaintingTool::RectFill || tool == PaintingTool::EllipseOutline ||
									tool == PaintingTool::EllipseFill);
				if (isShapeTool && hasPos) {
					if (m_zoomState.clicked) {
						//
						// Start drag
						//
						m_isDragging = true;
						m_dragStart = m_zoomState.mousePosition;
						m_dragPenIndex = penIndex;
					}
					if (m_isDragging && m_zoomState.pressed) {
						//
						// Draw live XOR-style preview on the foreground draw list so it renders
						// on top of all child windows (GetWindowDrawList would be obscured by the
						// Zoomable inner child window that owns the image surface)
						//
						ImDrawList* dl = ImGui::GetForegroundDrawList();
						const ImVec2 sdp = m_zoomState.screenDisplayPos;
						const ImVec2 sds = m_zoomState.screenDisplaySize;
						const float uxs = m_zoomState.uvSpanX;
						const float uys = m_zoomState.uvSpanY;
						const float pan0 = m_zoomState.panOffset.x;
						const float pan1 = m_zoomState.panOffset.y;
						const float iw = (float)m_image->GetWidth();
						const float ih = (float)m_image->GetHeight();
						auto ImageToScreen = [&](float ix, float iy) -> ImVec2 {
							float normX = ix / iw;
							float normY = iy / ih;
							float sx = sdp.x + (normX - pan0) * sds.x / uxs;
							float sy = sdp.y + (normY - pan1) * sds.y / uys;
							return ImVec2(sx, sy);
						};
						ImVec2 s = ImageToScreen(m_dragStart.x, m_dragStart.y);
						ImVec2 e = ImageToScreen((float)mx, (float)my);
						//
						// XOR-style: draw twice — white line on top of a slightly thicker black one
						//
						if (tool == PaintingTool::Line) {
							dl->AddLine(s, e, IM_COL32(0, 0, 0, 200), 3.0f);
							dl->AddLine(s, e, IM_COL32(255, 255, 255, 220), 1.0f);
						} else if (tool == PaintingTool::RectOutline || tool == PaintingTool::RectFill) {
							ImVec2 mn = ImVec2(std::min(s.x, e.x), std::min(s.y, e.y));
							ImVec2 mx2 = ImVec2(std::max(s.x, e.x), std::max(s.y, e.y));
							dl->AddRect(ImVec2(mn.x - 1, mn.y - 1), ImVec2(mx2.x + 1, mx2.y + 1), IM_COL32(0, 0, 0, 200), 0.0f, 0, 3.0f);
							dl->AddRect(mn, mx2, IM_COL32(255, 255, 255, 220), 0.0f, 0, 1.0f);
						} else if (tool == PaintingTool::EllipseOutline || tool == PaintingTool::EllipseFill) {
							ImVec2 center = ImVec2((s.x + e.x) * 0.5f, (s.y + e.y) * 0.5f);
							ImVec2 radii = ImVec2(std::abs(e.x - s.x) * 0.5f, std::abs(e.y - s.y) * 0.5f);
							dl->AddEllipse(center, ImVec2(radii.x + 1, radii.y + 1), IM_COL32(0, 0, 0, 200), 0.0f, 0, 3.0f);
							dl->AddEllipse(center, radii, IM_COL32(255, 255, 255, 220), 0.0f, 0, 1.0f);
						}
					}
					if (m_isDragging && !m_zoomState.pressed && !m_zoomState.clicked) {
						//
						// Mouse released: commit shape to image
						//
						CaptureSnapshot();
						m_isDragging = false;
						int x0 = (int)m_dragStart.x;
						int y0 = (int)m_dragStart.y;
						int x1 = mx;
						int y1 = my;
						penIndex = m_dragPenIndex;
						if (tool == PaintingTool::Line) {
							DrawLine(x0, y0, x1, y1);
						} else if (tool == PaintingTool::RectOutline) {
							DrawRectOutline(x0, y0, x1, y1);
						} else if (tool == PaintingTool::RectFill) {
							DrawRectFill(x0, y0, x1, y1);
						} else if (tool == PaintingTool::EllipseOutline) {
							int cx = (x0 + x1) / 2;
							int cy = (y0 + y1) / 2;
							DrawEllipseOutline(cx, cy, std::abs(x1 - x0) / 2, std::abs(y1 - y0) / 2);
						} else if (tool == PaintingTool::EllipseFill) {
							int cx = (x0 + x1) / 2;
							int cy = (y0 + y1) / 2;
							DrawEllipseFill(cx, cy, std::abs(x1 - x0) / 2, std::abs(y1 - y0) / 2);
						}
						SetModified(true);
						m_image->GetTexture(Application::GetRenderer());
					}
				} else if (!isShapeTool) {
					m_isDragging = false;
				}
				//
				// RegionSelect: capture stamp when selection is completed, paste on click
				//
				if (tool == PaintingTool::RegionSelect) {
					//
					// Phase 1: selection just completed — copy pixels into stamp buffer
					//
					if (m_zoomState.selectionActive && !m_hasStamp && m_zoomState.selection.z > 0.0f && m_zoomState.selection.w > 0.0f) {
						int sx = (int)m_zoomState.selection.x;
						int sy = (int)m_zoomState.selection.y;
						int sw = (int)m_zoomState.selection.z;
						int sh = (int)m_zoomState.selection.w;
						int imgW = m_image->GetWidth();
						int imgH = m_image->GetHeight();
						sx = std::max(0, std::min(sx, imgW - 1));
						sy = std::max(0, std::min(sy, imgH - 1));
						sw = std::max(1, std::min(sw, imgW - sx));
						sh = std::max(1, std::min(sh, imgH - sy));
						m_stampBuffer.resize(sw * sh);
						for (int row = 0; row < sh; row++) {
							for (int col = 0; col < sw; col++) {
								if (m_image->IsPaletized())
									m_stampBuffer[row * sw + col] = m_image->GetPixelIndex(sx + col, sy + row);
								else {
									RetrodevLib::RgbColor c = m_image->GetPixelColor(sx + col, sy + row);
									m_stampBuffer[row * sw + col] = (c.r << 16) | (c.g << 8) | c.b;
								}
							}
						}
						m_stampRect = ImVec4((float)sx, (float)sy, (float)sw, (float)sh);
						m_hasStamp = true;
					}
					//
					// Phase 2: stamp is held — show preview overlay and paste on click
					//
					if (m_hasStamp) {
						int sw = (int)m_stampRect.z;
						int sh = (int)m_stampRect.w;
						const ImVec2 sdp = m_zoomState.screenDisplayPos;
						const ImVec2 sds = m_zoomState.screenDisplaySize;
						const float uxs = m_zoomState.uvSpanX;
						const float uys = m_zoomState.uvSpanY;
						const float pan0 = m_zoomState.panOffset.x;
						const float pan1 = m_zoomState.panOffset.y;
						const float iw = (float)m_image->GetWidth();
						const float ih = (float)m_image->GetHeight();
						auto ImageToScreen = [&](float ix, float iy) -> ImVec2 {
							float normX = ix / iw;
							float normY = iy / ih;
							float sx2 = sdp.x + (normX - pan0) * sds.x / uxs;
							float sy2 = sdp.y + (normY - pan1) * sds.y / uys;
							return ImVec2(sx2, sy2);
						};
						//
						// Draw stamp preview at mouse position when hovering
						//
						if (hasPos) {
							ImDrawList* dl = ImGui::GetForegroundDrawList();
							ImVec2 topLeft = ImageToScreen((float)mx, (float)my);
							ImVec2 bottomRight = ImageToScreen((float)(mx + sw), (float)(my + sh));
							//
							// Draw each pixel of the stamp as a colored rectangle
							// Only draw when pixels are large enough to be visible (> 2px each)
							//
							float pixW = (bottomRight.x - topLeft.x) / (float)sw;
							float pixH = (bottomRight.y - topLeft.y) / (float)sh;
							if (pixW >= 2.0f && pixH >= 2.0f) {
								for (int row = 0; row < sh; row++) {
									for (int col = 0; col < sw; col++) {
										int packed = m_stampBuffer[row * sw + col];
										ImU32 col32;
										if (m_image->IsPaletized()) {
											RetrodevLib::RgbColor c = m_image->GetPaletteColor(packed);
											col32 = IM_COL32(c.r, c.g, c.b, 200);
										} else {
											col32 = IM_COL32((packed >> 16) & 0xFF, (packed >> 8) & 0xFF, packed & 0xFF, 200);
										}
										ImVec2 pMin = ImVec2(topLeft.x + col * pixW, topLeft.y + row * pixH);
										ImVec2 pMax = ImVec2(pMin.x + pixW, pMin.y + pixH);
										dl->AddRectFilled(pMin, pMax, col32);
									}
								}
							}
							//
							// Always draw outline around the stamp preview
							//
							dl->AddRect(ImVec2(topLeft.x - 1, topLeft.y - 1), ImVec2(bottomRight.x + 1, bottomRight.y + 1), IM_COL32(0, 0, 0, 220), 0.0f, 0, 2.0f);
							dl->AddRect(topLeft, bottomRight, IM_COL32(255, 255, 255, 220), 0.0f, 0, 1.0f);
							//
							// Paste stamp on click
							//
							if (m_zoomState.clicked) {
								CaptureSnapshot();
								for (int row = 0; row < sh; row++) {
									for (int col = 0; col < sw; col++) {
										int destX = mx + col;
										int destY = my + row;
										if (destX < 0 || destY < 0 || destX >= (int)iw || destY >= (int)ih)
											continue;
										int packed = m_stampBuffer[row * sw + col];
										if (m_image->IsPaletized())
											m_image->SetPixelIndex(destX, destY, packed);
										else {
											RetrodevLib::RgbColor c;
											c.r = (packed >> 16) & 0xFF;
											c.g = (packed >> 8) & 0xFF;
											c.b = packed & 0xFF;
											m_image->SetPixelColor(destX, destY, c);
										}
									}
								}
								SetModified(true);
								m_image->GetTexture(Application::GetRenderer());
							}
						}
					}
				}
			}
			ImGui::EndChild();
			//
			// Jump the splitter (only needed when there is a bottom panel)
			//
			if (isPaletized) {
				ImVec2 SplitterSpace = ImGui::GetCursorScreenPos();
				SplitterSpace.y += Application::splitterThickness;
				ImGui::SetCursorScreenPos(SplitterSpace);
			}
			// Bottom: operations panel (palette) — only for paletized images
			//
			if (isPaletized) {
				if (ImGui::BeginChild("OperationsPanel", ImVec2(-1, m_vSizeBottom), true)) {
					if (ImagePaletteWidget::Render(m_image)) {
						m_image->GetTexture(Application::GetRenderer());
					}
					m_activePen = ImagePaletteWidget::GetSelectedPen();
				}
				ImGui::EndChild();
			}
		}
		ImGui::EndChild();
		// Right area: tooling panel — only for paletized images
		//
		if (isPaletized) {
			ImGui::SameLine();
			if (ImGui::BeginChild("ToolingPanel", ImVec2(m_hSizeRight, 0), true)) {
				//
				// Save button
				//
				ImGui::AlignTextToFramePadding();
				ImGui::Text(IsModified() ? "Modified" : "Saved");
				ImGui::SameLine();
				if (ImGui::Button("Save"))
					Save();
				ImGui::Separator();
				PaintingWidget::Render(m_activePen, m_image);
			}
			ImGui::EndChild();
		}
	}
} // namespace RetrodevGui