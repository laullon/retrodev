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
		namespace Glsl {
			//
			// GLSL reserved keywords
			//
			const char* const kGlslKeywords[] = {"auto",	   "break",		"case",			  "char",		  "const",	  "continue", "default", "do",	   "double",   "else",
												 "enum",	   "extern",	"float",		  "for",		  "goto",	  "if",		  "inline",	 "int",	   "long",	   "register",
												 "restrict",   "return",	"short",		  "signed",		  "sizeof",	  "static",	  "struct",	 "switch", "typedef",  "union",
												 "unsigned",   "void",		"volatile",		  "while",		  "_Alignas", "_Alignof", "_Atomic", "_Bool",  "_Complex", "_Generic",
												 "_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local"};
			//
			// GLSL built-in functions
			//
			const char* const kGlslIdentifiers[] = {"abort",   "abs",	  "acos",	 "asin",	"atan",	   "atexit",   "atof",	  "atoi",	 "atol",	"ceil",
													"clock",   "cosh",	  "ctime",	 "div",		"exit",	   "fabs",	   "floor",	  "fmod",	 "getchar", "getenv",
													"isalnum", "isalpha", "isdigit", "isgraph", "ispunct", "isspace",  "isupper", "kbhit",	 "log10",	"log2",
													"log",	   "memcmp",  "modf",	 "pow",		"putchar", "putenv",   "puts",	  "rand",	 "remove",	"rename",
													"sinh",	   "sqrt",	  "srand",	 "strcat",	"strcmp",  "strerror", "time",	  "tolower", "toupper"};
		} // namespace Glsl
	} // namespace TextEditorLangs

	const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::Glsl() {
		static bool inited = false;
		static LanguageDefinition langDef;
		if (!inited) {
			// Populate keywords
			for (auto& k : TextEditorLangs::Glsl::kGlslKeywords)
				langDef.mKeywords.insert(k);
			// Populate known identifiers
			for (auto& k : TextEditorLangs::Glsl::kGlslIdentifiers) {
				Identifier id;
				id.mDeclaration = "Built-in function";
				langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
			}
			langDef.mCommentStart = "/*";
			langDef.mCommentEnd = "*/";
			langDef.mSingleLineComment = "//";
			langDef.mCaseSensitive = true;
			langDef.mName = "GLSL";
			inited = true;
		}
		return langDef;
	}

} // namespace ImGui
