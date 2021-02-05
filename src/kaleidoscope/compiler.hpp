#ifndef KALEIDOSCOPE_COMPILER_HPP
#define KALEIDOSCOPE_COMPILER_HPP

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/TargetSelect.h>

#pragma GCC diagnostic pop

#include "lexer.hpp"
#include "parser.hpp"
#include "jit.hpp"

namespace Kaleidoscope {

class Compiler
{
private:
	std::map<std::string, llvm::Value*> m_namedValues{};
	std::map<std::string, std::unique_ptr<Prototype>> m_prototypes{};

	llvm::LLVMContext m_context{};
	llvm::IRBuilder<> m_builder{m_context};
	std::unique_ptr<JIT> m_jit;
	std::unique_ptr<llvm::Module> m_module;
	std::unique_ptr<llvm::legacy::FunctionPassManager> m_functionPassManager;

public:
	Compiler() noexcept
	{
		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmParser();
		llvm::InitializeNativeTargetAsmPrinter();

		operatorPrecedence['<'] = 10;
		operatorPrecedence['+'] = 20;
		operatorPrecedence['-'] = 20;
		operatorPrecedence['*'] = 40;

		m_jit = std::make_unique<JIT>();

		initializeModuleAndPassManager();
	}

	void handle(std::string a_input) noexcept;

private:
	void initializeModuleAndPassManager() noexcept;

	llvm::Function* getFunction(const std::string& name) noexcept;

	void handleTopLevelExpression(std::unique_ptr<Function> a_function);
	void handleDefinition(std::unique_ptr<Function> a_function);
	void handleExtern(std::unique_ptr<Prototype> a_prototype);

	// IR
	llvm::Value* generateIRFrom(Node& a_node);
	llvm::Value* generateIRFrom(NumberExpression& a_expression);
	llvm::Value* generateIRFrom(VariableExpression& a_expression);
	llvm::Value* generateIRFrom(BinaryExpression& a_expression);
	llvm::Value* generateIRFrom(CallExpression& a_expression);
	llvm::Function* generateIRFrom(Prototype& a_prototype);
	llvm::Function* generateIRFrom(Function& a_function);
};

} //namespace Kaleidoscope

#endif //KALEIDOSCOPE_COMPILER_HPP
