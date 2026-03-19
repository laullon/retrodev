//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#include "conversion.widget.h"
#include <app/app.console.h>
#include <convert/converters.h>

namespace RetrodevGui {

	// Static member definitions
	bool ConversionWidget::m_targetOpen = true;
	bool ConversionWidget::m_resizeOpen = true;
	bool ConversionWidget::m_quantizationOpen = true;
	bool ConversionWidget::m_ditheringOpen = true;
	bool ConversionWidget::m_colorCorrectionOpen = true;

	//
	// Main render function for the conversion widget, called from within an existing child window
	//
	ConversionWidgetResult ConversionWidget::Render(RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IBitmapConverter> converter, const std::string& buildItemName,
													RetrodevLib::ProjectBuildType buildType) {
		ConversionWidgetResult result;
		if (!params)
			return result;
		bool changed = false;
		//
		// Target Conversion section
		//
		ImGui::SetNextItemOpen(m_targetOpen, ImGuiCond_Once);
		if (ImGui::CollapsingHeader("Target Conversion")) {
			m_targetOpen = true;
			auto targetResult = RenderTargetConversion(params, converter, buildItemName, buildType);
			changed |= targetResult.parametersChanged;
			if (targetResult.itemRenamed) {
				result.itemRenamed = true;
				result.newName = targetResult.newName;
			}
		} else {
			m_targetOpen = false;
		}
		//
		// Resize section
		//
		ImGui::SetNextItemOpen(m_resizeOpen, ImGuiCond_Once);
		if (ImGui::CollapsingHeader("Resize")) {
			m_resizeOpen = true;
			changed |= RenderResize(params, converter);
		} else {
			m_resizeOpen = false;
		}
		//
		// Quantization section
		//
		ImGui::SetNextItemOpen(m_quantizationOpen, ImGuiCond_Once);
		if (ImGui::CollapsingHeader("Quantization")) {
			m_quantizationOpen = true;
			changed |= RenderQuantization(params, converter);
		} else {
			m_quantizationOpen = false;
		}
		//
		// Dithering section
		//
		ImGui::SetNextItemOpen(m_ditheringOpen, ImGuiCond_Once);
		if (ImGui::CollapsingHeader("Dithering")) {
			m_ditheringOpen = true;
			changed |= RenderDithering(params);
		} else {
			m_ditheringOpen = false;
		}
		//
		// Color Correction section
		//
		ImGui::SetNextItemOpen(m_colorCorrectionOpen, ImGuiCond_Once);
		if (ImGui::CollapsingHeader("Color Correction")) {
			m_colorCorrectionOpen = true;
			changed |= RenderColorCorrection(params);
		} else {
			m_colorCorrectionOpen = false;
		}
		result.parametersChanged = changed;
		return result;
	}

