#ifndef KALEIDOSCOPE_LEXER_HPP
#define KALEIDOSCOPE_LEXER_HPP

#include <string>

namespace Kaleidoscope {

class Token
{
public:
	enum class Type : int8_t
	{
		Eof,

		// commands
		Def, Extern,

		// primary
		Identifier, Number,

		Other,
	};

private:
	Type m_type{Type::Eof};
	std::string m_content{EOF};

public:
	Token() noexcept = default;

	Token(Type a_type, std::string a_content) noexcept
		: m_type{a_type}, m_content{std::move(a_content)} {}

	Token(Type a_type, char a_content) noexcept
		: m_type{a_type}, m_content{a_content} {}


	[[nodiscard]] const Type& type() const noexcept { return m_type; }

	[[nodiscard]] const std::string& content() const noexcept { return m_content; }


	bool operator==(const char& a_other) const noexcept
	{
		const char* content = m_content.c_str();
		return content != nullptr && (*content == a_other);
	}

	bool operator!=(const char& a_other) const noexcept
	{
		const char* content = m_content.c_str();
		return content == nullptr || (*content != a_other);
	}
};

class TokenStream
{
private:
	char m_current{' '};
	uint32_t m_index{0};
	std::string m_input;

public:
	explicit TokenStream(std::string a_input) noexcept
		: m_input{std::move(a_input)} {}

	TokenStream& operator>>(Token& a_token) noexcept;

private:
	char getChar() noexcept;
};

} //namespace Kaleidoscope

#endif //KALEIDOSCOPE_LEXER_HPP
