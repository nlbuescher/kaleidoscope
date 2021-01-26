package kaleidoscope

import platform.posix.*

fun compileError(message: String): Nothing {
	fprintf(stderr, "$message\n")
	exit(1)
	error("not reached")
}

fun main() {
	// main input loop
	print("ready> ")
	var line = readLine()!!
	while (true) {
		if (line == "exit()") break

		val tokens = Lexer.tokenize(line)
		val nodes = Parser.parse(tokens)
		nodes.forEach {
			if (it is Function) {
				if (it.prototype.name == ANONYMOUS) {
					println("parsed top-level expression")
				} else {
					println("parsed function definition")
				}
			} else {
				println("parsed extern")
			}
		}

		print("ready> ")
		line = readLine()!!
	}
}
