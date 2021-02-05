#ifndef KALEIDOSCOPE_PARSER_HPP
#define KALEIDOSCOPE_PARSER_HPP

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <stdexcept>

#include "lexer.hpp"

#define ANONYMOUS "__anonexpr"

namespace Kaleidoscope {

extern std::map<char, int> operatorPrecedence;

class Node
{
public:
	enum class Type
	{
		NumberExpression,
		VariableExpression,
		BinaryExpression,
		CallExpression,
		Prototype,
		Function
	};

	virtual ~Node() = default;

	[[nodiscard]] virtual Type type() const noexcept = 0;
};

class Expression : public Node {};

class NumberExpression : public Expression
{
private:
	double m_value;

public:
	explicit NumberExpression(double a_value) noexcept: m_value{a_value} {}

	[[nodiscard]] const double& value() const noexcept { return m_value; }

	[[nodiscard]] Type type() const noexcept override { return Type::NumberExpression; }
};

class VariableExpression : public Expression
{
private:
	std::string m_name;

public:
	explicit VariableExpression(std::string a_name) noexcept: m_name{std::move(a_name)} {}

	[[nodiscard]] const std::string& name() const noexcept { return m_name; }

	[[nodiscard]] Type type() const noexcept override { return Type::VariableExpression; }
};

class BinaryExpression : public Expression
{
private:
	char m_op;
	std::unique_ptr<Expression> m_lhs, m_rhs;

public:
	BinaryExpression(char a_op, std::unique_ptr<Expression> a_lhs, std::unique_ptr<Expression> a_rhs) noexcept
		: m_op{a_op}, m_lhs{std::move(a_lhs)}, m_rhs{std::move(a_rhs)} {}

	[[nodiscard]] const char& op() const noexcept { return m_op; }
	[[nodiscard]] const std::unique_ptr<Expression>& lhs() const noexcept { return m_lhs; }
	[[nodiscard]] const std::unique_ptr<Expression>& rhs() const noexcept { return m_rhs; }

	[[nodiscard]] Type type() const noexcept override { return Type::BinaryExpression; }
};

class CallExpression : public Expression
{
private:
	std::string m_callee;
	std::vector<std::unique_ptr<Expression>> m_args;

public:
	CallExpression(std::string a_callee, std::vector<std::unique_ptr<Expression>> a_args) noexcept
		: m_callee{std::move(a_callee)}, m_args{std::move(a_args)} {}

	[[nodiscard]] const std::string& callee() const noexcept { return m_callee; }
	[[nodiscard]] const std::vector<std::unique_ptr<Expression>>& args() const noexcept { return m_args; }

	[[nodiscard]] Type type() const noexcept override { return Type::CallExpression; }
};

class Prototype : public Node
{
private:
	std::string m_name;
	std::vector<std::string> m_args;
public:
	Prototype(std::string a_name, std::vector<std::string> a_args) noexcept
		: m_name{std::move(a_name)}, m_args{std::move(a_args)} {}

	[[nodiscard]] const std::string& name() const noexcept { return m_name; }
	[[nodiscard]] const std::vector<std::string>& args() const noexcept { return m_args; }

	[[nodiscard]] Type type() const noexcept override { return Type::Prototype; }
};

class Function : public Node
{
private:
	std::unique_ptr<Prototype> m_prototype;
	std::unique_ptr<Expression> m_body;

public:
	Function(std::unique_ptr<Prototype> a_prototype, std::unique_ptr<Expression> a_body) noexcept
		: m_prototype{std::move(a_prototype)}, m_body{std::move(a_body)} {}

	[[nodiscard]] std::unique_ptr<Prototype>& prototype() noexcept { return m_prototype; }
	[[nodiscard]] const std::unique_ptr<Expression>& body() const noexcept { return m_body; }

	[[nodiscard]] Type type() const noexcept override { return Type::Function; }
};

class CompileError : public std::runtime_error
{
public:
	explicit CompileError(const char* a_what) : std::runtime_error{a_what} {}
};

class NodeStream
{
private:
	TokenStream m_input;
	Token m_current;

public:
	explicit NodeStream(std::string a_input) noexcept: m_input{std::move(a_input)}
	{
		m_input >> m_current;
		skipSemicolons();
	}

	[[nodiscard]] bool hasNext() noexcept { return m_current.type() != Token::Type::Eof; }

	/// top ::= definition | external | expression | ';'
	NodeStream& operator>>(std::unique_ptr<Node>& a_node);

private:
	void skipSemicolons() noexcept;

	/// numberExpression ::= number
	std::unique_ptr<NumberExpression> parseNumberExpression();

	/// parenExpression ::= '(' expression ')'
	std::unique_ptr<Expression> parseParenExpression();

	/// identifierExpression
	///		::= identifier
	///		::= identifier '(' expression* ')'
	std::unique_ptr<Expression> parseIdentifierExpression();

	/// primaryExpression
	///		::= identifierExpression
	///		::= numberExpression
	///		::= parenExpression
	std::unique_ptr<Expression> parsePrimaryExpression();

	/// get the precedence of the pending binary operator token
	int getTokenPrecedence();

	/// binaryOpRhs
	///		::= ('.' primary)*
	std::unique_ptr<Expression> parseBinaryOpRhs(int a_precedence, std::unique_ptr<Expression> a_lhs);

	/// expression
	/// 	::= primary binaryOpRhs
	std::unique_ptr<Expression> parseExpression();

	/// m_prototype
	/// 	::= identifier '(' identifier ')'
	std::unique_ptr<Prototype> parsePrototype();

	/// definition ::= 'def' m_prototype expression
	std::unique_ptr<Function> parseDefinition();

	/// external ::= 'extern' m_prototype
	std::unique_ptr<Prototype> parseExtern();

	/// topLevelExpression ::= expression
	std::unique_ptr<Function> parseTopLevelExpression();
};

} //namespace Kaleidoscope

#endif //KALEIDOSCOPE_PARSER_HPP
