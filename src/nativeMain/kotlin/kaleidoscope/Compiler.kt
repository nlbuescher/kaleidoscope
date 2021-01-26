package kaleidoscope

import kotlinx.cinterop.*
import platform.llvm.*
import platform.llvm.LLVMLinkage.*
import platform.llvm.LLVMRealPredicate.*
import platform.llvm.LLVMVerifierFailureAction.*
import platform.posix.*

class Compiler {
	private val module: LLVMModuleRef? = LLVMModuleCreateWithName("my cool jit")

	private val builder: LLVMBuilderRef? = LLVMCreateBuilder()
	private val namedValues = mutableMapOf<String, LLVMValueRef?>()

	val generatedCode: String get() = LLVMPrintModuleToString(module)!!.toKString()

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
		println("read top-level expression:")
		println(LLVMPrintValueToString(value)!!.toKString())
	}

	private fun handleDefinition(function: Function) {
		val value = function.generateIR()
		println("read function definition:")
		println(LLVMPrintValueToString(value)!!.toKString())
	}

	private fun handleExtern(function: Prototype) {
		val value = function.generateIR()
		println("read extern:")
		println(LLVMPrintValueToString(value)!!.toKString())
	}


	// generate IR

	private fun Node.generateIR(): LLVMValueRef? = when (this) {
		is Prototype -> generateIR()
		is Function -> generateIR()
		is NumberExpression -> generateIR()
		is VariableExpression -> generateIR()
		is BinaryExpression -> generateIR()
		is CallExpression -> generateIR()
	}

	private fun Prototype.generateIR(): LLVMValueRef? {
		val function = memScoped {
			val doubles = allocArray<LLVMTypeRefVar>(args.size) { value = LLVMDoubleType() }

			val functionType = LLVMFunctionType(LLVMDoubleType(), doubles, args.size.toUInt(), 0)
			LLVMAddFunction(module, name, functionType).also {
				LLVMSetLinkage(it, LLVMExternalLinkage)
			}
		}

		args.forEachIndexed { i, argName ->
			val arg = LLVMGetParam(function, i.toUInt())
			LLVMSetValueName(arg, argName)
		}

		return function
	}

	private fun Function.generateIR(): LLVMValueRef? {
		val function = LLVMGetNamedFunction(module, prototype.name) ?: prototype.generateIR()

		if (LLVMCountBasicBlocks(function).toInt() != 0) {
			compileError("function cannot be redefined")
		}

		val block = LLVMAppendBasicBlock(function, "entry")
		LLVMPositionBuilderAtEnd(builder, block)

		namedValues.clear()
		prototype.args.forEachIndexed { i, argName ->
			val arg = LLVMGetParam(function, i.toUInt())
			namedValues[argName] = arg
		}

		val result = try {
			body.generateIR()
		} catch (error: CompileError) {
			LLVMDeleteFunction(function)
			throw error
		}

		// finish off the function
		LLVMBuildRet(builder, result)

		// validate the generated code
		LLVMVerifyFunction(function, LLVMPrintMessageAction)

		return function
	}

	private fun NumberExpression.generateIR(): LLVMValueRef? {
		return LLVMConstReal(LLVMDoubleType(), value)
	}

	private fun VariableExpression.generateIR(): LLVMValueRef? {
		if (name !in namedValues) compileError("unknown variable name")
		return namedValues[name]
	}

	private fun BinaryExpression.generateIR(): LLVMValueRef? {
		val l = lhs.generateIR()
		val r = rhs.generateIR()

		return when (operator) {
			'+' -> LLVMBuildFAdd(builder, l, r, "addtmp")
			'-' -> LLVMBuildFSub(builder, l, r, "subtmp")
			'*' -> LLVMBuildFMul(builder, l, r, "multmp")
			'<' -> LLVMBuildFCmp(builder, LLVMRealULT, l, r, "cmptmp")?.let { comparison ->
				LLVMBuildUIToFP(builder, comparison, LLVMDoubleType(), "booltmp")
			}
			else -> compileError("invalid binary operator")
		}
	}

	private fun CallExpression.generateIR(): LLVMValueRef? {
		// look up the name in the global module table
		val function = LLVMGetNamedFunction(module, callee)
			?: compileError("unknown function referenced")

		if (LLVMCountParams(function).toInt() != args.size) {
			compileError("incorrect number of arguments")
		}

		return memScoped {
			val argValues = allocArray<LLVMValueRefVar>(args.size)
			args.forEachIndexed { i, it ->
				argValues[i] = it.generateIR()
			}

			LLVMBuildCall(builder, function, argValues, args.size.toUInt(), "calltmp")
		}
	}
}
