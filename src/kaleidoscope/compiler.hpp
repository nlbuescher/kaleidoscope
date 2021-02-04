#ifndef KALEIDOSCOPE_COMPILER_HPP
#define KALEIDOSCOPE_COMPILER_HPP

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

#pragma GCC diagnostic pop

#include "lexer.hpp"
#include "parser.hpp"

namespace Kaleidoscope {

class Compiler
{
private:
	std::map<std::string, llvm::Value*> m_namedValues{};

	llvm::LLVMContext m_context{};
	llvm::IRBuilder<> m_builder{m_context};
	std::unique_ptr<llvm::Module> m_module{std::make_unique<llvm::Module>("my cool jit", m_context)};

public:
	Compiler() noexcept
	{
		operatorPrecedence['<'] = 10;
		operatorPrecedence['+'] = 20;
		operatorPrecedence['-'] = 20;
		operatorPrecedence['*'] = 40;
	}

	void print(llvm::raw_ostream& a_stream) noexcept { m_module->print(a_stream, nullptr); }

	void handle(std::string a_input) noexcept;

private:
	void handleTopLevelExpression(std::unique_ptr<Function> a_function);
	void handleDefinition(std::unique_ptr<Function> a_function);
	void handleExtern(std::unique_ptr<Prototype> a_prototype);

	// IR
	llvm::Value* generateIRFrom(const Node& a_node);
	llvm::Value* generateIRFrom(const NumberExpression& a_expression);
	llvm::Value* generateIRFrom(const VariableExpression& a_expression);
	llvm::Value* generateIRFrom(const BinaryExpression& a_expression);
	llvm::Value* generateIRFrom(const CallExpression& a_expression);
	llvm::Function* generateIRFrom(const Prototype& a_prototype);
	llvm::Function* generateIRFrom(const Function& a_function);
};

} //namespace Kaleidoscope

#endif //KALEIDOSCOPE_COMPILER_HPP
