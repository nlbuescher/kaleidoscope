package kaleidoscope

import kotlinx.cinterop.*
import llvm.*
import platform.posix.*
import kotlin.collections.*

class Compiler {
	private val namedValues = mutableMapOf<String, Value>()

	// open a new context and module
	private val context = Context()
	private val module = Module("my cool jit", context)
	private val jit = Jit(module)

	// create a new builder for the module
	private val builder = IRBuilder(context)

	// create a new pass manager attached to it
	private val functionPassManager = FunctionPassManager(module).apply {
		// do simple "peephole" and bit-twiddling optimizations
		addInstructionCombiningPass()
		// reassociate expressions
		addReassociatePass()
		// eliminate common sub-expressions
		addGvnPass()
		// simplify the control flow graph (deleting unreachable blocks, etc)
		addCfgSimplificationPass()

		initialize()
	}


	val generatedCode: String get() = module.printToString()

	fun handle(string: String) {
		try {
			val tokens = Lexer.tokenize(string)
			val nodes = Parser.parse(tokens)
			nodes.forEach { node ->
				when {
					node is Function && node.prototype.name == ANONYMOUS -> {
						handleTopLevelExpression(node)
					}
					node is Function -> handleDefinition(node)
					node is Prototype -> handleExtern(node)
					else -> compileError("unexpected node type")
				}
			}
		} catch (error: CompileError) {
			fprintf(stderr, "ERROR: ${error.message}\n")
			fflush(stderr)
			usleep(100_000) // sleep for 100ms or error will be printed _after_ next prompt
		}
	}

	private fun handleTopLevelExpression(function: Function) {
		val value = function.generateIR()
		val anonFunction = jit.executionEngine.getFunctionAddress<() -> Double>(value.name)
		println("evaluated to ${anonFunction?.invoke()}")
	}

	private fun handleDefinition(function: Function) {
		val value = function.generateIR()
		println("read function definition:")
		println(value.printToString())
	}

	private fun handleExtern(function: Prototype) {
		val value = function.generateIR()
		println("read extern:")
		println(value.printToString())
	}


	// generate IR

	private fun Node.generateIR(): Value = when (this) {
		is Prototype -> generateIR()
		is Function -> generateIR()
		is NumberExpression -> generateIR()
		is VariableExpression -> generateIR()
		is BinaryExpression -> generateIR()
		is CallExpression -> generateIR()
	}

	private fun Prototype.generateIR(): llvm.Function {
		val doubles = List(args.size) { DoubleType(context) }
		val functionType = FunctionType(DoubleType(context), doubles)

		@Suppress("RemoveRedundantQualifierName")
		val function = llvm.Function(functionType, Linkage.External, name, module)

		function.args.forEachIndexed { i, it -> it.name = args[i] }

		return function
	}

	private fun Function.generateIR(): Value {
		// first check for an existing function from a previous 'extern' declaration
		val function = module.getFunction(prototype.name) ?: prototype.generateIR()

		// create a new basic block to start insertion into
		val block = BasicBlock(context, "entry", function)
		builder.setInsertPoint(block)

		// record the function arguments in the namedValues map
		namedValues.clear()
		function.args.forEach { namedValues[it.name] = it }

		val result = try {
			body.generateIR()
		} catch (error: CompileError) {
			// error reading body, remove function
			function.eraseFromParent()
			throw error
		}

		// finish off the function
		builder.createReturn(result)

		// validate the generated code, checking for consistency
		llvm.Function.verify(function)

		// optimize the function
		functionPassManager.runOn(function)

		return function
	}

	private fun NumberExpression.generateIR(): Value {
		return RealConstant(DoubleType(context), value)
	}

	private fun VariableExpression.generateIR(): Value {
		return namedValues[name] ?: compileError("unknown variable name")
	}

	private fun BinaryExpression.generateIR(): Value {
		val l = lhs.generateIR()
		val r = rhs.generateIR()

		return when (operator) {
			'+' -> builder.createFAdd(l, r, "addtmp")
			'-' -> builder.createFSub(l, r, "subtmp")
			'*' -> builder.createFMul(l, r, "multmp")
			'<' -> {
				val comparison = builder.createFCmpULT(l, r, "cmptmp")
				builder.createUIToFP(comparison, DoubleType(context), "booltmp")
			}
			else -> compileError("invalid binary operator")
		}
	}

	private fun CallExpression.generateIR(): Value {
		// look up the name in the global module table
		val function = module.getFunction(callee)
			?: compileError("unknown function referenced")

		if (function.args.size != args.size) {
			compileError("incorrect number of arguments")
		}

		val argValues = args.map { it.generateIR() }

		return builder.createCall(function, argValues, "calltmp")
	}
}
