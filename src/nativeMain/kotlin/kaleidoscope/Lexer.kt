package kaleidoscope

object Lexer {
	/**
	 * Splits the string at the index of the first character matching the given
	 * predicate.
	 * If no character matching the predicate is found, the first part will
	 * contain the entire String, and the second part, an empty String.
	 */
	private fun String.splitAtFirst(predicate: (Char) -> Boolean): Pair<String, String> {
		val index = indexOfFirst(predicate).let { if (it < 0) length else it }
		val first = substring(0, index)
		val second = substring(index)
		return first to second
	}

	fun tokenize(string: String): List<Token> {
		var input = string

		val tokens = mutableListOf<Token>()
		while (input.isNotEmpty()) {
			// skip whitespace
			input = input.trim()

			val char = input.firstOrNull() ?: continue

			// identifier: [a-zA-Z][a-zA-Z0-9]*
			if (char.isLetter()) {
				val (name, rest) = input.splitAtFirst { !it.isLetterOrDigit() }
				input = rest

				when (name) {
					"def" -> tokens.add(Token.Def)
					"extern" -> tokens.add(Token.Extern)
					else -> tokens.add(Token.Identifier(name))
				}
				continue
			}

			// number: [0-9\.]+
			if (char.isDigit() || char == '.') {
				val (valueString, rest) = input.splitAtFirst { !it.isDigit() && it != '.' }
				input = rest

				tokens.add(Token.Number(valueString.toDouble()))
				continue
			}

			// ignore comments
			if (char == '#') {
				val (_, rest) = input.splitAtFirst { char == '\n' || char == '\r' }
				input = rest
			}

			input = input.drop(1)
			tokens.add(Token.Other(char))
		}
		return tokens
	}
}
