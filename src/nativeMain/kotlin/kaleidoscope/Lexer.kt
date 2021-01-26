package kaleidoscope

object Lexer {
	/**
	 * Splits the string at the index of the first character matching the given predicate.
	 * If no character matching the predicate is found, the first part will contain the entire
	 * string, and the second part, an empty string.
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
			// ignore whitespace
			input = input.trim()

			val char = input.firstOrNull() ?: continue

			// identifier : [a-zA-Z][a-zA-Z0-9]*
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

			// number : '.' [0-9]+ | [0-9]+ '.'? [0-9]*
			if (char.isDigit() || (char == '.' && input.getOrNull(1)?.isDigit() == true)) {
				val (valueString, rest) = if (char == '.') {
					val (fractionalPart, rest) = input.drop(1).splitAtFirst { !it.isDigit() }
					".$fractionalPart" to rest
				} else {
					val (integerPart, afterDigits) = input.splitAtFirst { !it.isDigit() }
					val (fractionalPart, rest) = if (afterDigits.firstOrNull() == '.') {
						afterDigits.drop(1).splitAtFirst { !it.isDigit() }
					} else {
						"" to afterDigits
					}
					"$integerPart.$fractionalPart" to rest
				}

				input = rest

				tokens.add(Token.Number(valueString.toDouble()))
				continue
			}

			// ignore comments until end of line
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
