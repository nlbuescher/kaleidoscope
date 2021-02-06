#include "lexer.hpp"

namespace Kaleidoscope {

char TokenStream::getChar() noexcept
{
	if (m_index >= m_input.length()) { return EOF; }
	return m_input[m_index++];
}

TokenStream& TokenStream::operator>>(Token& a_token) noexcept
{
	// skip any whitespace
	while (isspace(m_current)) {
		m_current = getChar();
	}

	// identifier: [a-zA-Z][a-zA-Z0-9]*
	if (isalpha(m_current)) {
		std::string content{};
		do {
			content += m_current;
			m_current = getChar();
		} while (isalnum(m_current));

		if (content == "def") {
			a_token = Token{Token::Type::Def, content};
			return *this;
		}
		if (content == "extern") {
			a_token = Token{Token::Type::Extern, content};
			return *this;
		}
		if (content == "if") {
			a_token = Token{Token::Type::If, content};
			return *this;
		}
		if (content == "then") {
			a_token = Token{Token::Type::Then, content};
			return *this;
		}
		if (content == "else") {
			a_token = Token{Token::Type::Else, content};
			return *this;
		}
		if (content == "for") {
			a_token = Token{Token::Type::For, content};
			return *this;
		}
		if (content == "in") {
			a_token = Token{Token::Type::In, content};
			return *this;
		}
		a_token = Token{Token::Type::Identifier, content};
		return *this;
	}

	// number: [0-9\.]+
	if (isdigit(m_current) || m_current == '.') {
		std::string value{};
		do {
			value += m_current;
			m_current = getChar();
		} while (isdigit(m_current) || m_current == '.');

		a_token = Token{Token::Type::Number, value};
		return *this;
	}

	// skip comments
	if (m_current == '#') {
		do { m_current = getChar(); }
		while (m_current != EOF && m_current != '\n' && m_current != '\r');

		if (m_current != EOF) {
			return *this >> a_token;
		}
	}

	// check for end of file. don't eat the EOF
	if (m_current == EOF) {
		a_token = Token{Token::Type::Eof, EOF};
		return *this;
	}

	a_token = Token{Token::Type::Other, m_current};
	m_current = getChar();
	return *this;
}

} //namespace Kaleidoscope
