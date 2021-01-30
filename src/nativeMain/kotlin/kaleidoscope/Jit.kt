package kaleidoscope

import llvm.*
import platform.llvm.*

class Jit(module: Module) {
	init {
		LLVMLinkInMCJIT()
		LLVMInitializeNativeTarget()
		LLVMInitializeNativeAsmParser()
		LLVMInitializeNativeAsmPrinter()
	}

	val executionEngine = ExecutionEngine(module)
}
