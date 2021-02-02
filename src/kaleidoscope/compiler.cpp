#include "compiler.hpp"

#include <unistd.h>
#include <iostream>

namespace Kaleidoscope {

void Compiler::handle(std::string a_input) noexcept
{
	NodeStream input{std::move(a_input)};
	std::unique_ptr<Node> node;

	try {
		while (input.hasNext()) {
			input >> node;

			if (node->type() == Node::Type::Function) {
				std::unique_ptr<Function> function{dynamic_cast<Function*>(node.release())};
				if (function->prototype()->name() == ANONYMOUS) {
					handleTopLevelExpression(std::move(function));
				} else {
					handleDefinition(std::move(function));
				}
			} else { //node->type() == Node::Type::Prototype
				std::unique_ptr<Prototype> prototype{dynamic_cast<Prototype*>(node.release())};
				handleExtern(std::move(prototype));
			}
		}
	} catch (CompileError& error) {
		std::cerr << "ERROR: " << error.what() << std::endl;
		usleep(200000);
	}
}

void Compiler::handleTopLevelExpression(std::unique_ptr<Function> a_function)
{
	std::cout << "parsed top-level expression\n";
}

void Compiler::handleDefinition(std::unique_ptr<Function> a_function)
{
	std::cout << "parsed function definition\n";
}

void Compiler::handleExtern(std::unique_ptr<Prototype> a_prototype)
{
	std::cout << "parsed extern\n";
}

} //namespace Kaleidoscope
