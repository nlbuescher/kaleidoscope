#include "compiler.hpp"

#include <unistd.h>
#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"

#include <llvm/ADT/APFloat.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

#pragma GCC diagnostic pop

namespace Kaleidoscope {

void Compiler::initializeModuleAndPassManager() noexcept
{
	// open a new module
	m_module = std::make_unique<llvm::Module>("my cool jit", m_context);
	m_module->setDataLayout(m_jit->targetMachine().createDataLayout());

	// create a new pass manager attached to it
	m_functionPassManager = std::make_unique<llvm::legacy::FunctionPassManager>(m_module.get());

	// do simple "peephole" and bit-twiddling optimizations
	m_functionPassManager->add(llvm::createInstructionCombiningPass());
	// reassociate expressions
	m_functionPassManager->add(llvm::createReassociatePass());
	// eliminate common sub-expressions
	m_functionPassManager->add(llvm::createGVNPass());
	// simplify the control flow graph
	m_functionPassManager->add(llvm::createCFGSimplificationPass());

	m_functionPassManager->doInitialization();
}

llvm::Function* Compiler::getFunction(const std::string& name) noexcept
{
	// first see if the function has already been added to the current module
	if (auto function = m_module->getFunction(name)) { return function; }

	// if not, check whether we can codegen the declaration from some existing prototype
	auto prototype = m_prototypes.find(name);
	if (prototype != m_prototypes.end()) {
		return generateIRFrom(*prototype->second);
	}

	// if no existing prototype exists, return nullptr
	return nullptr;
}

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
	generateIRFrom(*a_function);
	// jit the module containing the anonymous expression,
	// keeping the key so we can free it later
	auto key = m_jit->addModule(std::move(m_module));
	initializeModuleAndPassManager();

	// search the jit for the ANONYMOUS symbol
	auto symbol = m_jit->findSymbol(ANONYMOUS);
	assert(symbol && "function not found");

	// get the symbol's address and cast it to the right type
	// so we can call it as a native function
	auto function = reinterpret_cast<double (*)()>(symbol.getAddress().get());
	auto result = function();
	std::cout << "evaluated to " << result << '\n';

	// delete the anonymous expression module from the jit
	m_jit->removeModule(key);
}

void Compiler::handleDefinition(std::unique_ptr<Function> a_function)
{
	auto value = generateIRFrom(*a_function);
	std::cout << "read function definition:\n";
	value->print(llvm::outs());
	m_jit->addModule(std::move(m_module));
	initializeModuleAndPassManager();
}

void Compiler::handleExtern(std::unique_ptr<Prototype> a_prototype)
{
	auto value = generateIRFrom(*a_prototype);
	std::cout << "read extern:\n";
	value->print(llvm::outs());
	m_prototypes[a_prototype->name()] = std::move(a_prototype);
}

llvm::Value* Compiler::generateIRFrom(Node& a_node)
{
	if (a_node.type() == Node::Type::NumberExpression) {
		return generateIRFrom(dynamic_cast<NumberExpression&>(a_node));
	}
	if (a_node.type() == Node::Type::VariableExpression) {
		return generateIRFrom(dynamic_cast<VariableExpression&>(a_node));
	}
	if (a_node.type() == Node::Type::BinaryExpression) {
		return generateIRFrom(dynamic_cast<BinaryExpression&>(a_node));
	}
	if (a_node.type() == Node::Type::CallExpression) {
		return generateIRFrom(dynamic_cast<CallExpression&>(a_node));
	}
	if (a_node.type() == Node::Type::Prototype) {
		return generateIRFrom(dynamic_cast<Prototype&>(a_node));
	}
	if (a_node.type() == Node::Type::Function) {
		return generateIRFrom(dynamic_cast<Function&>(a_node));
	}
	throw CompileError{"unexpected node type"};
}

llvm::Value* Compiler::generateIRFrom(NumberExpression& a_expression)
{
	return llvm::ConstantFP::get(llvm::Type::getDoubleTy(m_context), llvm::APFloat(a_expression.value()));
}

llvm::Value* Compiler::generateIRFrom(VariableExpression& a_expression)
{
	auto value = m_namedValues[a_expression.name()];
	if (!value) {
		throw CompileError{"unknown variable name"};
	}
	return value;
}

llvm::Value* Compiler::generateIRFrom(BinaryExpression& a_expression)
{
	auto lhs = generateIRFrom(*a_expression.lhs());
	auto rhs = generateIRFrom(*a_expression.rhs());

	switch (a_expression.op()) {
		case '+':
			return m_builder.CreateFAdd(lhs, rhs, "addtmp");
		case '-':
			return m_builder.CreateFSub(lhs, rhs, "subtmp");
		case '*':
			return m_builder.CreateFMul(lhs, rhs, "multmp");
		case '<':
			lhs = m_builder.CreateFCmpULT(lhs, rhs, "cmptmp");
			// convert bool 0/1 to double 0.0/1.0
			return m_builder.CreateUIToFP(lhs, llvm::Type::getDoubleTy(m_context), "booltmp");
		default:
			throw CompileError{"invalid binary operator"};
	}
}

llvm::Value* Compiler::generateIRFrom(CallExpression& a_expression)
{
	// look up the name in the global module table
	auto callee = getFunction(a_expression.callee());
	if (!callee) {
		throw CompileError{"unknown function referenced"};
	}

	// if argument mismatch error
	if (callee->arg_size() != a_expression.args().size()) {
		throw CompileError{"incorrect number of arguments passed"};
	}

	std::vector<llvm::Value*> args;
	for (auto& arg : a_expression.args()) {
		args.push_back(generateIRFrom(*arg));
	}

	return m_builder.CreateCall(callee, args, "calltmp");
}

llvm::Function* Compiler::generateIRFrom(Prototype& a_prototype)
{
	// make the function type
	std::vector<llvm::Type*> doubles{a_prototype.args().size(), llvm::Type::getDoubleTy(m_context)};
	auto functionType = llvm::FunctionType::get(llvm::Type::getDoubleTy(m_context), doubles, false);

	auto function = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, a_prototype.name(),
	                                       *m_module);

	// set names for the arguments
	for (uint i = 0; i < function->arg_size(); i++) {
		function->getArg(i)->setName(a_prototype.args()[i]);
	}

	return function;
}

llvm::Function* Compiler::generateIRFrom(Function& a_function)
{
	// first check for an existing function from a previous 'extern' declaration
	auto& prototype = *a_function.prototype();
	m_prototypes[prototype.name()] = std::move(a_function.prototype());
	auto function = getFunction(prototype.name());

	if (!function) {
		throw CompileError{"function not found"};
	}

	// create a new basic block to start insertion into
	auto block = llvm::BasicBlock::Create(m_context, "entry", function);
	m_builder.SetInsertPoint(block);

	// record the function arguments in the namedValues map
	m_namedValues.clear();
	for (auto& arg : function->args()) {
		m_namedValues[arg.getName().str()] = &arg;
	}

	llvm::Value* result;
	try {
		result = generateIRFrom(*a_function.body());
	} catch (CompileError& error) {
		// error reading body, remove function
		function->eraseFromParent();
		throw error;
	}

	// finish off the function
	m_builder.CreateRet(result);

	// validate the generated code, checking for consistency
	llvm::verifyFunction(*function);

	// optimize the function
	m_functionPassManager->run(*function);

	return function;
}

} //namespace Kaleidoscope