	ConversionWidgetResult ConversionWidget::RenderTargetConversion(RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IBitmapConverter> converter,
																	const std::string& buildItemName, RetrodevLib::ProjectBuildType buildType) {
		ConversionWidgetResult result;
		bool changed = false;
		//
		// Build item name (editable for renaming)
		//
		static char nameBuffer[256] = "";
		//
		// Initialize buffer with current name if empty or different
		//
		if (nameBuffer[0] == '\0' || buildItemName != std::string(nameBuffer)) {
			strncpy_s(nameBuffer, buildItemName.c_str(), sizeof(nameBuffer) - 1);
			nameBuffer[sizeof(nameBuffer) - 1] = '\0';
		}
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Build Item Name:");
		ImGui::SameLine();
		if (ImGui::InputText("##buildItemName", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
			//
			// User pressed Enter, try to rename the build item
			//
			std::string newName(nameBuffer);
			if (!newName.empty() && newName != buildItemName) {
				if (RetrodevLib::Project::RenameBuildItem(buildType, buildItemName, newName)) {
					AppConsole::AddLogF(AppConsole::LogLevel::Info, "Renamed build item '%s' to '%s'", buildItemName.c_str(), newName.c_str());
					result.itemRenamed = true;
					result.newName = newName;
				} else {
					AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed to rename build item '%s' to '%s' (name may already exist)", buildItemName.c_str(), newName.c_str());
					//
					// Revert buffer to original name
					//
					strncpy_s(nameBuffer, buildItemName.c_str(), sizeof(nameBuffer) - 1);
					nameBuffer[sizeof(nameBuffer) - 1] = '\0';
				}
			}
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Press Enter to rename the build item");
		}
		//
		// Display estimated size (informational, read-only)
		//
		if (converter) {
			int estimatedSize = converter->GetEstimatedSize(params);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Estimated Size:");
			ImGui::SameLine();
			//
			// Format size with appropriate units (bytes, KB)
			//
			if (estimatedSize < 1024) {
				ImGui::Text("%d bytes", estimatedSize);
			} else {
				float sizeKB = estimatedSize / 1024.0f;
				ImGui::Text("%.2f KB (%d bytes)", sizeKB, estimatedSize);
			}
		}
		ImGui::Separator();
		//
		// Target System selection
		//
		std::vector<std::string> systems = RetrodevLib::Converters::Get();
		if (params->SParams.TargetSystem.empty() && !systems.empty()) {
			//
			// If no system is set, default to first one
			//
			params->SParams.TargetSystem = systems[0];
			changed = true;
		}
		if (ImGui::BeginCombo("Target System", params->SParams.TargetSystem.c_str())) {
			for (const auto& systemName : systems) {
				bool isSelected = (params->SParams.TargetSystem == systemName);
				if (ImGui::Selectable(systemName.c_str(), isSelected)) {
					params->SParams.TargetSystem = systemName;
					changed = true;
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		//
		// Target Mode selection (from converter if available)
		//
		if (converter) {
			std::vector<std::string> modes = converter->GetTargetModes();
			if (!modes.empty()) {
				//
				// Find current mode or default to first
				//
				bool modeFound = false;
				for (const auto& mode : modes) {
					if (mode == params->SParams.TargetMode) {
						modeFound = true;
						break;
					}
				}
				if (!modeFound) {
					params->SParams.TargetMode = modes[0];
					changed = true;
				}
				if (ImGui::BeginCombo("Target Mode", params->SParams.TargetMode.c_str())) {
					for (const auto& mode : modes) {
						bool isSelected = (params->SParams.TargetMode == mode);
						if (ImGui::Selectable(mode.c_str(), isSelected)) {
							params->SParams.TargetMode = mode;
							changed = true;
						}
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}
			std::vector<std::string> paletteTypes = converter->GetTargetPalettes();
			if (!paletteTypes.empty()) {
				//
				// Find current palette type or default to first
				//
				bool paletteFound = false;
				for (const auto& palette : paletteTypes) {
					if (palette == params->SParams.PaletteType) {
						paletteFound = true;
						break;
					}
				}
				if (!paletteFound) {
					params->SParams.PaletteType = paletteTypes[0];
					changed = true;
				}
				if (ImGui::BeginCombo("Target Palette", params->SParams.PaletteType.c_str())) {
					for (const auto& palette : paletteTypes) {
						bool isSelected = (params->SParams.PaletteType == palette);
						if (ImGui::Selectable(palette.c_str(), isSelected)) {
							params->SParams.PaletteType = palette;
							changed = true;
						}
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}
		}
		result.parametersChanged = changed;
		return result;
	}

	bool ConversionWidget::RenderResize(RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IBitmapConverter> converter) {
		bool changed = false;
		//
		// Scale Mode combo
		//
		std::vector<std::string> resizeModes = RetrodevLib::GFXResize::GetScaleModes();
		int currentResizeMode = static_cast<int>(params->RParams.ResMode);
		if (ImGui::BeginCombo("Scale Mode", resizeModes[currentResizeMode].c_str())) {
			for (int i = 0; i < (int)resizeModes.size(); i++) {
				bool isSelected = (currentResizeMode == i);
				if (ImGui::Selectable(resizeModes[i].c_str(), isSelected)) {
					params->RParams.ResMode = static_cast<RetrodevLib::ResizeParams::ScaleMode>(i);
					changed = true;
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		//
		// Source Rectangle controls (only enabled when scale mode is Custom)
		//
		bool isCustomMode = (params->RParams.ResMode == RetrodevLib::ResizeParams::ScaleMode::Custom);
		//
		// Source rectangle with original image dimensions in any mode if not custom one
		//
		if (!isCustomMode) {
			std::shared_ptr<RetrodevLib::Image> originalBitmap = converter->GetOriginal();
			params->RParams.SourceRect.X = 0;
			params->RParams.SourceRect.Y = 0;
			params->RParams.SourceRect.Width = originalBitmap->GetWidth();
			params->RParams.SourceRect.Height = originalBitmap->GetHeight();
		}
		ImGui::Indent();
		ImGui::BeginDisabled(!isCustomMode);
		if (ImGui::InputInt("Source X", &params->RParams.SourceRect.X))
			changed = true;
		if (ImGui::InputInt("Source Y", &params->RParams.SourceRect.Y))
			changed = true;
		if (ImGui::InputInt("Source Width", &params->RParams.SourceRect.Width)) {
			//
			// Ensure width is not negative
			//
			if (params->RParams.SourceRect.Width < 0)
				params->RParams.SourceRect.Width = 0;
			changed = true;
		}
		if (ImGui::InputInt("Source Height", &params->RParams.SourceRect.Height)) {
			//
			// Ensure height is not negative
			//
			if (params->RParams.SourceRect.Height < 0)
				params->RParams.SourceRect.Height = 0;
			changed = true;
		}
		ImGui::EndDisabled();
		ImGui::Unindent();
		//
		// Interpolation mode
		//
		std::vector<std::string> interpModes = RetrodevLib::GFXResize::GetInterpolationModes();
		int currentInterpMode = static_cast<int>(params->RParams.InterpMode);
		if (ImGui::BeginCombo("Interpolation", interpModes[currentInterpMode].c_str())) {
			for (int i = 0; i < (int)interpModes.size(); i++) {
				bool isSelected = (currentInterpMode == i);
				if (ImGui::Selectable(interpModes[i].c_str(), isSelected)) {
					params->RParams.InterpMode = static_cast<RetrodevLib::ResizeParams::InterpolationMode>(i);
					changed = true;
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		if (converter) {
			//
			// Target Resolution selection (from converter if available)
			//
			std::vector<std::string> resolutions = converter->GetTargetResolutions();
			if (!resolutions.empty()) {
				//
				// Find current resolution index from stored TargetResolution string
				// If not found or empty, default to first resolution
				//
				int selectedResolution = 0;
				if (!params->RParams.TargetResolution.empty()) {
					for (int i = 0; i < (int)resolutions.size(); i++) {
						if (resolutions[i] == params->RParams.TargetResolution) {
							selectedResolution = i;
							break;
						}
					}
				} else {
					//
					// Initialize with first resolution if not set
					//
					params->RParams.TargetResolution = resolutions[0];
				}
				//
				// Initialize target dimensions if empty with the current selected resolution
				//
				if (params->RParams.TargetWidth == 0 || params->RParams.TargetHeight == 0) {
					RetrodevLib::Image::Size res = converter->GetTargetResolution(resolutions[selectedResolution], params);
					params->RParams.TargetWidth = res.Width;
					params->RParams.TargetHeight = res.Height;
					changed = true;
				}
				if (ImGui::BeginCombo("Target Resolution", resolutions[selectedResolution].c_str())) {
					for (int i = 0; i < (int)resolutions.size(); i++) {
						bool isSelected = (selectedResolution == i);
						if (ImGui::Selectable(resolutions[i].c_str(), isSelected)) {
							selectedResolution = i;
							//
							// Store the selected resolution name
							//
							params->RParams.TargetResolution = resolutions[i];
							changed = true;
						}
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				//
				// Target dimensions (only enabled when Custom resolution is selected)
				//
				bool isCustomResolution = (resolutions[selectedResolution] == "Custom");
				bool isOriginalResolution = (resolutions[selectedResolution] == "Original");
				if (isOriginalResolution) {
					//
					// Update target dimensions using original size
					//
					std::shared_ptr<RetrodevLib::Image> originalBitmap = converter->GetOriginal();
					params->RParams.TargetWidth = originalBitmap->GetWidth();
					params->RParams.TargetHeight = originalBitmap->GetHeight();
				}
				if (!isCustomResolution && !isOriginalResolution) {
					//
					// Update target dimensions with prefixed resolution from converter
					//
					RetrodevLib::Image::Size res = converter->GetTargetResolution(resolutions[selectedResolution], params);
					params->RParams.TargetWidth = res.Width;
					params->RParams.TargetHeight = res.Height;
				}
				ImGui::Indent();
				ImGui::BeginDisabled(!isCustomResolution);
				if (ImGui::Button("x2##Width")) {
					//
					// Multiply width by 2
					//
					params->RParams.TargetWidth *= 2;
					changed = true;
				}
				ImGui::SameLine(0, 0);
				if (ImGui::Button("/2##Width")) {
					//
					// Divide width by 2
					//
					params->RParams.TargetWidth /= 2;
					if (params->RParams.TargetWidth < 1)
						params->RParams.TargetWidth = 1;
					changed = true;
				}
				ImGui::SameLine();
				if (ImGui::InputInt("Width", &params->RParams.TargetWidth)) {
					//
					// Ensure width is not negative
					//
					if (params->RParams.TargetWidth < 0)
						params->RParams.TargetWidth = 0;
					changed = true;
				}
				if (ImGui::Button("x2##Height")) {
					//
					// Multiply height by 2
					//
					params->RParams.TargetHeight *= 2;
					changed = true;
				}
				ImGui::SameLine(0, 0);
				if (ImGui::Button("/2##Height")) {
					//
					// Divide height by 2
					//
					params->RParams.TargetHeight /= 2;
					if (params->RParams.TargetHeight < 1)
						params->RParams.TargetHeight = 1;
					changed = true;
				}
				ImGui::SameLine();
				if (ImGui::InputInt("Height", &params->RParams.TargetHeight)) {
					//
					// Ensure height is not negative
					//
					if (params->RParams.TargetHeight < 0)
						params->RParams.TargetHeight = 0;
					changed = true;
				}
				ImGui::EndDisabled();
				ImGui::Unindent();
			}
		}
		return changed;
	}

	bool ConversionWidget::RenderQuantization(RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IBitmapConverter> converter) {
		bool changed = false;
		//
		// Use Source Palette: only meaningful when the source image is paletized
		//
		bool sourcePaletized = false;
		if (converter) {
			auto original = converter->GetOriginal();
			if (original)
				sourcePaletized = original->IsPaletized();
		}
		if (!sourcePaletized)
			ImGui::BeginDisabled();
		changed |= ImGui::Checkbox("Use Source Palette", &params->QParams.UseSourcePalette);
		if (!sourcePaletized)
			ImGui::EndDisabled();
		//
		// Smoothness
		//
		changed |= ImGui::Checkbox("Smoothness", &params->QParams.Smoothness);
		//
		// Color selection mode (from palette if converter available)
		//
		if (converter) {
			std::shared_ptr<RetrodevLib::IPaletteConverter> palette = converter->GetPalette();
			if (palette) {
				std::vector<std::string> colorModes = palette->GetColorSelectionModes();
				if (!colorModes.empty()) {
					//
					// Find current mode or default to first
					//
					bool modeFound = false;
					for (const auto& mode : colorModes) {
						if (mode == params->SParams.ColorSelectionMode) {
							modeFound = true;
							break;
						}
					}
					if (!modeFound) {
						params->SParams.ColorSelectionMode = colorModes[0];
						changed = true;
					}
					if (ImGui::BeginCombo("Color Selection Mode", params->SParams.ColorSelectionMode.c_str())) {
						for (const auto& mode : colorModes) {
							bool isSelected = (params->SParams.ColorSelectionMode == mode);
							if (ImGui::Selectable(mode.c_str(), isSelected)) {
								params->SParams.ColorSelectionMode = mode;
								changed = true;
							}
							if (isSelected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
				}
			}
		}
		//
		// Sort palette
		//
		changed |= ImGui::Checkbox("Sort Palette", &params->QParams.SortPalette);
		//
		// Reduction method
		//
		const char* reductionMethods[] = {"Higher Frequencies", "Higher Distances"};
		int reductionMethod = static_cast<int>(params->QParams.ReductionType);
		if (ImGui::Combo("Reduction Method", &reductionMethod, reductionMethods, IM_ARRAYSIZE(reductionMethods))) {
			params->QParams.ReductionType = static_cast<RetrodevLib::QuantizationParams::ReductionMethod>(reductionMethod);
			changed = true;
		}
		//
		// Reduction time
		//
		changed |= ImGui::Checkbox("Reduction Before Dithering", &params->QParams.ReductionTime);
		return changed;
	}

	bool ConversionWidget::RenderDithering(RetrodevLib::GFXParams* params) {
		bool changed = false;
		//
		// Dithering method (combo box)
		//
		std::vector<std::string> ditheringMethods = RetrodevLib::GFXDithering::GetDitheringMethods();
		if (!ditheringMethods.empty()) {
			//
			// Find current method or default to first
			//
			bool methodFound = false;
			for (const auto& method : ditheringMethods) {
				if (method == params->DParams.Method) {
					methodFound = true;
					break;
				}
			}
			if (!methodFound) {
				params->DParams.Method = ditheringMethods[0];
				changed = true;
			}
			if (ImGui::BeginCombo("Method", params->DParams.Method.c_str())) {
				for (const auto& method : ditheringMethods) {
					bool isSelected = (params->DParams.Method == method);
					if (ImGui::Selectable(method.c_str(), isSelected)) {
						params->DParams.Method = method;
						changed = true;
					}
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		//
		// Dithering percentage
		//
		changed |= ImGui::SliderInt("Percentage", &params->DParams.Percentage, 0, 400);
		//
		// Error diffusion
		//
		changed |= ImGui::Checkbox("Error Diffusion", &params->DParams.ErrorDiffusion);
		//
		// Pattern dithering
		//
		changed |= ImGui::Checkbox("Pattern", &params->DParams.Pattern);
		if (params->DParams.Pattern) {
			changed |= ImGui::SliderFloat("Pattern Low", &params->DParams.PatternLow, 1.0f, 4.0f);
			changed |= ImGui::SliderFloat("Pattern High", &params->DParams.PatternHigh, 1.0f, 4.0f);
		}
		return changed;
	}

	bool ConversionWidget::RenderColorCorrection(RetrodevLib::GFXParams* params) {
		bool changed = false;
		//
		// Color correction per channel
		//
		changed |= ImGui::SliderInt("Red Correction", &params->CParams.ColorCorrectionRed, -255, 255);
		changed |= ImGui::SliderInt("Green Correction", &params->CParams.ColorCorrectionGreen, -255, 255);
		changed |= ImGui::SliderInt("Blue Correction", &params->CParams.ColorCorrectionBlue, -255, 255);
		//
		// Contrast
		//
		changed |= ImGui::SliderInt("Contrast", &params->CParams.Contrast, -100, 100);
		//
		// Saturation
		//
		changed |= ImGui::SliderInt("Saturation", &params->CParams.Saturation, -100, 100);
		//
		// Brightness
		//
		changed |= ImGui::SliderInt("Brightness", &params->CParams.Brightness, -100, 100);
		//
		// Color bits
		//
		const char* colorBits[] = {"24 bits", "12 bits", "9 bits", "6 bits"};
		int colorBitsIndex = 0;
		if (params->CParams.ColorBits == 24)
			colorBitsIndex = 0;
		else if (params->CParams.ColorBits == 12)
			colorBitsIndex = 1;
		else if (params->CParams.ColorBits == 9)
			colorBitsIndex = 2;
		else if (params->CParams.ColorBits == 6)
			colorBitsIndex = 3;
		if (ImGui::Combo("Color Bits", &colorBitsIndex, colorBits, IM_ARRAYSIZE(colorBits))) {
			if (colorBitsIndex == 0)
				params->CParams.ColorBits = 24;
			else if (colorBitsIndex == 1)
				params->CParams.ColorBits = 12;
			else if (colorBitsIndex == 2)
				params->CParams.ColorBits = 9;
			else if (colorBitsIndex == 3)
				params->CParams.ColorBits = 6;
			changed = true;
		}
		//
		// Palette reduction limits (combo boxes)
		//
		std::vector<std::string> lowerLimits = RetrodevLib::GFXColor::GetPaletteReductionLowerLimits();
		if (!lowerLimits.empty()) {
			//
			// Find current lower limit or default to first
			//
			bool lowerFound = false;
			for (const auto& limit : lowerLimits) {
				if (limit == params->CParams.PaletteReductionLower) {
					lowerFound = true;
					break;
				}
			}
			if (!lowerFound) {
				params->CParams.PaletteReductionLower = lowerLimits[0];
				changed = true;
			}
			if (ImGui::BeginCombo("Lower Limit", params->CParams.PaletteReductionLower.c_str())) {
				for (const auto& limit : lowerLimits) {
					bool isSelected = (params->CParams.PaletteReductionLower == limit);
					if (ImGui::Selectable(limit.c_str(), isSelected)) {
						params->CParams.PaletteReductionLower = limit;
						changed = true;
					}
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		std::vector<std::string> upperLimits = RetrodevLib::GFXColor::GetPaletteReductionUpperLimits();
		if (!upperLimits.empty()) {
			//
			// Find current upper limit or default to first
			//
			bool upperFound = false;
			for (const auto& limit : upperLimits) {
				if (limit == params->CParams.PaletteReductionUpper) {
					upperFound = true;
					break;
				}
			}
			if (!upperFound) {
				params->CParams.PaletteReductionUpper = upperLimits[0];
				changed = true;
			}
			if (ImGui::BeginCombo("Upper Limit", params->CParams.PaletteReductionUpper.c_str())) {
				for (const auto& limit : upperLimits) {
					bool isSelected = (params->CParams.PaletteReductionUpper == limit);
					if (ImGui::Selectable(limit.c_str(), isSelected)) {
						params->CParams.PaletteReductionUpper = limit;
						changed = true;
					}
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		//
		// Reset button
		//
		if (ImGui::Button("Reset to Defaults")) {
			params->CParams.ColorCorrectionRed = 100;
			params->CParams.ColorCorrectionGreen = 100;
			params->CParams.ColorCorrectionBlue = 100;
			params->CParams.Contrast = 100;
			params->CParams.Saturation = 100;
			params->CParams.Brightness = 100;
			changed = true;
		}
		return changed;
	}

} // namespace RetrodevGui
