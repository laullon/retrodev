//----------------------------------------------------------------------------------------------------
//
//
//
//
// https://github.com/Teselka/imgui_hex_editor
//----------------------------------------------------------------------------------------------------

#pragma once
#include <imgui.h>

namespace ImGui::HexEditor {

	//
	// Flags controlling per-byte and range highlight rendering behaviour.
	//
	enum HighlightFlags_ : int {
		HighlightFlags_None = 0,
		HighlightFlags_Apply = 1 << 0,
		HighlightFlags_TextAutomaticContrast = 1 << 1,
		HighlightFlags_FullSized = 1 << 2, // Highlight entire byte space including its container, has no effect on ascii
		HighlightFlags_Ascii = 1 << 3,     // Highlight ascii (doesn't affect single byte highlighting)
		HighlightFlags_Border = 1 << 4,
		HighlightFlags_OverrideBorderColor = 1 << 5,
		HighlightFlags_BorderAutomaticContrast = 1 << 6,
	};
	//
	// Bitmask type for HighlightFlags_ values passed to callbacks and stored in State.
	//
	typedef int HighlightFlags;

	//
	// Describes a contiguous byte range that should be highlighted with a given color and flags.
	//
	struct HighlightRange {
		int From;
		int To;
		ImColor Color;
		ImColor BorderColor;
		HighlightFlags Flags;
	};

	//
	// Flags controlling clipboard copy formatting.
	//
	enum ClipboardFlags_ : int {
		ClipboardFlags_None = 0,
		ClipboardFlags_Multiline = 1 << 0, // Separate resulting hex editor lines with carriage return
	};
	//
	// Bitmask type for ClipboardFlags_ values stored in State.
	//
	typedef int ClipboardFlags;

	//
	// All configuration and runtime state for a hex editor instance.
	// Pass a pointer to BeginHexEditor each frame.
	//
	struct State {
		//
		// Data source — pointer to raw bytes (used when ReadCallback is null).
		//
		void* Bytes;
		//
		// Total number of bytes in the data source.
		//
		int MaxBytes;
		//
		// Fixed bytes per display row; -1 = auto-calculate from available width.
		//
		int BytesPerLine = -1;
		//
		// Show printable ASCII characters in a sidebar column.
		//
		bool ShowPrintable = false;
		//
		// Render hex digits in lowercase (a–f) instead of uppercase (A–F).
		//
		bool LowercaseBytes = false;
		//
		// Render zero bytes dimmed/disabled instead of with full text colour.
		//
		bool RenderZeroesDisabled = true;
		//
		// Show the address / offset column on the left.
		//
		bool ShowAddress = true;
		//
		// Width of the address column in characters; -1 = auto-calculate from MaxBytes.
		//
		int AddressChars = -1;
		//
		// Show the ASCII sidebar column on the right.
		//
		bool ShowAscii = true;
		//
		// When true the editor renders read-only: no byte editing via keyboard.
		//
		bool ReadOnly = false;
		//
		// Insert an extra spacing gap every N bytes (0 = no separators).
		//
		int Separators = 8;
		//
		// Caller-defined context pointer; available inside all callbacks via state->UserData.
		//
		void* UserData = nullptr;
		//
		// Static highlight ranges rendered each frame (added, cleared, or updated by the caller).
		//
		ImVector<HighlightRange> HighlightRanges;
		//
		// Enable Ctrl+C clipboard copy of the current selection.
		//
		bool EnableClipboard = true;
		//
		// Controls how the copied text is formatted (e.g. multiline line breaks).
		//
		ClipboardFlags ClipboardFlags = ClipboardFlags_Multiline;

