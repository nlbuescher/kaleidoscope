#ifndef KALEIDOSCOPE_COMPILER_HPP
#define KALEIDOSCOPE_COMPILER_HPP

#include "lexer.hpp"
#include "parser.hpp"

namespace Kaleidoscope {

class Compiler
{
public:
	Compiler() noexcept
	{
		operatorPrecedence['<'] = 10;
		operatorPrecedence['+'] = 20;
		operatorPrecedence['-'] = 20;
		operatorPrecedence['*'] = 40;
	}

	void handle(std::string a_input) noexcept;

private:
	void handleTopLevelExpression(std::unique_ptr<Function> a_function);
	void handleDefinition(std::unique_ptr<Function> a_function);
	void handleExtern(std::unique_ptr<Prototype> a_prototype);
};

} //namespace Kaleidoscope

#endif //KALEIDOSCOPE_COMPILER_HPP
