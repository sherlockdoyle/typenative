from dataclasses import dataclass


@dataclass
class Node:
  # def codegen(self):
  #   return NotImplemented

  def to_string(self):
    return NotImplemented


@dataclass
class Program(Node):
  statements: list[Node]

  def codegen(self):
    return (
      '\n'.join(['#include "src/core/core.hpp"', '#include <cmath>', '#include <initializer_list>'])
      + '\n\n'
      + '\n'.join([s.codegen() for s in self.statements])
    )

  def to_string(self):
    return '\n'.join([s.to_string() for s in self.statements])


@dataclass
class Type(Node):
  name: str
  array_dims: int = 0

  VAL_TYPES = {'i8', 'i16', 'i32', 'i64', 'i128', 'u8', 'u16', 'u32', 'u64', 'u128', 'f32', 'f64', 'f128', 'void'}

  def codegen(self):
    if self.name == 'boolean':
      return 'bool'

    if self.name in self.VAL_TYPES:
      name = self.name
    else:
      name = 'AutoRef<' + self.name + '>'

    if self.array_dims:
      return 'AutoRef<' + 'Array<' * self.array_dims + name + '>' * self.array_dims + '>'
    else:
      return name

  def to_string(self):
    return f'{self.name}{"[]" * self.array_dims}'


@dataclass
class VarDecl(Node):
  name: str
  type: Type
  value: Node

  def codegen(self):
    # inject type and name
    self.value._type = self.type
    self.value._name = self.name

    return f'{self.type.codegen()} {self.name} = {self.value.codegen()};'

  def to_string(self):
    return f'let {self.name}: {self.type.to_string()} = {self.value.to_string()};'


@dataclass
class IfStmt(Node):
  condition: Node
  then_branch: Node
  else_branch: Node | None

  def codegen(self):
    return f'if ({self.condition.codegen()}) {self.then_branch.codegen()}{f" else {self.else_branch.codegen()}" if self.else_branch else ""}'

  def to_string(self):
    return f'if ({self.condition.to_string()}) {self.then_branch.to_string()}{f" else {self.else_branch.to_string()}" if self.else_branch else ""}'


@dataclass
class WhileStmt(Node):
  condition: Node
  body: Node

  def codegen(self):
    return f'while ({self.condition.codegen()}) {self.body.codegen()}'

  def to_string(self):
    return f'while ({self.condition.to_string()}) {self.body.to_string()}'


@dataclass
class Block(Node):
  statements: list[Node]

  def codegen(self):
    return '{\n' + '\n'.join([s.codegen() for s in self.statements]) + '\n}'

  def to_string(self):
    return '{\n' + '\n'.join([s.to_string() for s in self.statements]) + '\n}'


@dataclass
class Param(Node):
  name: str
  type: Type

  def codegen(self):
    return f'{self.type.codegen()} {self.name}'

  def to_string(self):
    return f'{self.name}: {self.type.to_string()}'


@dataclass
class FunctionDecl(Node):
  name: str
  parameters: list[Param]
  return_type: Type
  body: Block

  def codegen(self):
    return f'{self.return_type.codegen()} {self.name}({", ".join([p.codegen() for p in self.parameters])})\n{self.body.codegen()}'

  def to_string(self):
    return f'function {self.name}({", ".join([p.to_string() for p in self.parameters])}): {self.return_type.to_string()}\n{self.body.to_string()}'


@dataclass
class ReturnStmt(Node):
  value: Node

  def codegen(self):
    return f'return {self.value.codegen()};'

  def to_string(self):
    return f'return {self.value.to_string()};'


@dataclass
class ExprStmt(Node):
  expr: Node

  def codegen(self):
    return f'{self.expr.codegen()};'

  def to_string(self):
    return f'{self.expr.to_string()};'


@dataclass
class InterfaceMember(Node):
  name: str
  type: Type

  def codegen(self):
    return f'{self.type.codegen()} {self.name};'

  def to_string(self):
    return f'{self.name}: {self.type.to_string()};'


@dataclass
class InterfaceDecl(Node):
  name: str
  extends: str | None
  members: list[InterfaceMember]

  def codegen(self):
    return (
      f'struct {self.name} : virtual public {self.extends or "Object"} {{\n'
      + '\n'.join([m.codegen() for m in self.members])
      + '\n};'
    )

  def to_string(self):
    return (
      f'interface {self.name}{" extends " + self.extends if self.extends else ""} {{\n'
      + '\n'.join([m.to_string() for m in self.members])
      + '\n}'
    )


