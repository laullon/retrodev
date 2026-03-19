//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#include "painting.widget.h"
#include <app/app.icons.mdi.h>

namespace RetrodevGui {

	PaintingTool PaintingWidget::m_selectedTool = PaintingTool::None;
	int PaintingWidget::m_selectedColor = 0;
	int PaintingWidget::m_brushSize = 1;
	BrushShape PaintingWidget::m_brushShape = BrushShape::Square;
	int PaintingWidget::m_eraseColor = 0;
	PaintSlot PaintingWidget::m_activeSlot = PaintSlot::Paint;
	int PaintingWidget::m_fillTolerance = 0;

	bool PaintingWidget::Render(int selectedColor, std::shared_ptr<RetrodevLib::Image> image) {
		//
		// Accept the externally driven color index from ImagePaletteWidget
		//
		m_selectedColor = selectedColor;
		//
		// Active color section — always visible; dimmed when no drawing tool is active
		//
		ImGui::SeparatorText("Active Color");
		bool noTool = (m_selectedTool == PaintingTool::None);
		if (noTool)
			ImGui::BeginDisabled();
		//
		// Overlapping swatch pair drawn with ImDrawList
		// The active slot's swatch sits on top, covering the upper-left corner of the inactive one
		// Layout: total area is (swatchSize + overlap) wide and (swatchSize + overlap) tall
		//
		const float swatchSize = ImGui::GetFrameHeight() * 3.2f;
		const float overlap = swatchSize * 0.35f;
		const float totalW = swatchSize + overlap;
		const float totalH = swatchSize + overlap;
		//
		// Reserve space and center the swatch pair in the panel
		//
		float panelWidth = ImGui::GetContentRegionAvail().x;
		float offsetX = (panelWidth - totalW) * 0.5f;
		ImVec2 blockOrigin = ImGui::GetCursorScreenPos();
		blockOrigin.x += offsetX;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
		ImGui::Dummy(ImVec2(totalW, totalH));
		//
		// Determine which slot is active to decide draw order (active drawn last = on top)
		//
		bool paintActive = (m_activeSlot == PaintSlot::Paint);
		//
		// Swatch rects: paint = top-left, erase = bottom-right (offset by overlap)
		//
		ImVec2 paintMin = blockOrigin;
		ImVec2 paintMax = ImVec2(paintMin.x + swatchSize, paintMin.y + swatchSize);
		ImVec2 eraseMin = ImVec2(blockOrigin.x + overlap, blockOrigin.y + overlap);
		ImVec2 eraseMax = ImVec2(eraseMin.x + swatchSize, eraseMin.y + swatchSize);
		//
		// Resolve colors
		//
		ImVec4 paintCol = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
		ImVec4 eraseCol = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
		if (image && image->IsPaletized()) {
			if (m_selectedColor >= 0) {
				RetrodevLib::RgbColor c = image->GetPaletteColor(m_selectedColor);
				paintCol = ImVec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f);
			}
			if (m_eraseColor >= 0) {
				RetrodevLib::RgbColor e = image->GetPaletteColor(m_eraseColor);
				eraseCol = ImVec4(e.r / 255.0f, e.g / 255.0f, e.b / 255.0f, 1.0f);
			}
		}
		//
		// Draw helper: fills rect and draws border (gold if active, dark grey otherwise)
		//
		ImDrawList* dl = ImGui::GetWindowDrawList();
		auto DrawSwatch = [&](ImVec2 mn, ImVec2 mx, ImVec4 col, bool active) {
			dl->AddRectFilled(mn, mx, ImGui::ColorConvertFloat4ToU32(col));
			ImU32 borderCol = active ? IM_COL32(255, 217, 0, 255) : IM_COL32(80, 80, 80, 255);
			float borderThick = active ? 2.5f : 1.0f;
			dl->AddRect(mn, mx, borderCol, 0.0f, 0, borderThick);
		};
		//
		// Draw inactive swatch first, then active on top
		//
		if (paintActive) {
			DrawSwatch(eraseMin, eraseMax, eraseCol, false);
			DrawSwatch(paintMin, paintMax, paintCol, true);
		} else {
			DrawSwatch(paintMin, paintMax, paintCol, false);
			DrawSwatch(eraseMin, eraseMax, eraseCol, true);
		}
		//
		// Invisible buttons for hit-testing — placed after Dummy so they don't shift layout
		// Paint button covers the paint swatch area not obscured by erase swatch when erase is active
		// Erase button covers the erase swatch area not obscured by paint swatch when paint is active
		// We use the full swatch rects; ImGui processes them in order so the top one wins on overlap
		//
		ImGui::SetCursorScreenPos(paintActive ? eraseMin : paintMin);
		if (paintActive) {
			//
			// Erase is behind, paint is on top — click erase area first (behind), then paint on top
			//
			ImGui::InvisibleButton("##erasehit", ImVec2(swatchSize, swatchSize));
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
				m_activeSlot = PaintSlot::Erase;
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
				ImGui::OpenPopup("##erasepicker");
			if (ImGui::IsItemHovered() && image && image->IsPaletized())
				ImGui::SetTooltip("Erase\nIndex %d\nLeft-click: activate  Right-click: change color", m_eraseColor);
			ImGui::SetCursorScreenPos(paintMin);
			ImGui::InvisibleButton("##painthit", ImVec2(swatchSize, swatchSize));
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
				m_activeSlot = PaintSlot::Paint;
			if (ImGui::IsItemHovered() && image && image->IsPaletized())
				ImGui::SetTooltip("Paint\nIndex %d\nClick to activate", m_selectedColor);
		} else {
			//
			// Paint is behind, erase is on top — click paint area first (behind), then erase on top
			//
			ImGui::InvisibleButton("##painthit", ImVec2(swatchSize, swatchSize));
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
				m_activeSlot = PaintSlot::Paint;
			if (ImGui::IsItemHovered() && image && image->IsPaletized())
				ImGui::SetTooltip("Paint\nIndex %d\nClick to activate", m_selectedColor);
			ImGui::SetCursorScreenPos(eraseMin);
			ImGui::InvisibleButton("##erasehit", ImVec2(swatchSize, swatchSize));
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
				m_activeSlot = PaintSlot::Erase;
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
				ImGui::OpenPopup("##erasepicker");
			if (ImGui::IsItemHovered() && image && image->IsPaletized())
				ImGui::SetTooltip("Erase\nIndex %d\nLeft-click: activate  Right-click: change color", m_eraseColor);
		}
		//
		// Restore cursor below the swatch block for subsequent widgets
		//
		ImGui::SetCursorScreenPos(ImVec2(blockOrigin.x - offsetX, blockOrigin.y + totalH + ImGui::GetStyle().ItemSpacing.y));
		//
		// Erase color picker popup: palette grid, right-click on erase swatch to open
		//
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
		if (ImGui::BeginPopup("##erasepicker")) {
			ImGui::TextUnformatted("Pick erase color:");
			ImGui::Separator();
			if (image && image->IsPaletized()) {
				int paletteSize = image->GetPaletteSize();
				const float sw = 22.0f;
				const float sg = 3.0f;
				int perRow = 8;
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(sg, sg));
				for (int i = 0; i < paletteSize; i++) {
					RetrodevLib::RgbColor pc = image->GetPaletteColor(i);
					ImVec4 pcCol = ImVec4(pc.r / 255.0f, pc.g / 255.0f, pc.b / 255.0f, 1.0f);
					bool sel = (i == m_eraseColor);
					ImGui::PushID(i);
					if (sel) {
						ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.85f, 0.0f, 1.0f));
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.5f);
					}
					if (ImGui::ColorButton("##ep", pcCol, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoBorder, ImVec2(sw, sw))) {
						m_eraseColor = i;
						ImGui::CloseCurrentPopup();
					}
					if (sel) {
						ImGui::PopStyleVar();
						ImGui::PopStyleColor();
					}
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("Index %d  R:%d G:%d B:%d", i, pc.r, pc.g, pc.b);
					ImGui::PopID();
					if ((i + 1) % perRow != 0 && i < paletteSize - 1)
						ImGui::SameLine();
				}
				ImGui::PopStyleVar();
			}
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();
		if (noTool)
			ImGui::EndDisabled();
		//
		// Tool selection
		//
		ImGui::SeparatorText("Tool");
		PaintingTool prevTool = m_selectedTool;
		RenderToolButtons();
		//
		// Auto-arm the matching slot when the user switches tools:
		// Eraser automatically activates the Erase slot; any other drawing tool activates Paint slot
		//
		if (m_selectedTool != prevTool) {
			if (m_selectedTool == PaintingTool::Eraser)
				m_activeSlot = PaintSlot::Erase;
			else if (m_selectedTool != PaintingTool::None && m_selectedTool != PaintingTool::RegionSelect)
				m_activeSlot = PaintSlot::Paint;
		}
		//
		// Brush size and shape — shown for all tools that draw stroked pixels
		//
		bool hasBrush = (m_selectedTool == PaintingTool::Pencil || m_selectedTool == PaintingTool::Eraser || m_selectedTool == PaintingTool::Line ||
						 m_selectedTool == PaintingTool::RectOutline || m_selectedTool == PaintingTool::RectFill || m_selectedTool == PaintingTool::EllipseOutline ||
						 m_selectedTool == PaintingTool::EllipseFill);
		if (hasBrush) {
			ImGui::SeparatorText("Brush Size & Shape");
			RenderBrushSizeButtons();
		}
		//
		// Fill tolerance — only relevant for Fill tool
		//
		if (m_selectedTool == PaintingTool::Fill) {
			ImGui::SeparatorText("Fill Tolerance");
			float panelWidth = ImGui::GetContentRegionAvail().x;
			ImGui::SetNextItemWidth(panelWidth);
			ImGui::SliderInt("##filltolerance", &m_fillTolerance, 0, 255);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Color tolerance: %d\n0 = exact match only\n255 = fill everything", m_fillTolerance);
		}
		return false;
	}

	void PaintingWidget::RenderToolButtons() {
		//
		// Tool entries: icon, tooltip, tool value — laid out in a 5-column grid
		//
		struct ToolEntry {
			const char* icon;
			const char* tip;
			PaintingTool tool;
		};
		static const ToolEntry tools[] = {
			{ICON_CURSOR_DEFAULT, "No Tool\nPan & Zoom", PaintingTool::None},
			{ICON_LEAD_PENCIL, "Pencil", PaintingTool::Pencil},
			{ICON_ERASER, "Eraser", PaintingTool::Eraser},
			{ICON_FORMAT_COLOR_FILL, "Fill", PaintingTool::Fill},
			{ICON_SELECT_DRAG, "Region Select", PaintingTool::RegionSelect},
			{ICON_VECTOR_LINE, "Line", PaintingTool::Line},
			{ICON_RECTANGLE_OUTLINE, "Rectangle (outline)", PaintingTool::RectOutline},
			{ICON_VECTOR_SQUARE, "Rectangle (filled)", PaintingTool::RectFill},
			{ICON_ELLIPSE_OUTLINE, "Ellipse (outline)", PaintingTool::EllipseOutline},
			{ICON_ELLIPSE, "Ellipse (filled)", PaintingTool::EllipseFill},
		};
		//
		// 5 buttons per row, 2 rows
		//
		const int cols = 5;
		const int count = 10;
		float spacing = ImGui::GetStyle().ItemSpacing.x;
		float totalSpacing = spacing * (cols - 1);
		float btnW = (ImGui::GetContentRegionAvail().x - totalSpacing) / cols;
		float btnH = btnW;
		for (int i = 0; i < count; i++) {
			if (i > 0 && i % cols != 0)
				ImGui::SameLine();
			bool active = (m_selectedTool == tools[i].tool);
			if (active)
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			if (ImGui::Button((std::string(tools[i].icon) + "##t" + std::to_string(i)).c_str(), ImVec2(btnW, btnH)))
				m_selectedTool = tools[i].tool;
			if (active)
				ImGui::PopStyleColor();
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", tools[i].tip);
		}
	}

	void PaintingWidget::RenderBrushSizeButtons() {
		float panelWidth = ImGui::GetContentRegionAvail().x;
		//
		// Shape selector: Square, Circle, Diamond
		//
		struct ShapeEntry {
			const char* icon;
			const char* tip;
			BrushShape shape;
		};
		static const ShapeEntry shapes[] = {
			{ICON_SQUARE_OUTLINE, "Square", BrushShape::Square},
			{ICON_CIRCLE_OUTLINE, "Circle", BrushShape::Circle},
			{ICON_SQUARE_ROUNDED_OUTLINE, "Diamond", BrushShape::Diamond},
		};
		const int shapeCount = 3;
		float shapeSpacing = ImGui::GetStyle().ItemSpacing.x;
		float shapeBtnW = (panelWidth - shapeSpacing * (shapeCount - 1)) / shapeCount;
		float shapeBtnH = ImGui::GetFrameHeight() * 1.6f;
		for (int i = 0; i < shapeCount; i++) {
			if (i > 0)
				ImGui::SameLine();
			bool active = (m_brushShape == shapes[i].shape);
			if (active)
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			if (ImGui::Button((std::string(shapes[i].icon) + "##sh" + std::to_string(i)).c_str(), ImVec2(shapeBtnW, shapeBtnH)))
				m_brushShape = shapes[i].shape;
			if (active)
				ImGui::PopStyleColor();
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", shapes[i].tip);
		}
		//
		// Slider for brush size 1–8
		//
		ImGui::SetNextItemWidth(panelWidth);
		ImGui::SliderInt("##brushsize", &m_brushSize, 1, 8);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Brush size: %d pixel(s)", m_brushSize);
		//
		// Shape preview drawn with ImDrawList — shows the actual brush footprint
		//
		const float maxPreview = 48.0f;
		const float minPreview = 8.0f;
		float previewArea = minPreview + (maxPreview - minPreview) * ((m_brushSize - 1) / 7.0f);
		float previewX = ImGui::GetCursorScreenPos().x + (panelWidth - previewArea) * 0.5f;
		float previewY = ImGui::GetCursorScreenPos().y;
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImU32 shapeColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text));
		if (m_brushShape == BrushShape::Square) {
			drawList->AddRectFilled(ImVec2(previewX, previewY), ImVec2(previewX + previewArea, previewY + previewArea), shapeColor);
		} else if (m_brushShape == BrushShape::Circle) {
			float cx = previewX + previewArea * 0.5f;
			float cy = previewY + previewArea * 0.5f;
			drawList->AddCircleFilled(ImVec2(cx, cy), previewArea * 0.5f, shapeColor);
		} else if (m_brushShape == BrushShape::Diamond) {
			float cx = previewX + previewArea * 0.5f;
			float cy = previewY + previewArea * 0.5f;
			float r = previewArea * 0.5f;
			ImVec2 pts[4] = {
				ImVec2(cx, cy - r),
				ImVec2(cx + r, cy),
				ImVec2(cx, cy + r),
				ImVec2(cx - r, cy),
			};
			drawList->AddConvexPolyFilled(pts, 4, shapeColor);
		}
		//
		// Advance cursor past the preview area
		//
		ImGui::Dummy(ImVec2(panelWidth, previewArea));
	}

} // namespace RetrodevGui
