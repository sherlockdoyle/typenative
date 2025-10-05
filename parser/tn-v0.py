import shutil
import subprocess
from lark import Lark
from argparse import ArgumentParser
from pathlib import Path
from ta_transformer import TSTransformer


tn_parser = Lark.open('parser/tn-v0.lark')
tn_transformer = TSTransformer()

# tn build filename.tn -> parses file with parser, transforms and generates code with transformer.transform(parsed data).codegen() to build/filename.cpp. then compile with clang++ to dist/filename.a/exe. compiler command can be specified with --compiler argument
parser = ArgumentParser(description='TypeNative compiler', prog='tn')
subparsers = parser.add_subparsers(title='subcommands', dest='command', required=True)

compile_parser = subparsers.add_parser('compile', help='Compile the specified file')
compile_parser.add_argument('filename', type=str, help='The TN file to compile')
compile_parser.add_argument(
  '--compiler', type=str, default='clang++-20', help='The compiler to use (default: clang++-20)'
)

args = parser.parse_args()

filepath = Path(args.filename)
with filepath.open() as f:
  parsed = tn_parser.parse(f.read())
ast = tn_transformer.transform(parsed)

transpiled_path = Path('build') / filepath.with_suffix('.cpp')
transpiled_path.parent.mkdir(parents=True, exist_ok=True)
with transpiled_path.open('w') as f:
  f.write(ast.codegen())
shutil.copytree('src', 'build/src', dirs_exist_ok=True)

bin_path = Path('dist') / filepath.with_suffix('')
bin_path.parent.mkdir(parents=True, exist_ok=True)
subprocess.run([args.compiler, transpiled_path, '-std=c++20', '-Wall', '-o', bin_path], check=True)
