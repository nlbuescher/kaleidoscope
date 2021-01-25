package kaleidoscope

fun main() {
	// main input loop
	print("ready> ")
	var line = readLine()!!
	while (true) {
		if (line == "exit()") break

		val tokens = Lexer.tokenize(line)
		println(tokens)

		print("ready> ")
		line = readLine()!!
	}
}