		//
		// Optional callback to read bytes from a custom source.
		// Must copy `size` bytes starting at `offset` into `buf` and return the number of bytes read.
		// When null, Bytes pointer is used directly.
		//
		int (*ReadCallback)(State* state, int offset, void* buf, int size) = nullptr;
		//
		// Optional callback invoked when the user edits a byte.
		// Must write `size` bytes from `buf` at `offset` and return the number of bytes written.
		// When null, Bytes pointer is written directly.
		//
		int (*WriteCallback)(State* state, int offset, void* buf, int size) = nullptr;
		//
		// Optional callback to supply a human-readable label for a given row address.
		// Write the label into `buf` (max `size` chars) and return true to use it,
		// or return false to fall back to the default hex address format.
		//
		bool (*GetAddressNameCallback)(State* state, int offset, char* buf, int size) = nullptr;
		//
		// Optional per-byte highlight callback.
		// Fill `color`, `text_color`, and `border_color` and return flags with HighlightFlags_Apply set
		// to override the rendering of the byte at `offset`.
		//
		HighlightFlags (*SingleHighlightCallback)(State* state, int offset, ImColor* color, ImColor* text_color, ImColor* border_color) = nullptr;
		//
		// Optional callback invoked once per frame before range highlighting.
		// Use it to update HighlightRanges based on the currently visible byte window.
		//
		void (*HighlightRangesCallback)(State* state, int display_start, int display_end) = nullptr;

		//
		// Selection state — maintained by the editor each frame.
		//
		int SelectStartByte = -1;
		int SelectStartSubByte = 0;
		int SelectEndByte = -1;
		int SelectEndSubByte = 0;
		int LastSelectedByte = -1;
		int SelectDragByte = -1;
		int SelectDragSubByte = 0;
		float SelectCursorAnimationTime = 0.f;
		//
		// Flags applied to the current selection highlight (FullSized + Ascii by default).
		//
		HighlightFlags SelectionHighlightFlags = HighlightFlags_FullSized | HighlightFlags_Ascii;
	};

	//
	// Begin rendering a hex editor child window.
	// Call EndHexEditor() unconditionally after this, mirroring BeginChild/EndChild semantics.
	//
	bool Begin(const char* str_id, State* state, const ImVec2& size = {0.f, 0.f}, ImGuiChildFlags child_flags = 0, ImGuiWindowFlags window_flags = 0);
	//
	// End the hex editor child window started by Begin().
	//
	void End();

	//
	// Helper: given a display row starting at row_offset spanning row_bytes_count bytes,
	// compute the intersection with the highlight range [range_min, range_max].
	// Writes row-local indices into out_min/out_max and returns true when there is an intersection.
	//
	bool CalcRowRange(int row_offset, int row_bytes_count, int range_min, int range_max, int* out_min, int* out_max);

} // namespace ImGui::HexEditor

//
// Backward-compatible aliases so existing call sites need no changes.
//
using ImGuiHexEditorHighlightFlags_ = ImGui::HexEditor::HighlightFlags_;
using ImGuiHexEditorHighlightFlags = ImGui::HexEditor::HighlightFlags;
using ImGuiHexEditorHighlightRange = ImGui::HexEditor::HighlightRange;
using ImGuiHexEditorClipboardFlags_ = ImGui::HexEditor::ClipboardFlags_;
using ImGuiHexEditorClipboardFlags = ImGui::HexEditor::ClipboardFlags;
using ImGuiHexEditorState = ImGui::HexEditor::State;
//
// Inline wrappers so ImGui::BeginHexEditor / ImGui::EndHexEditor / ImGui::CalcHexEditorRowRange
// continue to work unchanged at all call sites.
//
namespace ImGui {
	//
	// Backward-compatible wrapper for ImGui::HexEditor::Begin().
	// Begin rendering a hex editor child window. Call EndHexEditor() unconditionally after this,
	// mirroring BeginChild/EndChild semantics.
	//
	inline bool BeginHexEditor(const char* str_id, HexEditor::State* state, const ImVec2& size = {0.f, 0.f}, ImGuiChildFlags child_flags = 0, ImGuiWindowFlags window_flags = 0) {
		return HexEditor::Begin(str_id, state, size, child_flags, window_flags);
	}
	//
	// Backward-compatible wrapper for ImGui::HexEditor::End().
	// End the hex editor child window started by BeginHexEditor().
	//
	inline void EndHexEditor() {
		HexEditor::End();
	}
	//
	// Backward-compatible wrapper for ImGui::HexEditor::CalcRowRange().
	// Given a display row starting at row_offset spanning row_bytes_count bytes,
	// compute the intersection with the highlight range [range_min, range_max].
	// Writes row-local indices into out_min/out_max and returns true when there is an intersection.
	//
	inline bool CalcHexEditorRowRange(int row_offset, int row_bytes_count, int range_min, int range_max, int* out_min, int* out_max) {
		return HexEditor::CalcRowRange(row_offset, row_bytes_count, range_min, range_max, out_min, out_max);
	}
}