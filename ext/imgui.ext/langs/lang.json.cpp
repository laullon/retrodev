//----------------------------------------------------------------------------------------------------
//
//
//
//
//
//----------------------------------------------------------------------------------------------------

#include "../imgui.text.editor.h"

namespace ImGui {

	const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::Json() {
		static bool inited = false;
		static LanguageDefinition langDef;
		if (!inited) {
			langDef.mKeywords.clear();
			langDef.mIdentifiers.clear();
			langDef.mCommentStart = "/*";
			langDef.mCommentEnd = "*/";
			langDef.mSingleLineComment = "//";
			langDef.mCaseSensitive = true;
			langDef.mName = "Json";
			inited = true;
		}
		return langDef;
	}

} // namespace ImGui
