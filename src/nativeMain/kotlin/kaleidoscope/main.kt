package kaleidoscope

class CompileError(message: String) : Exception(message)

fun compileError(message: String): Nothing {
	throw CompileError(message)
}

fun main() {
	val compiler = Compiler()

	// main input loop
	while (true) {
		print("ready> ")
		val line = readLine()!!
		if (line == "exit()") {
			println(compiler.generatedCode)

			println("finished")
			break
		}

		compiler.handle(line)
	}
}
