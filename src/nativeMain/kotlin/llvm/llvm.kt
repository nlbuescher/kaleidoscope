package llvm

import kotlinx.cinterop.*
import platform.llvm.*
import platform.llvm.LLVMRealPredicate.*
import platform.llvm.LLVMVerifierFailureAction.*

enum class Linkage(val value: LLVMLinkage) {
	External(LLVMLinkage.LLVMExternalLinkage)
}

open class Type internal constructor(val ptr: LLVMTypeRef?)

class DoubleType(context: Context) : Type(LLVMDoubleTypeInContext(context.ptr))

class FunctionType(returnType: Type, paramTypes: List<Type>, isVararg: Boolean = false) :
	Type(memScoped {
		val params = allocArray<LLVMTypeRefVar>(paramTypes.size)
		paramTypes.forEachIndexed { i, it -> params[i] = it.ptr }
		LLVMFunctionType(returnType.ptr, params, paramTypes.size.toUInt(), isVararg.toByte().toInt())
	})

open class Value internal constructor(val ptr: LLVMValueRef?) {
	var name: String
		get() = LLVMGetValueName(ptr)!!.toKString()
		set(name) = run { LLVMSetValueName(ptr, name) }

	fun printToString() = LLVMPrintValueToString(ptr)!!.toKString()
}

class RealConstant internal constructor(ptr: LLVMValueRef?) : Value(ptr) {
	constructor(type: DoubleType, value: Double) : this(LLVMConstReal(type.ptr, value))
}

class Function internal constructor(ptr: LLVMValueRef?) : Value(ptr) {
	constructor(
		functionType: FunctionType,
		linkage: Linkage,
		name: String,
		module: Module
	) : this(
		LLVMAddFunction(module.ptr, name, functionType.ptr).also { LLVMSetLinkage(it, linkage.value) }
	)

	@OptIn(ExperimentalStdlibApi::class)
	val args: List<Value>
		get() = buildList {
			for (i in 0u until LLVMCountParams(ptr)) {
				add(Value(LLVMGetParam(ptr, i)))
			}
		}

	fun isEmpty() = LLVMCountBasicBlocks(ptr) == 0u
	fun isNotEmpty() = !isEmpty()

	fun eraseFromParent() = LLVMDeleteFunction(ptr)


	companion object {
		fun verify(function: Function) = LLVMVerifyFunction(function.ptr, LLVMPrintMessageAction)
	}
}

class BasicBlock internal constructor(val ptr: LLVMBasicBlockRef?) {
	constructor(context: Context, name: String, function: Function) :
		this(LLVMAppendBasicBlockInContext(context.ptr, function.ptr, name))
}


class Context internal constructor(val ptr: LLVMContextRef?) {
	constructor() : this(LLVMContextCreate())
}

class Module internal constructor(val ptr: LLVMModuleRef?) {
	constructor(name: String, context: Context)
		: this(LLVMModuleCreateWithNameInContext(name, context.ptr))

	fun getFunction(name: String): Function? = LLVMGetNamedFunction(ptr, name)?.let { Function(it) }

	fun printToString(): String = LLVMPrintModuleToString(ptr)!!.toKString()
}

class IRBuilder internal constructor(val ptr: LLVMBuilderRef?) {
	constructor(context: Context) : this(LLVMCreateBuilderInContext(context.ptr))

	fun setInsertPoint(block: BasicBlock) = LLVMPositionBuilderAtEnd(ptr, block.ptr)

	fun createReturn(value: Value) = Value(LLVMBuildRet(ptr, value.ptr))

	fun createFAdd(lhs: Value, rhs: Value, name: String = ""): Value {
		return Value(LLVMBuildFAdd(ptr, lhs.ptr, rhs.ptr, name))
	}

	fun createFSub(lhs: Value, rhs: Value, name: String = ""): Value {
		return Value(LLVMBuildFSub(ptr, lhs.ptr, rhs.ptr, name))
	}

	fun createFMul(lhs: Value, rhs: Value, name: String = ""): Value {
		return Value(LLVMBuildFMul(ptr, lhs.ptr, rhs.ptr, name))
	}

	fun createFCmpULT(lhs: Value, rhs: Value, name: String = ""): Value {
		return Value(LLVMBuildFCmp(ptr, LLVMRealULT, lhs.ptr, rhs.ptr, name))
	}

	fun createUIToFP(value: Value, destinationType: DoubleType, name: String = ""): Value {
		return Value(LLVMBuildUIToFP(ptr, value.ptr, destinationType.ptr, name))
	}

	fun createCall(function: Function, args: List<Value>, name: String = ""): Value = memScoped {
		val cargs = allocArray<LLVMValueRefVar>(args.size)
		args.forEachIndexed { i, it -> cargs[i] = it.ptr }
		Value(LLVMBuildCall(ptr, function.ptr, cargs, args.size.toUInt(), name))
	}
}

class FunctionPassManager internal constructor(val ptr: LLVMPassManagerRef?) {
	constructor(module: Module) : this(LLVMCreateFunctionPassManagerForModule(module.ptr))

	fun addInstructionCombiningPass() = LLVMAddInstructionCombiningPass(ptr)
	fun addReassociatePass() = LLVMAddReassociatePass(ptr)
	fun addGvnPass() = LLVMAddGVNPass(ptr)
	fun addCfgSimplificationPass() = LLVMAddCFGSimplificationPass(ptr)

	fun initialize() = LLVMInitializeFunctionPassManager(ptr)

	fun runOn(function: Function) = LLVMRunFunctionPassManager(ptr, function.ptr)
}

class ExecutionEngine internal constructor(val ptr: LLVMExecutionEngineRef?) {
	constructor(module: Module) : this(memScoped {
		val error = allocPointerTo<ByteVar>()
		val engine = alloc<LLVMExecutionEngineRefVar>()
		LLVMCreateExecutionEngineForModule(engine.ptr, module.ptr, error.ptr)
		if (error.value != null) error(error.value!!.toKString())
		engine.value
	})

	fun <T : kotlin.Function<*>> getFunctionAddress(name: String): CPointer<CFunction<T>>? {
		return LLVMGetFunctionAddress(ptr, name).toLong().toCPointer()
	}
}
