//----------------------------------------------------------------------------------------------------
//
//
//
//
//
//----------------------------------------------------------------------------------------------------

#include "../imgui.text.editor.h"

namespace ImGui {
	namespace TextEditorLangs {
		namespace Cs {
			//
			// C# reserved keywords
			//
			const char* const kCsKeywords[] = {"abstract",
											   "as",
											   "base",
											   "bool",
											   "break",
											   "byte",
											   "case",
											   "catch",
											   "char",
											   "checked",
											   "class",
											   "const",
											   "continue",
											   "decimal",
											   "default",
											   "delegate",
											   "do",
											   "double",
											   "else",
											   "enum",
											   "event",
											   "explicit",
											   "extern",
											   "false",
											   "finally",
											   "fixed",
											   "float",
											   "for",
											   "foreach",
											   "goto",
											   "if",
											   "implicit",
											   "in",
											   "in (generic modifier)",
											   "int",
											   "interface",
											   "internal",
											   "is",
											   "lock",
											   "long",
											   "namespace",
											   "new",
											   "null",
											   "object",
											   "operator",
											   "out",
											   "out (generic modifier)",
											   "override",
											   "params",
											   "private",
											   "protected",
											   "public",
											   "readonly",
											   "ref",
											   "return",
											   "sbyte",
											   "sealed",
											   "short",
											   "sizeof",
											   "stackalloc",
											   "static",
											   "string",
											   "struct",
											   "switch",
											   "this",
											   "throw",
											   "true",
											   "try",
											   "typeof",
											   "uint",
											   "ulong",
											   "unchecked",
											   "unsafe",
											   "ushort",
											   "using",
											   "using static",
											   "void",
											   "volatile",
											   "while"};
			//
			// C# contextual keywords (used as identifiers in certain contexts)
			//
			const char* const kCsIdentifiers[] = {"add",  "alias", "ascending", "async",   "await",	 "descending", "dynamic", "from",  "get", "global", "group", "into",
												  "join", "let",   "orderby",	"partial", "remove", "select",	   "set",	  "value", "var", "when",	"where", "yield"};
		} // namespace Cs
	} // namespace TextEditorLangs

	const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::Cs() {
		static bool inited = false;
		static LanguageDefinition langDef;
		if (!inited) {
			// Populate keywords
			for (auto& k : TextEditorLangs::Cs::kCsKeywords)
				langDef.mKeywords.insert(k);
			// Populate known identifiers
			for (auto& k : TextEditorLangs::Cs::kCsIdentifiers) {
				Identifier id;
				id.mDeclaration = "Built-in function";
				langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
			}
			langDef.mCommentStart = "/*";
			langDef.mCommentEnd = "*/";
			langDef.mSingleLineComment = "//";
			langDef.mCaseSensitive = true;
			langDef.mName = "C#";
			inited = true;
		}
		return langDef;
	}

} // namespace ImGui