@dataclass
class Number(Node):
  value: float

  def codegen(self):
    return str(int(self.value)) if self.value.is_integer() else str(self.value) + 'f'

  def to_string(self):
    return str(int(self.value) if self.value.is_integer() else self.value)


@dataclass
class String(Node):
  value: str

  def codegen(self):
    return 'newString("' + self.value.replace('\\', '\\\\').replace('"', '\\"') + '")'

  def to_string(self):
    return repr(self.value)


@dataclass
class Boolean(Node):
  value: bool

  def codegen(self):
    return 'true' if self.value else 'false'

  def to_string(self):
    return str(self.value)


@dataclass
class Null(Node):
  def codegen(self):
    return 'nullptr'

  def to_string(self):
    return 'null'


@dataclass
class Variable(Node):
  name: str

  def codegen(self):
    return self.name

  def to_string(self):
    return self.name


@dataclass
class ArrayLiteral(Node):
  elements: list[Node]

  def codegen(self):
    if self.elements:
      return 'newArray(std::initializer_list({' + ', '.join([e.codegen() for e in self.elements]) + '}))'
    return self._type.codegen() + '::make()'

  def to_string(self):
    return f'[{", ".join([e.to_string() for e in self.elements])}]'


@dataclass
class KeyValue(Node):
  key: str
  value: Node

  def codegen(self, name: str):
    return f'{name}->{self.key} = {self.value.codegen()}'

  def to_string(self):
    return f'{self.key}: {self.value.to_string()}'


@dataclass
class ObjectLiteral(Node):
  properties: list[KeyValue]

  def codegen(self):
    return f'{self._type.codegen()}::make(); ' + '; '.join([p.codegen(self._name) for p in self.properties])

  def to_string(self):
    return '{\n' + ',\n'.join([p.to_string() for p in self.properties]) + '\n}'


@dataclass
class Assignment(Node):
  lvalue: Node
  value: Node

  def codegen(self):
    return f'{self.lvalue.codegen()} = {self.value.codegen()}'

  def to_string(self):
    return f'{self.lvalue.to_string()} = {self.value.to_string()}'


@dataclass
class BinOp(Node):
  left: Node
  op: str
  right: Node

  def codegen(self):
    if self.op == '**':
      return f'std::pow({self.left.codegen()}, {self.right.codegen()})'
    if self.op == '===':
      return f'(*({self.left.codegen()}) == ({self.right.codegen()}))'
    if self.op == '!==':
      return f'(*({self.left.codegen()}) != ({self.right.codegen()}))'
    return f'({self.left.codegen()} {self.op} {self.right.codegen()})'

  def to_string(self):
    return f'({self.left.to_string()} {self.op} {self.right.to_string()})'


@dataclass
class UnaryOp(Node):
  op: str
  operand: Node

  def codegen(self):
    return f'({self.op}{self.operand.codegen()})'

  def to_string(self):
    return f'({self.op}{self.operand.to_string()})'


@dataclass
class PostOp(Node):
  operand: Node
  op: str

  def codegen(self):
    return f'({self.operand.codegen()}{self.op})'

  def to_string(self):
    return f'({self.operand.to_string()}{self.op})'


@dataclass
class TypeCast(Node):
  expression: Node
  type: Type

  def codegen(self):
    return f'({self.expression.codegen()}.as<{self.type.name}>())'

  def to_string(self):
    return f'({self.expression.to_string()} as {self.type.to_string()})'


@dataclass
class FunctionCall(Node):
  callee: Node
  args: list[Node]

  def codegen(self):
    return f'{self.callee.codegen()}({", ".join([a.codegen() for a in self.args])})'

  def to_string(self):
    return f'{self.callee.to_string()}({", ".join([a.to_string() for a in self.args])})'


@dataclass
class ArrayAccess(Node):
  array: Node
  index: Node

  def codegen(self):
    return f'{self.array.codegen()}->at({self.index.codegen()})'

  def to_string(self):
    return f'{self.array.to_string()}[{self.index.to_string()}]'


@dataclass
class FieldAccess(Node):
  object: Node
  field: str

  def codegen(self):
    return f'{self.object.codegen()}->{self.field}'

  def to_string(self):
    return f'{self.object.to_string()}.{self.field}'
