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
		namespace Cpp {
			//
			// C++ reserved keywords
			//
			const char* const kCppKeywords[] = {"alignas",
												"alignof",
												"and",
												"and_eq",
												"asm",
												"atomic_cancel",
												"atomic_commit",
												"atomic_noexcept",
												"auto",
												"bitand",
												"bitor",
												"bool",
												"break",
												"case",
												"catch",
												"char",
												"char16_t",
												"char32_t",
												"class",
												"compl",
												"concept",
												"const",
												"constexpr",
												"const_cast",
												"continue",
												"decltype",
												"default",
												"delete",
												"do",
												"double",
												"dynamic_cast",
												"else",
												"enum",
												"explicit",
												"export",
												"extern",
												"false",
												"float",
												"for",
												"friend",
												"goto",
												"if",
												"import",
												"inline",
												"int",
												"long",
												"module",
												"mutable",
												"namespace",
												"new",
												"noexcept",
												"not",
												"not_eq",
												"nullptr",
												"operator",
												"or",
												"or_eq",
												"private",
												"protected",
												"public",
												"register",
												"reinterpret_cast",
												"requires",
												"return",
												"short",
												"signed",
												"sizeof",
												"static",
												"static_assert",
												"static_cast",
												"struct",
												"switch",
												"synchronized",
												"template",
												"this",
												"thread_local",
												"throw",
												"true",
												"try",
												"typedef",
												"typeid",
												"typename",
												"union",
												"unsigned",
												"using",
												"virtual",
												"void",
												"volatile",
												"wchar_t",
												"while",
												"xor",
												"xor_eq"};
			//
			// C++ standard library identifiers and built-in functions
			//
			const char* const kCppIdentifiers[] = {
				"abort",   "abs",	 "acos",   "asin",	 "atan",		  "atexit",	 "atof",		  "atoi",	 "atol",	"ceil",		"clock",	"cosh",	   "ctime",
				"div",	   "exit",	 "fabs",   "floor",	 "fmod",		  "getchar", "getenv",		  "isalnum", "isalpha", "isdigit",	"isgraph",	"ispunct", "isspace",
				"isupper", "kbhit",	 "log10",  "log2",	 "log",			  "memcmp",	 "modf",		  "pow",	 "printf",	"sprintf",	"snprintf", "putchar", "putenv",
				"puts",	   "rand",	 "remove", "rename", "sinh",		  "sqrt",	 "srand",		  "strcat",	 "strcmp",	"strerror", "time",		"tolower", "toupper",
				"std",	   "string", "vector", "map",	 "unordered_map", "set",	 "unordered_set", "min",	 "max"};
			static bool TokenizeCStyleString(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end) {
				const char* p = in_begin;
				if (*p == '"') {
					p++;
					while (p < in_end) {
						if (*p == '"') {
							out_begin = in_begin;
							out_end = p + 1;
							return true;
						}
						if (*p == '\\' && p + 1 < in_end && p[1] == '"')
							p++;
						p++;
					}
				}
				return false;
			}
			static bool TokenizeCStyleCharacterLiteral(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end) {
				const char* p = in_begin;
				if (*p == '\'') {
					p++;
					if (p < in_end && *p == '\\')
						p++;
					if (p < in_end)
						p++;
					if (p < in_end && *p == '\'') {
						out_begin = in_begin;
						out_end = p + 1;
						return true;
					}
				}
				return false;
			}
			static bool TokenizeCStyleIdentifier(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end) {
				const char* p = in_begin;
				if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_') {
					p++;
					while ((p < in_end) && ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_'))
						p++;
					out_begin = in_begin;
					out_end = p;
					return true;
				}
				return false;
			}
			static bool TokenizeCStyleNumber(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end) {
				const char* p = in_begin;
				const bool startsWithNumber = *p >= '0' && *p <= '9';
				if (*p != '+' && *p != '-' && !startsWithNumber)
					return false;
				p++;
				bool hasNumber = startsWithNumber;
				while (p < in_end && (*p >= '0' && *p <= '9')) {
					hasNumber = true;
					p++;
				}
				if (hasNumber == false)
					return false;
				if (p < in_end) {
					if (*p == '.') {
						p++;
						while (p < in_end && (*p >= '0' && *p <= '9'))
							p++;
					} else if (*p == 'x' || *p == 'X') {
						// hex formatted integer of the type 0xef80
						p++;
						while (p < in_end && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')))
							p++;
					} else if (*p == 'b' || *p == 'B') {
						// binary formatted integer of the type 0b01011101
						p++;
						while (p < in_end && (*p >= '0' && *p <= '1'))
							p++;
					}
				}
				if (true) {
					// floating point exponent
					if (p < in_end && (*p == 'e' || *p == 'E')) {
						p++;
						if (p < in_end && (*p == '+' || *p == '-'))
							p++;
						bool hasDigits = false;
						while (p < in_end && (*p >= '0' && *p <= '9')) {
							hasDigits = true;
							p++;
						}
						if (hasDigits == false)
							return false;
					}
					// single precision floating point type
					if (p < in_end && *p == 'f')
						p++;
				}
				if (true) {
					// integer size type
					while (p < in_end && (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L'))
						p++;
				}
				out_begin = in_begin;
				out_end = p;
				return true;
			}
			static bool TokenizeCStylePunctuation(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end) {
				(void)in_end;
				switch (*in_begin) {
					case '[':
					case ']':
					case '{':
					case '}':
					case '!':
					case '%':
					case '^':
					case '&':
					case '*':
					case '(':
					case ')':
					case '-':
					case '+':
					case '=':
					case '~':
					case '|':
					case '<':
					case '>':
					case '?':
					case ':':
					case '/':
					case ';':
					case ',':
					case '.':
						out_begin = in_begin;
						out_end = in_begin + 1;
						return true;
				}
				return false;
			}
		} // namespace Cpp
	} // namespace TextEditorLangs

	const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::Cpp() {
		static bool inited = false;
		static LanguageDefinition langDef;
		if (!inited) {
			// Populate keywords
			for (auto& k : TextEditorLangs::Cpp::kCppKeywords)
				langDef.mKeywords.insert(k);
			// Populate known identifiers
			for (auto& k : TextEditorLangs::Cpp::kCppIdentifiers) {
				Identifier id;
				id.mDeclaration = "Built-in function";
				langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
			}
			// C-style tokenizer: strings, char literals, identifiers, numbers, punctuation
			langDef.mTokenize = [](const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end, PaletteIndex& paletteIndex) -> bool {
				paletteIndex = PaletteIndex::Max;
				while (in_begin < in_end && isascii(*in_begin) && isblank(*in_begin))
					in_begin++;
				if (in_begin == in_end) {
					out_begin = in_end;
					out_end = in_end;
					paletteIndex = PaletteIndex::Default;
				} else if (TextEditorLangs::Cpp::TokenizeCStyleString(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::String;
				else if (TextEditorLangs::Cpp::TokenizeCStyleCharacterLiteral(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::CharLiteral;
				else if (TextEditorLangs::Cpp::TokenizeCStyleIdentifier(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::Identifier;
				else if (TextEditorLangs::Cpp::TokenizeCStyleNumber(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::Number;
				else if (TextEditorLangs::Cpp::TokenizeCStylePunctuation(in_begin, in_end, out_begin, out_end))
					paletteIndex = PaletteIndex::Punctuation;
				return paletteIndex != PaletteIndex::Max;
			};
			langDef.mCommentStart = "/*";
			langDef.mCommentEnd = "*/";
			langDef.mSingleLineComment = "//";
			langDef.mCaseSensitive = true;
			langDef.mName = "C++";
			inited = true;
		}
		return langDef;
	}

} // namespace ImGui
