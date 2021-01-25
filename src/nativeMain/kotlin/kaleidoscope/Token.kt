package kaleidoscope

sealed class Token {
	object Def : Token() {
		override fun toString() = "Def"
	}

	object Extern : Token() {
		override fun toString() = "Extern"
	}


	data class Identifier(val name: String) : Token()

	data class Number(val value: Double) : Token()


	data class Other(val char: Char) : Token()
}
