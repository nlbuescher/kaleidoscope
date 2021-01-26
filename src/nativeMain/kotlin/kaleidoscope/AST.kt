package kaleidoscope

const val ANONYMOUS = "__anonexpr"

sealed class Node

class Prototype(
	val name: String,
	val args: List<String>
) : Node()

class Function(
	val prototype: Prototype,
	val body: Expression
) : Node()

sealed class Expression : Node()

class NumberExpression(
	val value: Double
) : Expression()

class VariableExpression(
	val name: String
) : Expression()

class BinaryExpression(
	val operator: Char,
	val lhs: Expression,
	val rhs: Expression,
) : Expression()

class CallExpression(
	val callee: String,
	val args: List<Expression>
) : Expression()
