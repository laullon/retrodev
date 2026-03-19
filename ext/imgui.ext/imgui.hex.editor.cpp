//----------------------------------------------------------------------------------------------------
//
//
//
// 
//
//----------------------------------------------------------------------------------------------------

#include "imgui.hex.editor.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <ctype.h>

namespace ImGui::HexEditor {

//
// Convert the lower nibble of a byte to its printable hex character.
// lower=true produces lowercase (a-f), lower=false produces uppercase (A-F).
//
char HalfByteToPrintable(unsigned char half_byte, bool lower) {
	IM_ASSERT(!(half_byte & 0xf0));
	return half_byte <= 9 ? '0' + half_byte : (lower ? 'a' : 'A') + half_byte - 10;
}
//
// Map an ImGuiKey (0-9, A-F) to the corresponding nibble value (0-15).
//
unsigned char KeyToHalfByte(ImGuiKey key) {
	IM_ASSERT((key >= ImGuiKey_A && key <= ImGuiKey_F) || (key >= ImGuiKey_0 && key <= ImGuiKey_9));
	return (key >= ImGuiKey_A && key <= ImGuiKey_F) ? (char)(key - ImGuiKey_A) + 10 : (char)(key - ImGuiKey_0);
}
//
// Returns true when the byte has a visible ASCII glyph (printable range '!' to '~').
//
bool HasAsciiRepresentation(unsigned char byte) {
	return (byte >= '!' && byte <= '~');
}
//
// Calculate how many bytes fit per display row given the available pixel width.
// The calculation is iterative: a first pass computes the raw count ignoring
// separator gaps, then a second pass subtracts the separator pixel cost and
// recomputes to get the final value.
//
int CalcBytesPerLine(float bytes_avail_x, const ImVec2& byte_size, const ImVec2& spacing, bool show_ascii, const ImVec2& char_size, int separators) {
	const float byte_width = byte_size.x + spacing.x + (show_ascii ? char_size.x : 0.f);
	int bytes_per_line = (int)(bytes_avail_x / byte_width);
	bytes_per_line = bytes_per_line <= 0 ? 1 : bytes_per_line;

	int actual_separators = separators > 0 ? (int)(bytes_per_line / separators) : 0;
	if (actual_separators != 0 && separators > 0 && bytes_per_line > actual_separators && (bytes_per_line - 1) % actual_separators == 0)
		--actual_separators;

	return separators > 0 ? CalcBytesPerLine(bytes_avail_x - (actual_separators * spacing.x), byte_size, spacing, show_ascii, char_size, 0) : bytes_per_line;
}

//
// Return black or white, whichever has better contrast against the given color.
// Uses the standard perceptual luminance formula (BT.601).
//
ImColor CalcContrastColor(ImColor color) {
#ifdef IMGUI_USE_BGRA_PACKED_COLOR
	const float l = (0.299f * (color.Value.z * 255.f) + 0.587f * (color.Value.y * 255.f) + 0.114f * (color.Value.x * 255.f)) / 255.f;
#else
	const float l = (0.299f * (color.Value.x * 255.f) + 0.587f * (color.Value.y * 255.f) + 0.114f * (color.Value.z * 255.f)) / 255.f;
#endif
	const int c = l > 0.5f ? 0 : 255;
	return IM_COL32(c, c, c, 255);
}

//
// Compute the intersection of two integer ranges [a_min, a_max] and [b_min, b_max].
// Returns true and writes the overlap into out_min/out_max when the ranges overlap.
//
bool RangeRangeIntersection(int a_min, int a_max, int b_min, int b_max, int* out_min, int* out_max) {
	if (a_max < b_min || b_max < a_min)
		return false;

	*out_min = ImMax(a_min, b_min);
	*out_max = ImMin(a_max, b_max);

	if (*out_min <= *out_max)
		return true;

	return false;
}

//
// Clamp rounding so it never exceeds half the shortest side of the rect,
// preventing the arcs from overlapping on small boxes.
//
void RenderRectCornerCalcRounding(const ImVec2& ra, const ImVec2& rb, float& rounding) {
	rounding = ImMin(rounding, ImFabs(rb.x - ra.x) * 0.5f);
	rounding = ImMin(rounding, ImFabs(rb.y - ra.y) * 0.5f);
}

//
// The following six helpers stroke a partial open-rectangle border
// for a specific corner/edge combination used when drawing multi-byte
// selection or highlight regions that span row boundaries.
// Each function traces only the sides that belong to that corner so
// that adjacent cells can tile seamlessly without double-drawing edges.
//
void RenderTopLeftCornerRect(ImDrawList* draw_list, const ImVec2& a, const ImVec2& b, ImColor color, float rounding) {
	const ImVec2 ra = {a.x + 0.5f, a.y + 0.5f};
	const ImVec2 rb = {b.x, b.y};

	RenderRectCornerCalcRounding(ra, rb, rounding);

	draw_list->PathArcToFast({ra.x, rb.y}, 0, 3, 6);
	draw_list->PathArcToFast({ra.x + rounding, ra.y + rounding}, rounding, 6, 9);
	draw_list->PathArcToFast({rb.x, ra.y}, 0, 9, 12);

	draw_list->PathStroke(color, ImDrawFlags_None, 1.f);
}

void RenderBottomRightCornerRect(ImDrawList* draw_list, const ImVec2& a, const ImVec2& b, ImColor color, float rounding) {
	const ImVec2 ra = {a.x, a.y + 0.5f};
	const ImVec2 rb = {b.x - 0.5f, b.y + 0.5f};

	RenderRectCornerCalcRounding(ra, rb, rounding);

	draw_list->PathArcToFast({rb.x, ra.y}, 0, 9, 12);
	draw_list->PathArcToFast({rb.x - rounding, rb.y - rounding}, rounding, 0, 3);
	draw_list->PathArcToFast({ra.x, rb.y}, 0, 3, 6);

	draw_list->PathStroke(color, ImDrawFlags_None, 1.f);
}

void RenderTopRightCornerRect(ImDrawList* draw_list, const ImVec2& a, const ImVec2& b, ImColor color, float rounding) {
	const ImVec2 ra = {a.x + 0.5f, a.y + 0.5f};
	const ImVec2 rb = {b.x - 0.5f, b.y};

	RenderRectCornerCalcRounding(ra, rb, rounding);

	draw_list->PathArcToFast(ra, 0.f, 6, 9);
	draw_list->PathArcToFast({rb.x - rounding, ra.y + rounding}, rounding, 9, 12);
	draw_list->PathArcToFast(rb, 0.f, 0, 3);

	draw_list->PathStroke(color, ImDrawFlags_None, 1.f);
}

void RenderBottomLeftCornerRect(ImDrawList* draw_list, const ImVec2& a, const ImVec2& b, ImColor color, float rounding) {
	const ImVec2 ra = {a.x + 0.5f, a.y + 0.5f};
	const ImVec2 rb = {b.x + 0.5f, b.y + 0.5f};

	RenderRectCornerCalcRounding(ra, rb, rounding);

	draw_list->PathArcToFast({rb.x, rb.y}, 0.f, 0, 3);
	draw_list->PathArcToFast({ra.x + rounding, rb.y - rounding}, rounding, 3, 6);
	draw_list->PathArcToFast({ra.x, ra.y}, 0.f, 9, 12);

	draw_list->PathStroke(color, ImDrawFlags_None, 1.f);
}

void RenderBottomCornerRect(ImDrawList* draw_list, const ImVec2& a, const ImVec2& b, ImColor color, float rounding) {
	const ImVec2 ra = {a.x + 0.5f, a.y + 0.5f};
	const ImVec2 rb = {b.x + 0.5f, b.y + 0.5f};

	RenderRectCornerCalcRounding(ra, rb, rounding);

	draw_list->PathArcToFast({rb.x, ra.y}, 0.f, 0, 3);
	draw_list->PathArcToFast({rb.x - rounding, rb.y - rounding}, rounding, 0, 3);
	draw_list->PathArcToFast({ra.x + rounding, rb.y - rounding}, rounding, 3, 6);
	draw_list->PathArcToFast({ra.x, ra.y}, 0.f, 9, 12);

	draw_list->PathStroke(color, ImDrawFlags_None, 1.f);
}

void RenderTopCornerRect(ImDrawList* draw_list, const ImVec2& a, const ImVec2& b, ImColor color, float rounding) {
	const ImVec2 ra = {a.x + 0.5f, a.y + 0.5f};
	const ImVec2 rb = {b.x - 0.5f, b.y + 0.5f};

	RenderRectCornerCalcRounding(ra, rb, rounding);

	draw_list->PathArcToFast({ra.x, rb.y}, 0.f, 3, 6);
	draw_list->PathArcToFast({ra.x + rounding, ra.y + rounding}, rounding, 6, 9);
	draw_list->PathArcToFast({rb.x - rounding, ra.y + rounding}, rounding, 9, 12);
	draw_list->PathArcToFast({rb.x, rb.y}, 0.f, 0, 3);

	draw_list->PathStroke(color, ImDrawFlags_None, 1.f);
}

//
// Render the background fill and optional border strokes for a single byte cell
// at column index `i` within the current display row (base offset `line_base`).
// `offset` is the absolute byte offset; `range_min`/`range_max` are the full
// extent of the highlight region so that the correct rounded corner variant
// can be chosen for each cell position (start, end, interior, row-wrap, etc.).
//
void RenderByteDecorations(ImDrawList* draw_list, const ImRect& bb, ImColor bg_color, HighlightFlags flags, ImColor border_color, float rounding, int offset,
								  int range_min, int range_max, int bytes_per_line, int i, int line_base) {
	const bool has_border = flags & HighlightFlags_Border;

	//
	// Fast paths: no border requested, or the entire range is a single byte.
	//
	if (!has_border) {
		draw_list->AddRectFilled(bb.Min, bb.Max, bg_color, 0.f);
		return;
	}

	if (range_min == range_max) {
		draw_list->AddRectFilled(bb.Min, bb.Max, bg_color, rounding);
		draw_list->AddRect(bb.Min, bb.Max, border_color, rounding);
		return;
	}
	//
	// Determine which display lines the range starts and ends on, and whether
	// the current cell sits on those boundary lines.
	//
	const int start_line = range_min / bytes_per_line;
	const int end_line = range_max / bytes_per_line;
	const int current_line = line_base / bytes_per_line;

	const bool is_start_line = start_line == (line_base / bytes_per_line);
	const bool is_end_line = end_line == (line_base / bytes_per_line);
	const bool is_last_byte = i == (bytes_per_line - 1);

	bool rendered_bg = false;

	//
	// First byte of the range: round the top-left (or both top corners when the
	// range fits on a single row and this is also the last column).
	//
	if (offset == range_min) {
		if (!is_last_byte) {
			draw_list->AddRectFilled(bb.Min, bb.Max, bg_color, rounding, ImDrawFlags_RoundCornersTopLeft);
			RenderTopLeftCornerRect(draw_list, bb.Min, bb.Max, border_color, rounding);

			if (start_line == end_line)
				draw_list->AddLine({bb.Min.x, bb.Max.y}, {bb.Max.x, bb.Max.y}, border_color);
		} else {
			draw_list->AddRectFilled(bb.Min, bb.Max, bg_color, rounding, ImDrawFlags_RoundCornersTop);
			RenderTopCornerRect(draw_list, bb.Min, bb.Max, border_color, rounding);
		}

		rendered_bg = true;
	} else if (i == 0) {
		if (is_end_line) {
			if (offset == range_max) {
				draw_list->AddRectFilled(bb.Min, bb.Max, bg_color, rounding, ImDrawFlags_RoundCornersBottom);
				RenderBottomCornerRect(draw_list, bb.Min, bb.Max, border_color, rounding);
			} else {
				draw_list->AddRectFilled(bb.Min, bb.Max, bg_color, rounding, ImDrawFlags_RoundCornersBottomLeft);
				RenderBottomLeftCornerRect(draw_list, bb.Min, bb.Max, border_color, rounding);
			}

			rendered_bg = true;
		} else if (current_line == start_line + 1 && (range_min % bytes_per_line) != 0) {
			draw_list->AddRectFilled(bb.Min, bb.Max, bg_color, rounding, ImDrawFlags_RoundCornersTopLeft);
			RenderTopLeftCornerRect(draw_list, bb.Min, bb.Max, border_color, rounding);
			rendered_bg = true;
		} else {
			if (!rendered_bg) {
				draw_list->AddRectFilled(bb.Min, bb.Max, bg_color, 0.f);
				rendered_bg = true;
			}

			draw_list->AddLine({bb.Min.x, bb.Min.y}, {bb.Min.x, bb.Max.y}, border_color);
		}
	}

	if (i != 0 && offset == range_max) {
		if (start_line == end_line) {
			draw_list->AddRectFilled(bb.Min, bb.Max, bg_color, rounding, ImDrawFlags_RoundCornersTopRight);
			RenderTopRightCornerRect(draw_list, bb.Min, bb.Max, border_color, rounding);
			draw_list->AddLine({bb.Min.x, bb.Max.y}, {bb.Max.x, bb.Max.y}, border_color);
		} else {
			draw_list->AddRectFilled(bb.Min, bb.Max, bg_color, rounding, ImDrawFlags_RoundCornersBottomRight);
			RenderBottomRightCornerRect(draw_list, bb.Min, bb.Max, border_color, rounding);
		}

		rendered_bg = true;
	} else if (is_last_byte && offset != range_min) {
		if (is_start_line) {
			draw_list->AddRectFilled(bb.Min, bb.Max, bg_color, rounding, ImDrawFlags_RoundCornersTopRight);
			RenderTopRightCornerRect(draw_list, bb.Min, bb.Max, border_color, rounding);
			rendered_bg = true;
		} else if (current_line == end_line - 1 && (range_max % bytes_per_line) != bytes_per_line - 1) {
			draw_list->AddRectFilled(bb.Min, bb.Max, bg_color, rounding, ImDrawFlags_RoundCornersBottomRight);
			RenderBottomRightCornerRect(draw_list, bb.Min, bb.Max, border_color, rounding);
			rendered_bg = true;
		} else {
			if (!rendered_bg) {
				draw_list->AddRectFilled(bb.Min, bb.Max, bg_color, 0.f);
				rendered_bg = true;
			}

			draw_list->AddLine({bb.Max.x - 1.f, bb.Min.y}, {bb.Max.x - 1.f, bb.Max.y}, border_color);
		}
	}

	if ((is_start_line && offset != range_min && !is_last_byte && offset != range_max) || (current_line == start_line + 1 && (i < (range_min % bytes_per_line) && i != 0))) {
		if (!rendered_bg) {
			draw_list->AddRectFilled(bb.Min, bb.Max, bg_color, 0.f);
			rendered_bg = true;
		}

		draw_list->AddLine({bb.Min.x, bb.Min.y}, {bb.Max.x, bb.Min.y}, border_color);
	}

	if ((is_end_line && offset != range_max && i != 0) || (current_line == end_line - 1 && (i > (range_max % bytes_per_line) && !is_last_byte))) {
		if (!rendered_bg) {
			draw_list->AddRectFilled(bb.Min, bb.Max, bg_color, 0.f);
			rendered_bg = true;
		}

		draw_list->AddLine({bb.Min.x, bb.Max.y}, {bb.Max.x, bb.Max.y}, border_color);
	}

	if (!rendered_bg)
		draw_list->AddRectFilled(bb.Min, bb.Max, bg_color, 0.f);
}

//
// Main entry point. Creates the hex editor child window and renders all content
// for this frame. Returns false only when the window is clipped (same semantics
// as BeginChild). EndHexEditor must always be called after this.
//
bool Begin(const char* str_id, State* state, const ImVec2& size, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags) {
	if (!ImGui::BeginChild(str_id, size, child_flags, window_flags))
		return false;
	//
	// Measure a single monospace glyph; derive byte-cell size (two hex digits wide).
	//
	const ImVec2 char_size = ImGui::CalcTextSize("0");
	const ImVec2 byte_size = {char_size.x * 2.f, char_size.y};

	const ImGuiStyle& style = ImGui::GetStyle();
	const ImVec2 spacing = style.ItemSpacing;

	ImVec2 content_avail = ImGui::GetContentRegionAvail();
	//
	// Compute how wide the address column is; when ShowAddress is off both
	// values are zero so the byte area starts at the left edge.
	//
	float address_max_size;
	int address_max_chars;
	if (state->ShowAddress) {
		int address_chars = state->AddressChars;
		if (address_chars == -1)
			address_chars = ImFormatString(nullptr, 0, "%zX", (size_t)state->MaxBytes) + 1;

		address_max_chars = address_chars + 1;
		address_max_size = char_size.x * address_max_chars + spacing.x * 0.5f;
	} else {
		address_max_size = 0.f;
		address_max_chars = 0;
	}

	//
	// Subtract the address column and, when a scrollbar is visible, its width
	// from the available horizontal space before fitting bytes per row.
	//
	float bytes_avail_x = content_avail.x - address_max_size;
	if (ImGui::GetScrollMaxY() > 0.f)
		bytes_avail_x -= style.ScrollbarSize;

	const bool show_ascii = state->ShowAscii;
	//
	// Reserve a half-char gap before the ASCII sidebar separator line.
	//
	if (show_ascii)
		bytes_avail_x -= char_size.x * 0.5f;

	bytes_avail_x = bytes_avail_x < 0.f ? 0.f : bytes_avail_x;
	//
	// Resolve bytes per line: auto-fit when BytesPerLine == -1, fixed otherwise.
	//
	int bytes_per_line;

	if (state->BytesPerLine == -1) {
		bytes_per_line = CalcBytesPerLine(bytes_avail_x, byte_size, spacing, show_ascii, char_size, state->Separators);
	} else {
		bytes_per_line = state->BytesPerLine;
	}
	//
	// Compute the actual number of separator gaps that will appear in each row
	// (one gap every N bytes, excluding the last position).
	//
	int actual_separators = (int)(bytes_per_line / state->Separators);
	if (bytes_per_line % state->Separators == 0)
		--actual_separators;
	//
	// Total display rows needed, rounded up to cover any partial final row.
	//
	int lines_count;
	if (bytes_per_line != 0) {
		lines_count = state->MaxBytes / bytes_per_line;
		if (lines_count * bytes_per_line < state->MaxBytes) {
			++lines_count;
		}
	} else
		lines_count = 0;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImGuiIO& io = ImGui::GetIO();
	//
	// Cache all theme colors used during rendering so style lookups happen once.
	//
	const ImColor text_color = ImGui::GetColorU32(ImGuiCol_Text);
	const ImColor text_disabled_color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
	const ImColor text_selected_bg_color = ImGui::GetColorU32(ImGuiCol_TextSelectedBg);
	const ImColor separator_color = ImGui::GetColorU32(ImGuiCol_Separator);
	const ImColor border_color = ImGui::GetColorU32(ImGuiCol_FrameBgActive);

	const bool lowercase_bytes = state->LowercaseBytes;
	//
	// Snapshot the current selection state at the start of the frame.
	// All mutations are written into the next_* variables and flushed back
	// into state at the end of the function so mid-frame reads stay consistent.
	//
	const int select_start_byte = state->SelectStartByte;
	const int select_start_subbyte = state->SelectStartSubByte;
	const int select_end_byte = state->SelectEndByte;
	const int select_end_subbyte = state->SelectEndSubByte;
	const int last_selected_byte = state->LastSelectedByte;
	const int select_drag_byte = state->SelectDragByte;
	const int select_drag_subbyte = state->SelectDragSubByte;

	int next_select_start_byte = select_start_byte;
	int next_select_start_subbyte = select_start_subbyte;
	int next_select_end_byte = select_end_byte;
	int next_select_end_subbyte = select_end_subbyte;
	int next_last_selected_byte = last_selected_byte;
	int next_select_drag_byte = select_drag_byte;
	int next_select_drag_subbyte = select_drag_subbyte;

	ImGuiKey hex_key_pressed = ImGuiKey_None;

	//
		// Ctrl+C: copy the selected bytes to the clipboard as space-separated hex.
		// When ClipboardFlags_Multiline is set, a newline is inserted at every row boundary.
		//
		if (state->EnableClipboard && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_C)) {
			if (state->SelectStartByte != -1) {
				const int bytes_count = (state->SelectEndByte + 1) - state->SelectStartByte;

			char* bytes = (char*)ImGui::MemAlloc((size_t)bytes_count);
			if (bytes) {
				int read_bytes;

				if (state->ReadCallback)
					read_bytes = state->ReadCallback(state, state->SelectStartByte, bytes, bytes_count);
				else {
					memcpy(bytes, (char*)state->Bytes + state->SelectStartByte, bytes_count);
					read_bytes = bytes_count;
				}

				if (read_bytes > 0) {
						ImGui::LogToClipboard();

					for (int i = 0, abs_i = state->SelectStartByte; i < bytes_count; i++, abs_i++) {
						const char byte = bytes[i];

						char text[3];
						text[0] = HalfByteToPrintable((byte & 0xf0) >> 4, lowercase_bytes);
						text[1] = HalfByteToPrintable(byte & 0x0f, lowercase_bytes);
						text[2] = '\0';

						ImGui::LogText("%s", text);

						if (bytes_per_line != 0 && ((abs_i % bytes_per_line) == bytes_per_line - 1) && abs_i != 0) {
							ImGui::LogText(IM_NEWLINE);
						} else {
							ImGui::LogText(" ");
						}
					}

					ImGui::LogFinish();
				}

				ImGui::MemFree(bytes);
			}
		}
	} else {
		//
		// Keyboard navigation: arrow keys move the cursor one nibble or one row at a time.
		// A-F and 0-9 key presses are captured for in-place byte editing.
		//
		if (last_selected_byte != -1) {
			bool any_pressed = false;
			if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
				if (!select_start_subbyte) {
					if (last_selected_byte == 0) {
						next_last_selected_byte = 0;
					} else {
						next_last_selected_byte = last_selected_byte - 1;
						next_select_start_subbyte = 1;
					}
				} else
					next_select_start_subbyte = 0;

				any_pressed = true;
			} else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
				if (select_start_subbyte) {
					if (last_selected_byte >= state->MaxBytes - 1) {
						next_last_selected_byte = state->MaxBytes - 1;
					} else {
						next_last_selected_byte = last_selected_byte + 1;
						next_select_start_subbyte = 0;
					}
				} else
					next_select_start_subbyte = 1;

				any_pressed = true;
			} else if (bytes_per_line != 0) {
				if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
					if (last_selected_byte >= bytes_per_line) {
						next_last_selected_byte = last_selected_byte - bytes_per_line;
					}

					any_pressed = true;
				} else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
					if (last_selected_byte < state->MaxBytes - bytes_per_line) {
						next_last_selected_byte = last_selected_byte + bytes_per_line;
					}

