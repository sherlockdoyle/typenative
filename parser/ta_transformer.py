import ast
from lark import Transformer, v_args
from ts_ast import (
  ArrayAccess,
  ArrayLiteral,
  Assignment,
  BinOp,
  Block,
  Boolean,
  ExprStmt,
  FieldAccess,
  FunctionCall,
  FunctionDecl,
  IfStmt,
  InterfaceDecl,
  InterfaceMember,
  KeyValue,
  Null,
  Number,
  ObjectLiteral,
  Param,
  PostOp,
  Program,
  ReturnStmt,
  String,
  Type,
  TypeCast,
  UnaryOp,
  VarDecl,
  Variable,
  WhileStmt,
)


@v_args(inline=True)
class TSTransformer(Transformer):
  def CNAME(self, name):
    return str(name)

  def NUMBER(self, n):
    return Number(float(n))

  def STRING(self, s):
    return String(ast.literal_eval(s))

  def true_lit(self):
    return Boolean(True)

  def false_lit(self):
    return Boolean(False)

  def null_lit(self):
    return Null()

  def var(self, name):
    return Variable(name)

  def arguments(self, *args):
    return list(args)

  def array_literal(self, elements=None):
    return ArrayLiteral(elements or [])

  def key_value(self, key, value):
    return KeyValue(key, value)

  def key_value_pairs(self, *pairs):
    return list(pairs)

  def object_literal(self, properties=None):
    return ObjectLiteral(properties or [])

  def assign(self, lvalue, value):
    return Assignment(lvalue, value)

  def add(self, l, r):
    return BinOp(l, '+', r)

  def sub(self, l, r):
    return BinOp(l, '-', r)

  def mul(self, l, r):
    return BinOp(l, '*', r)

  def div(self, l, r):
    return BinOp(l, '/', r)

  def mod(self, l, r):
    return BinOp(l, '%', r)

  def pow(self, l, r):
    return BinOp(l, '**', r)

  def or_op(self, l, r):
    return BinOp(l, '||', r)

  def and_op(self, l, r):
    return BinOp(l, '&&', r)

  def bitwise_or_op(self, l, r):
    return BinOp(l, '|', r)

  def bitwise_xor_op(self, l, r):
    return BinOp(l, '^', r)

  def bitwise_and_op(self, l, r):
    return BinOp(l, '&', r)

  def eq(self, l, r):
    return BinOp(l, '==', r)

  def neq(self, l, r):
    return BinOp(l, '!=', r)

  def strict_eq(self, l, r):
    return BinOp(l, '===', r)

  def strict_neq(self, l, r):
    return BinOp(l, '!==', r)

  def lt(self, l, r):
    return BinOp(l, '<', r)

  def gt(self, l, r):
    return BinOp(l, '>', r)

  def lte(self, l, r):
    return BinOp(l, '<=', r)

  def gte(self, l, r):
    return BinOp(l, '>=', r)

  def lshift(self, l, r):
    return BinOp(l, '<<', r)

  def rshift(self, l, r):
    return BinOp(l, '>>', r)

  def pos(self, v):
    return UnaryOp('+', v)

  def neg(self, v):
    return UnaryOp('-', v)

  def not_op(self, v):
    return UnaryOp('!', v)

  def bitwise_not_op(self, v):
    return UnaryOp('~', v)

  def pre_inc(self, v):
    return UnaryOp('++', v)

  def pre_dec(self, v):
    return UnaryOp('--', v)

  def post_inc(self, v):
    return PostOp(v, '++')

  def post_dec(self, v):
    return PostOp(v, '--')

  def type_cast(self, expression, type_node):
    return TypeCast(expression, type_node)

  def function_call(self, callee, args=None):
    return FunctionCall(callee, args or [])

  def array_access(self, array, index):
    return ArrayAccess(array, index)

  def field_access(self, object, field):
    return FieldAccess(object, field)

  def start(self, *statements):
    return Program(list(statements))

  def var_decl(self, name, type_node, value):
    return VarDecl(name, type_node, value)

  def block(self, *statements):
    return Block(list(statements))

  def if_stmt(self, condition, then_branch, else_branch=None):
    return IfStmt(condition, then_branch, else_branch)

  def while_stmt(self, condition, body):
    return WhileStmt(condition, body)

  def return_stmt(self, value):
    return ReturnStmt(value)

  def expr_stmt(self, expr):
    return ExprStmt(expr)

  def type(self, name, *brackets):
    return Type(name, len(brackets))

  def param(self, name, type_node):
    return Param(name, type_node)

  def parameters(self, *params):
    return list(params)

  def function_decl(self, name, params, return_type, body):
    return FunctionDecl(name, params or [], return_type, body)

  def extends_clause(self, name):
    return name

  def interface_member(self, name, type_node):
    return InterfaceMember(name, type_node)

  def interface_decl(self, name, extends, *members):
    return InterfaceDecl(name, extends, list(members))
