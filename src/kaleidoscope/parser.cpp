#include "parser.hpp"

namespace Kaleidoscope {

std::map<char, int> operatorPrecedence{};

int NodeStream::getTokenPrecedence()
{
	char op = m_current.content()[0];
	if (!isascii(op)) { return -1; }

	// make sure it's a declared binary operator
	int precedence = operatorPrecedence[op];
	if (precedence <= 0) { return -1; }
	return precedence;
}

void NodeStream::skipSemicolons() noexcept
{
	while (m_current == ';') {
		m_input >> m_current;
	}
}

NodeStream& NodeStream::operator>>(std::unique_ptr<Node>& a_node)
{
	if (m_current.type() == Token::Type::Def) {
		a_node = parseDefinition();
		skipSemicolons();
		return *this;
	}

	if (m_current.type() == Token::Type::Extern) {
		a_node = parseExtern();
		skipSemicolons();
		return *this;
	}

	a_node = parseTopLevelExpression();
	skipSemicolons();
	return *this;
}

std::unique_ptr<NumberExpression> NodeStream::parseNumberExpression()
{
	auto result = std::make_unique<NumberExpression>(strtod(m_current.content().c_str(), nullptr));
	m_input >> m_current; // eat number
	return result;
}

std::unique_ptr<Expression> NodeStream::parseParenExpression()
{
	m_input >> m_current; // eat '('
	auto value = parseExpression();
	if (m_current != ')') {
		throw CompileError{"expected ')'"};
	}
	m_input >> m_current; // eat ')'
	return value;
}

std::unique_ptr<Expression> NodeStream::parseIdentifierExpression()
{
	std::string name = m_current.content();

	m_input >> m_current; // eat identifier

	// simple variable ref
	if (m_current != '(') {
		return std::make_unique<VariableExpression>(name);
	}

	// call
	m_input >> m_current; // eat '('
	std::vector<std::unique_ptr<Expression>> args{};
	if (m_current != ')') {
		while (true) {
			auto arg = parseExpression();
			args.push_back(std::move(arg));

			if (m_current == ')') { break; }

			if (m_current != ',') {
				throw CompileError{"expected ')' or ',' in argument list"};
			}

			m_input >> m_current;
		}
	}

	m_input >> m_current; // eat ')'

	return std::make_unique<CallExpression>(std::move(name), std::move(args));
}

std::unique_ptr<Expression> NodeStream::parsePrimaryExpression()
{
	if (m_current.type() == Token::Type::Identifier) {
		return parseIdentifierExpression();
	}
	if (m_current.type() == Token::Type::Number) {
		return parseNumberExpression();
	}
	if (m_current == '(') {
		return parseParenExpression();
	}
	throw CompileError{"unknown token when expecting expression"};
}

std::unique_ptr<Expression> NodeStream::parseBinaryOpRhs(int a_precedence, std::unique_ptr<Expression> a_lhs)
{
	while (true) {
		// if this is a binary operator, find its precedence
		int tokenPrecedence = getTokenPrecedence();

		// if this is a binary operator that binds at least as tightly as
		// the current operator, consume it, otherwise we are done
		if (tokenPrecedence < a_precedence) { return a_lhs; }

		// we know this is a binary operator now
		char op = m_current.content()[0];
		m_input >> m_current; // eat the operator

		// parse the primary expression after the operator
		auto rhs = parsePrimaryExpression();

		// if the current operator binds less tightly with rhs than the
		// operator after rhs, let the pending operator take rhs as its lhs
		int nextPrecedence = getTokenPrecedence();
		if (tokenPrecedence < nextPrecedence) {
			rhs = parseBinaryOpRhs(tokenPrecedence + 1, std::move(rhs));
		}

		// merge lhs and rhs
		a_lhs = std::make_unique<BinaryExpression>(op, std::move(a_lhs), std::move(rhs));
	}
}

std::unique_ptr<Expression> NodeStream::parseExpression()
{
	auto lhs = parsePrimaryExpression();
	return parseBinaryOpRhs(0, std::move(lhs));
}

std::unique_ptr<Prototype> NodeStream::parsePrototype()
{
	if (m_current.type() != Token::Type::Identifier) {
		throw CompileError{"expected function name in prototype"};
	}

	std::string name = m_current.content();
	m_input >> m_current;

	if (m_current != '(') {
		throw CompileError{"expected '(' in prototype"};
	}

	// read the list of argument names
	std::vector<std::string> argNames{};
	m_input >> m_current;
	while (m_current.type() == Token::Type::Identifier) {
		argNames.push_back(m_current.content());
		m_input >> m_current;
	}

	if (m_current != ')') {
		throw CompileError{"expected ')' in prototype"};
	}

	// success
	m_input >> m_current; // eat ')'

	return std::make_unique<Prototype>(std::move(name), std::move(argNames));
}

std::unique_ptr<Function> NodeStream::parseDefinition()
{
	m_input >> m_current; // eat 'def'
	auto prototype = parsePrototype();
	auto body = parseExpression();
	return std::make_unique<Function>(std::move(prototype), std::move(body));
}

std::unique_ptr<Prototype> NodeStream::parseExtern()
{
	m_input >> m_current; // eat 'extern'
	return parsePrototype();
}

std::unique_ptr<Function> NodeStream::parseTopLevelExpression()
{
	auto expression = parseExpression();
	// make an anonymous prototype
	auto prototype = std::make_unique<Prototype>(ANONYMOUS, std::vector<std::string>{});
	return std::make_unique<Function>(std::move(prototype), std::move(expression));
}

} //namespace Kaleidoscope