					any_pressed = true;
				}
			}

			if (any_pressed) {
				next_select_start_byte = next_last_selected_byte;
				next_select_end_byte = next_last_selected_byte;
			}
		}

		for (ImGuiKey key = ImGuiKey_A; key != ImGuiKey_G; key = (ImGuiKey)((int)key + 1)) {
			if (ImGui::IsKeyPressed(key)) {
				hex_key_pressed = key;
				break;
			}
		}

		if (hex_key_pressed == ImGuiKey_None) {
			for (ImGuiKey key = ImGuiKey_0; key != ImGuiKey_A; key = (ImGuiKey)((int)key + 1)) {
				if (ImGui::IsKeyPressed(key)) {
					hex_key_pressed = key;
					break;
				}
			}
		}
	}

	//
	// Allocate per-row read buffers. Use stack storage when the row fits in the
	// fixed-size arrays; fall back to heap for unusually wide fixed layouts.
	//
	unsigned char stack_line_buf[128];
	unsigned char* line_buf = (size_t)bytes_per_line <= sizeof(stack_line_buf) ? stack_line_buf : (unsigned char*)ImGui::MemAlloc(bytes_per_line);
	if (!line_buf)
		return true;

	char stack_address_buf[32];
	char* address_buf = (size_t)address_max_chars <= sizeof(stack_address_buf) ? stack_address_buf : (char*)ImGui::MemAlloc(address_max_chars);
	if (!address_buf) {
		if (line_buf != stack_line_buf)
			ImGui::MemFree(line_buf);

		return true;
	}

	const ImVec2 mouse_pos = ImGui::GetMousePos();
	const bool mouse_left_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);

	//
	// Virtual-scroll: the clipper tells us which rows are actually visible so we
	// skip read and draw work for off-screen rows entirely.
	//
	ImGuiListClipper clipper;
	clipper.Begin(lines_count, byte_size.y + spacing.y);
	while (clipper.Step()) {
		const int clipper_lines = clipper.DisplayEnd - clipper.DisplayStart;

		ImVec2 cursor = ImGui::GetCursorScreenPos();
		//
		// The ASCII sidebar starts to the right of all byte columns and separator gaps.
		// Draw the vertical separator line that runs the full height of the visible block.
		//
		ImVec2 ascii_cursor = {cursor.x + address_max_size + (spacing.x * 0.5f) + (bytes_per_line * (byte_size.x + spacing.x)) + (actual_separators * spacing.x), cursor.y};
		if (show_ascii) {
			draw_list->AddLine(ascii_cursor, {ascii_cursor.x, ascii_cursor.y + clipper_lines * (byte_size.y + spacing.y)}, separator_color);
		}
		//
		// Pre-reserve draw-list capacity: 2 quads per byte (hex text + optional ascii),
		// each quad needs 6 indices and 4 vertices.
		//
		{
			int count = clipper_lines * bytes_per_line * 2;
			if (show_ascii)
				count += clipper_lines * bytes_per_line;

			draw_list->IdxBuffer.reserve(draw_list->IdxBuffer.Size + (count * 6));
			draw_list->VtxBuffer.reserve(draw_list->VtxBuffer.Size + (count * 4));
		}

		for (int n = clipper.DisplayStart; n != clipper.DisplayEnd; n++) {
			const int line_base = n * bytes_per_line;
			//
			// Draw the row address: try the name callback first; fall back to hex offset.
			//
			if (state->ShowAddress) {
				if (!state->GetAddressNameCallback || !state->GetAddressNameCallback(state, line_base, address_buf, address_max_chars))
					ImFormatString(address_buf, (size_t)address_max_chars, "%0.*zX", address_max_chars - 1, (size_t)line_base);

				const ImVec2 text_size = ImGui::CalcTextSize(address_buf);
				draw_list->AddText(cursor, text_color, address_buf);
				draw_list->AddText({cursor.x + text_size.x, cursor.y}, text_disabled_color, ":");
				cursor.x += address_max_size;
			}
			//
			// Read the bytes for this row into line_buf via callback or direct pointer.
			//
			int max_bytes_per_line = line_base;
			max_bytes_per_line = max_bytes_per_line > state->MaxBytes ? max_bytes_per_line - state->MaxBytes : bytes_per_line;

			int bytes_read;
			if (!state->ReadCallback) {
				memcpy(line_buf, (char*)state->Bytes + line_base, max_bytes_per_line);
				bytes_read = max_bytes_per_line;
			} else
				bytes_read = state->ReadCallback(state, line_base, line_buf, max_bytes_per_line);

			cursor.x += spacing.x * 0.5f;

			//
			// Per-byte inner loop: compute bounding boxes, read the byte value,
			// and build the two-character hex text string.
			//
			for (int i = 0; i != bytes_per_line; i++) {
				//
				// byte_bb is the tight glyph rect; item_bb expands into half the inter-item
				// spacing so that hover and click detection covers the full cell area.
				//
				const ImRect byte_bb = {{cursor.x, cursor.y}, {cursor.x + byte_size.x, cursor.y + byte_size.y}};

				ImRect item_bb = byte_bb;

				item_bb.Min.x -= spacing.x * 0.5f;

				if (n != clipper.DisplayStart)
					item_bb.Min.y -= spacing.y * 0.5f;

				item_bb.Max.x += spacing.x * 0.5f;
				item_bb.Max.y += spacing.y * 0.5f;

				const int offset = bytes_per_line * n + i;
				unsigned char byte;
				//
				// Compute the screen position of this byte's ASCII sidebar character.
				//
				ImVec2 byte_ascii = ascii_cursor;

				byte_ascii.x += (char_size.x * i) + spacing.x;
				byte_ascii.y += (char_size.y + spacing.y) * (n - clipper.DisplayStart);
				//
				// Read the byte and format it as two hex digits.
				// Out-of-range positions display "??" as a visual indicator.
				//
				char text[3];
				if (offset < state->MaxBytes && i < bytes_read) {
					byte = line_buf[i];

					text[0] = HalfByteToPrintable((byte & 0xf0) >> 4, lowercase_bytes);
					text[1] = HalfByteToPrintable(byte & 0x0f, lowercase_bytes);
					text[2] = '\0';
				} else {
					byte = 0x00;

					text[0] = '?';
					text[1] = '?';
					text[2] = '\0';
				}

				const ImGuiID id = ImGui::GetID(offset);

				if (!ImGui::ItemAdd(item_bb, id, 0, ImGuiItemFlags_Inputable))
					continue;

				//
				// Determine base text color; dim bytes that are out of range, zero, or unread.
				//
				ImColor byte_text_color = (offset >= state->MaxBytes || (state->RenderZeroesDisabled && byte == 0x00) || i >= bytes_read) ? text_disabled_color : text_color;
				//
				// Highlight priority: selection > SingleHighlightCallback > HighlightRanges.
				//
				if (offset >= select_start_byte && offset <= select_end_byte) {
					HighlightFlags flags = state->SelectionHighlightFlags;

					if (select_start_byte == select_end_byte) {
						flags &= ~HighlightFlags_FullSized;
					}

					ImRect bb = (flags & HighlightFlags_FullSized) ? item_bb : byte_bb;

					if (select_start_byte == select_end_byte) {
						if (select_start_subbyte)
							bb.Min.x = byte_bb.GetCenter().x;
						else
							bb.Max.x = byte_bb.GetCenter().x;
					}

					RenderByteDecorations(draw_list, bb, text_selected_bg_color, flags, border_color, style.FrameRounding, offset, select_start_byte, select_end_byte,
										  bytes_per_line, i, line_base);

					if (flags & HighlightFlags_Ascii) {
						RenderByteDecorations(draw_list, {byte_ascii, {byte_ascii.x + char_size.x, byte_ascii.y + char_size.y}}, text_selected_bg_color, flags, border_color,
											  style.FrameRounding, offset, offset, offset, bytes_per_line, i, line_base);
					}
				} else {
					bool single_highlight = false;

					if (state->SingleHighlightCallback) {
						ImColor color;
						ImColor custom_border_color;

						HighlightFlags flags = state->SingleHighlightCallback(state, offset, &color, &byte_text_color, &custom_border_color);

						if (flags & HighlightFlags_Apply) {
							ImColor highlight_border_color;

							if (flags & HighlightFlags_BorderAutomaticContrast)
								highlight_border_color = CalcContrastColor(color);
							else if (flags & HighlightFlags_OverrideBorderColor)
								highlight_border_color = custom_border_color;
							else
								highlight_border_color = border_color;

							single_highlight = true;

							RenderByteDecorations(draw_list, (flags & HighlightFlags_FullSized) ? item_bb : byte_bb, color, flags, highlight_border_color,
												  style.FrameRounding, offset, offset, offset, bytes_per_line, i, line_base);

							if (flags & HighlightFlags_Ascii) {
								RenderByteDecorations(draw_list, {byte_ascii, {byte_ascii.x + char_size.x, byte_ascii.y + char_size.y}}, color, flags, highlight_border_color,
													  style.FrameRounding, offset, offset, offset, bytes_per_line, i, line_base);
							}

							if (flags & HighlightFlags_TextAutomaticContrast)
								byte_text_color = CalcContrastColor(color);
						}
					}

					if (!single_highlight) {
						for (int j = 0; j != state->HighlightRanges.Size; j++) {
							HighlightRange& range = state->HighlightRanges[j];

							if (line_base + i >= range.From && line_base + i <= range.To) {
								ImColor highlight_border_color;

								if (range.Flags & HighlightFlags_BorderAutomaticContrast)
									highlight_border_color = CalcContrastColor(range.Color);
								else if (range.Flags & HighlightFlags_OverrideBorderColor)
									highlight_border_color = range.BorderColor;
								else
									highlight_border_color = border_color;

								RenderByteDecorations(draw_list, (range.Flags & HighlightFlags_FullSized) ? item_bb : byte_bb, range.Color, range.Flags,
													  highlight_border_color, style.FrameRounding, offset, range.From, range.To, bytes_per_line, i, line_base);

								if (range.Flags & HighlightFlags_Ascii) {
									RenderByteDecorations(draw_list, {byte_ascii, {byte_ascii.x + char_size.x, byte_ascii.y + char_size.y}}, range.Color, range.Flags,
														  highlight_border_color, style.FrameRounding, offset, range.From, range.To, bytes_per_line, i, line_base);
								}

								if (range.Flags & HighlightFlags_TextAutomaticContrast)
									byte_text_color = CalcContrastColor(range.Color);
							}
						}
					}
				}

				draw_list->AddText(byte_bb.Min, byte_text_color, text);
				//
				// Draw the blinking cursor underline beneath the active nibble.
				// The underline is one char-width wide and positioned under the
				// high nibble (subbyte=0) or the low nibble (subbyte=1).
				//
				if (offset == select_start_byte) {
					state->SelectCursorAnimationTime += io.DeltaTime;

					if (!io.ConfigInputTextCursorBlink || ImFmod(state->SelectCursorAnimationTime, 1.20f) <= 0.80f) {
						ImVec2 pos;
						pos.x = byte_bb.Min.x;
						pos.y = byte_bb.Max.y;

						if (select_start_subbyte)
							pos.x += char_size.x;

						draw_list->AddLine({pos.x, pos.y}, {pos.x + char_size.x, pos.y}, text_color);
					}
				}

				//
				// Mouse interaction: click starts a new selection; holding and dragging
				// extends it. Releasing the button clears the drag anchor.
				//
				const bool hovered = ImGui::ItemHoverable(item_bb, id, ImGuiItemFlags_Inputable);

				if (select_drag_byte != -1 && offset == select_drag_byte && !mouse_left_down) {
					next_select_drag_byte = -1;
				} else {
					if (hovered) {
						const bool clicked = ImGui::IsItemClicked();

						if (clicked) {
							next_select_start_byte = offset;
							next_select_end_byte = offset;
							next_select_drag_byte = offset;
							next_select_drag_subbyte = mouse_pos.x > byte_bb.GetCenter().x;
							next_select_start_subbyte = next_select_drag_subbyte;
							next_last_selected_byte = offset;

							ImGui::SetKeyboardFocusHere();
						} else if (mouse_left_down && select_drag_byte != -1) {
							if (offset >= select_drag_byte) {
								next_select_end_byte = offset;
							} else {
								next_select_start_byte = offset;
								next_select_end_byte = select_drag_byte;
								next_select_start_subbyte = 0;
							}

							ImGui::SetKeyboardFocusHere();
						}
					}
				}

				if (hovered && offset < state->MaxBytes && i < bytes_read) {
					//
					// Show a tooltip with HEX/DEC/BIN/CHR representations of the hovered byte.
					// Build binary string in two nibble groups: "XXXX XXXX".
					//
					char bin_str[10];
					for (int b = 7; b >= 4; b--)
						bin_str[7 - b] = ((byte >> b) & 1) ? '1' : '0';
					bin_str[4] = ' ';
					for (int b = 3; b >= 0; b--)
						bin_str[8 - b] = ((byte >> b) & 1) ? '1' : '0';
					bin_str[9] = '\0';
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
					ImGui::BeginTooltip();
					ImGui::Text("HEX  0x%02X", (unsigned int)byte);
					ImGui::Text("DEC  %u", (unsigned int)byte);
					ImGui::Text("BIN  %s", bin_str);
					if (HasAsciiRepresentation(byte))
						ImGui::Text("CHR  '%c'", (char)byte);
					else
						ImGui::TextDisabled("CHR  ---");
					ImGui::EndTooltip();
					ImGui::PopStyleVar();
				}

				if (offset == next_last_selected_byte && last_selected_byte != next_last_selected_byte) {
					ImGui::SetKeyboardFocusHere();
				}

				//
								// Keyboard byte edit: apply the pressed hex key to the high or low nibble
								// of the selected byte, then advance the cursor by one nibble (or one byte
								// when the low nibble was just written).
								//
								if (offset == last_selected_byte && !state->ReadOnly && hex_key_pressed != ImGuiKey_None) {
									IM_ASSERT(offset == select_start_byte || offset == select_end_byte);
									const int subbyte = offset == select_start_byte ? select_start_subbyte : select_end_subbyte;

					unsigned char wbyte;
					if (subbyte)
						wbyte = (byte & 0xf0) | KeyToHalfByte(hex_key_pressed);
					else
						wbyte = (KeyToHalfByte(hex_key_pressed) << 4) | (byte & 0x0f);

					if (!state->WriteCallback)
						*(unsigned char*)((char*)state->Bytes + n * bytes_per_line + i) = wbyte;
					else
						state->WriteCallback(state, n * bytes_per_line + i, &wbyte, sizeof(wbyte));

					int* next_subbyte = (int*)(offset == select_start_byte ? &next_select_start_subbyte : &next_select_end_subbyte);
					if (!subbyte) {
						next_select_start_byte = offset;
						next_select_end_byte = offset;
						*next_subbyte = 1;
					} else {
						next_last_selected_byte = offset + 1;
						if (next_last_selected_byte >= state->MaxBytes - 1)
							next_last_selected_byte = state->MaxBytes - 1;
						else
							*next_subbyte = 0;

						next_select_start_byte = next_last_selected_byte;
						next_select_end_byte = next_last_selected_byte;
					}

					state->SelectCursorAnimationTime = 0.f;
				}

				//
				// Advance cursor; insert an extra spacing gap at every separator boundary
				// except after the very last byte in the row.
				//
				cursor.x += byte_size.x + spacing.x;
				if (i > 0 && state->Separators > 0 && (i + 1) % state->Separators == 0 && i != bytes_per_line - 1)
					cursor.x += spacing.x;
				//
				// Draw the ASCII sidebar character: printable bytes show their glyph,
				// everything else shows a '.' placeholder.
				//
				if (show_ascii) {
					unsigned char byte;
					if (offset < state->MaxBytes)
						byte = line_buf[i];
					else
						byte = 0x00;

					bool has_ascii = HasAsciiRepresentation(byte);

					const ImRect char_bb = {byte_ascii, {byte_ascii.x + char_size.x, byte_ascii.y + char_size.y}};

					/*if (offset >= select_start_byte && offset <= select_end_byte)
					{
						draw_list->AddRectFilled(char_bb.Min, char_bb.Max, text_selected_bg_color);
					}*/

					char text[2];
					text[0] = has_ascii ? *(char*)&byte : '.';
					text[1] = '\0';

					draw_list->AddText(byte_ascii, byte_text_color, text);
				}

				ImGui::SetCursorScreenPos(cursor);
			}

			ImGui::NewLine();
			cursor = ImGui::GetCursorScreenPos();
		}
	}

	//
	// Flush the updated selection state back into the persistent State struct
	// so changes are visible to the caller on the next frame.
	//
	state->SelectStartByte = next_select_start_byte;
	state->SelectStartSubByte = next_select_start_subbyte;
	state->SelectEndByte = next_select_end_byte;
	state->SelectEndSubByte = next_select_end_subbyte;
	state->LastSelectedByte = next_last_selected_byte;
	state->SelectDragByte = next_select_drag_byte;
	state->SelectDragSubByte = next_select_drag_subbyte;
	//
	// Release heap buffers if they were allocated (stack buffers are left alone).
	//
	if (line_buf != stack_line_buf)
		ImGui::MemFree(line_buf);

	if (address_buf != stack_address_buf)
		ImGui::MemFree(address_buf);

	return true;
}

//
// Close the hex editor child window opened by Begin().
//
void End() {
	ImGui::EndChild();
}
//
// Helper used by callers that maintain their own HighlightRanges: given a display
// row starting at row_offset, compute the row-local byte indices [out_min, out_max]
// of the intersection with the absolute range [range_min, range_max].
// Returns false when the range does not touch this row at all.
//
bool CalcRowRange(int row_offset, int row_bytes_count, int range_min, int range_max, int* out_min, int* out_max) {
	int abs_min;
	int abs_max;

	if (RangeRangeIntersection(row_offset, row_offset + row_bytes_count, range_min, range_max, &abs_min, &abs_max)) {
		*out_min = abs_min - row_offset;
		*out_max = abs_max - row_offset;
		return true;
	}

	return false;
}

} // namespace ImGui::HexEditor