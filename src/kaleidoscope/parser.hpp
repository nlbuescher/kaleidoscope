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
		IfExpression,
		ForExpression,
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

	[[nodiscard]] double& value() noexcept { return m_value; }

	[[nodiscard]] Type type() const noexcept override { return Type::NumberExpression; }
};

class VariableExpression : public Expression
{
private:
	std::string m_name;

public:
	explicit VariableExpression(std::string a_name) noexcept: m_name{std::move(a_name)} {}

	[[nodiscard]] std::string& name() noexcept { return m_name; }

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

	[[nodiscard]] char& op() noexcept { return m_op; }
	[[nodiscard]] std::unique_ptr<Expression>& lhs() noexcept { return m_lhs; }
	[[nodiscard]] std::unique_ptr<Expression>& rhs() noexcept { return m_rhs; }

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

	[[nodiscard]] std::string& callee() noexcept { return m_callee; }
	[[nodiscard]] std::vector<std::unique_ptr<Expression>>& args() noexcept { return m_args; }

	[[nodiscard]] Type type() const noexcept override { return Type::CallExpression; }
};

class IfExpression : public Expression
{
private:
	std::unique_ptr<Expression> m_condition, m_thenBody, m_elseBody;

public:
	IfExpression(
		std::unique_ptr<Expression> a_condition,
		std::unique_ptr<Expression> a_thenBody,
		std::unique_ptr<Expression> a_elseBody
	) noexcept
		: m_condition{std::move(a_condition)}
		, m_thenBody{std::move(a_thenBody)}
		, m_elseBody{std::move(a_elseBody)} {}

	[[nodiscard]] std::unique_ptr<Expression>& condition() noexcept { return m_condition; }
	[[nodiscard]] std::unique_ptr<Expression>& thenBody() noexcept { return m_thenBody; }
	[[nodiscard]] std::unique_ptr<Expression>& elseBody() noexcept { return m_elseBody; }

	[[nodiscard]] Type type() const noexcept override { return Type::IfExpression; }
};

class ForExpression : public Expression
{
private:
	std::string m_varName;
	std::unique_ptr<Expression> m_start, m_end, m_step, m_body;

public:
	ForExpression(
		std::string a_varName,
		std::unique_ptr<Expression> a_start,
		std::unique_ptr<Expression> a_end,
		std::unique_ptr<Expression> a_step,
		std::unique_ptr<Expression> a_body
	) noexcept
		: m_varName{std::move(a_varName)}
		, m_start{std::move(a_start)}
		, m_end{std::move(a_end)}
		, m_step{std::move(a_step)}
		, m_body{std::move(a_body)} {}

	[[nodiscard]] std::string& varName() noexcept { return m_varName; }
	[[nodiscard]] std::unique_ptr<Expression>& start() noexcept { return m_start; }
	[[nodiscard]] std::unique_ptr<Expression>& end() noexcept { return m_end; }
	[[nodiscard]] std::unique_ptr<Expression>& step() noexcept { return m_step; }
	[[nodiscard]] std::unique_ptr<Expression>& body() noexcept { return m_body; }

	[[nodiscard]] Type type() const noexcept override { return Type::ForExpression; }
};

class Prototype : public Node
{
private:
	std::string m_name;
	std::vector<std::string> m_args;
public:
	Prototype(std::string a_name, std::vector<std::string> a_args) noexcept
		: m_name{std::move(a_name)}, m_args{std::move(a_args)} {}

	[[nodiscard]] std::string& name() noexcept { return m_name; }
	[[nodiscard]] std::vector<std::string>& args() noexcept { return m_args; }

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
	[[nodiscard]] std::unique_ptr<Expression>& body() noexcept { return m_body; }

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

	/// ifExpression ::= 'if' expression 'then' expression 'else' expression
	std::unique_ptr<Expression> parseIfExpression();

	/// forExpression ::= 'for' identifier '=' expression ',' expression (',' expression)? 'in' expression
	std::unique_ptr<Expression> parseForExpression();

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

	/// prototype
	/// 	::= identifier '(' identifier ')'
	std::unique_ptr<Prototype> parsePrototype();

	/// definition ::= 'def' prototype expression
	std::unique_ptr<Function> parseDefinition();

	/// external ::= 'extern' prototype
	std::unique_ptr<Prototype> parseExtern();

	/// topLevelExpression ::= expression
	std::unique_ptr<Function> parseTopLevelExpression();
};

} //namespace Kaleidoscope

#endif //KALEIDOSCOPE_PARSER_HPP
