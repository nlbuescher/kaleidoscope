package kaleidoscope

object Parser {
	private val precedenceMap = mapOf(
		'<' to 10,
		'+' to 20,
		'-' to 20,
		'*' to 40,
	)

	private fun precedenceOf(token: Token?): Int {
		if (token !is Token.Other) return -1
		return precedenceMap[token.char] ?: -1
	}

	/** numberExpression : number */
	private fun parseNumberExpression(tokens: List<Token>): Pair<Expression, List<Token>> {
		val result = NumberExpression((tokens.first() as Token.Number).value)
		return result to tokens.drop(1)
	}

	/** parenExpression : '(' expression ')' */
	private fun parseParenExpression(tokens: List<Token>): Pair<Expression, List<Token>> {
		val (expression, rest) = parseExpression(tokens.drop(1))
		if (rest.firstOrNull() != Token.Other(')')) {
			compileError("expected ')'")
		}
		return expression to rest.drop(1)
	}

	/** identifierExpression : identifier | identifier '(' expression* ')' */
	private fun parseIdentifierExpression(tokens: List<Token>): Pair<Expression, List<Token>> {
		val name = (tokens.first() as Token.Identifier).name

		// simple variable ref
		if (tokens.getOrNull(1) != Token.Other('(')) {
			return VariableExpression(name) to tokens.drop(1)
		}

		// function call
		var rest = tokens.drop(2)
		val args = mutableListOf<Expression>()
		if (rest.firstOrNull() != Token.Other(')')) {
			while (true) {
				val arg: Expression
				parseExpression(rest).let { arg = it.first; rest = it.second }
				args.add(arg)

				if (rest.firstOrNull() == Token.Other(')')) break

				if (rest.firstOrNull() != Token.Other(',')) {
					compileError("expected ')' or ',' in argument list")
				}

				rest = rest.drop(1)
			}
		}

		return CallExpression(name, args) to rest.drop(1)
	}

	/** primaryExpression : identifierExpression | numberExpression | parenExpression */
	private fun parsePrimaryExpression(tokens: List<Token>): Pair<Expression, List<Token>> {
		return when (tokens.firstOrNull()) {
			is Token.Identifier -> parseIdentifierExpression(tokens)
			is Token.Number -> parseNumberExpression(tokens)
			Token.Other('(') -> parseParenExpression(tokens)
			else -> compileError("unknown token when expecting expression")
		}
	}

	/** binaryOperatorRhs : (operator primaryExpression)* */
	private fun parseBinaryOperatorRhs(
		precedence: Int,
		expression: Expression,
		tokens: List<Token>
	): Pair<Expression, List<Token>> {
		var lhs = expression
		var rest = tokens
		while (true) {
			// check that the first token is a binary operator. if not, return
			val tokenPrecedence = precedenceOf(rest.firstOrNull())
			if (tokenPrecedence < precedence) {
				return lhs to rest
			}

			// get operator char
			val operator = (rest.first() as Token.Other).char

			var rhs: Expression
			parsePrimaryExpression(rest.drop(1)).let { rhs = it.first; rest = it.second }

			// if the next operator's precedence is higher than this operator's,
			// use rhs as lhs for next level
			val nextPrecedence = precedenceOf(rest.firstOrNull())
			if (tokenPrecedence < nextPrecedence) {
				parseBinaryOperatorRhs(tokenPrecedence + 1, rhs, rest).let {
					rhs = it.first; rest = it.second
				}
			}

			// merge lhs and rhs
			lhs = BinaryExpression(operator, lhs, rhs)
		}
	}

	/** expression : primaryExpression binaryOperatorRhs */
	private fun parseExpression(tokens: List<Token>): Pair<Expression, List<Token>> {
		val (lhs, rest) = parsePrimaryExpression(tokens)
		return parseBinaryOperatorRhs(0, lhs, rest)
	}

	/** prototype : identifier '(' identifier* ')' */
	private fun parsePrototype(tokens: List<Token>): Pair<Prototype, List<Token>> {
		if (tokens.firstOrNull() !is Token.Identifier) {
			compileError("expected function name in prototype")
		}

		val name = (tokens.first() as Token.Identifier).name

		if (tokens.getOrNull(1) != Token.Other('(')) {
			compileError("expected '(' in prototype")
		}

		var i = 2
		val argNames = mutableListOf<String>()
		while (tokens.getOrNull(i) is Token.Identifier) {
			argNames.add((tokens[i++] as Token.Identifier).name)
		}

		if (tokens.getOrNull(i++) != Token.Other(')')) {
			compileError("expected ')' in prototype")
		}

		return Prototype(name, argNames) to tokens.drop(i)
	}

	/** definition : 'def' prototype expression */
	private fun parseDefinition(tokens: List<Token>): Pair<Function, List<Token>> {
		val (prototype, afterPrototype) = parsePrototype(tokens.drop(1))
		val (body, afterBody) = parseExpression(afterPrototype)
		return Function(prototype, body) to afterBody
	}

	/** extern : 'extern' prototype */
	private fun parseExtern(tokens: List<Token>): Pair<Prototype, List<Token>> {
		return parsePrototype(tokens.drop(1))
	}

	/** topLevelExpression : expression */
	private fun parseTopLevelExpression(tokens: List<Token>): Pair<Function, List<Token>> {
		val (expression, rest) = parseExpression(tokens)
		val prototype = Prototype(ANONYMOUS, emptyList())
		return Function(prototype, expression) to rest
	}


	fun parse(tokens: List<Token>): List<Node> {
		var input = tokens.dropWhile { it == Token.Other(';') }

		val nodes = mutableListOf<Node>()
		while (input.isNotEmpty()) {
			val (node, rest) = when (input.first()) {
				Token.Def -> parseDefinition(input)
				Token.Extern -> parseExtern(input)
				else -> parseTopLevelExpression(input)
			}
			nodes.add(node)
			input = rest.dropWhile { it == Token.Other(';') }
		}
		return nodes
	}
}
