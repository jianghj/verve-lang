#define _DARWIN_BETTER_REALPATH
#include <cassert>
#include <cstdio>
#include <fstream>
#include <libgen.h>

#if __APPLE__
#include <mach-o/dyld.h>
#else
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#endif

#include "ast/printer.h"
#include "parser/lexer.h"
#include "parser/parser.h"
#include "bytecode/generator.h"
#include "bytecode/disassembler.h"
#include "runtime/vm.h"

void printUsage() {
  puts("Usage:");
  printf("  %-30s", "verve <input>");
  puts("Execute <input> as verve source code");

  printf("  %-30s", "verve -d <input>");
  puts("Print bytecode generated for <input>");

  printf("  %-30s", "verve -c <input> <output>");
  puts("Generate bytecode for <input> and save it at <output>");

  printf("  %-30s", "verve -b <input>");
  puts("Execute <input> as verve bytecode");

  printf("  %-30s", "verve --print-ast <input>");
  puts("Print the Abstract Syntax Tree for <input>");
}

#if !__APPLE__
static int _NSGetExecutablePath(char *output, uint32_t *bufferSize) {
  char path[PATH_MAX];
  pid_t pid = getpid();
  sprintf(path, "/proc/%d/exe", pid);
  return readlink(path, output, *bufferSize);
}
#endif

int main(int argc, char **argv) {
  char buffer[PATH_MAX];
  uint32_t bufferSize = PATH_MAX;
  _NSGetExecutablePath(buffer, &bufferSize);
  char buffer2[PATH_MAX];
  realpath(buffer, buffer2);
  ROOT_DIR = dirname(buffer2);

  char *first = argv[1];
  bool isDebug = first && strcmp(first, "-d") == 0;
  bool isCompile = first && strcmp(first, "-c") == 0;
  bool isBytecode = first && strcmp(first, "-b") == 0;
  bool isAST = first && strcmp(first, "--print-ast") == 0;
  bool isHelp = first && (strcmp(first, "-h") == 0 || strcmp(first, "--help") == 0);

  if (
      (isCompile && argc != 4) ||
      ((isDebug || isBytecode || isAST) && argc != 3) ||
      isHelp ||
      (!isHelp && !isDebug && !isCompile && !isBytecode && !isAST && argc != 2)
     )
  {
    printUsage();
    return EXIT_FAILURE;
  }

  auto filename = isDebug || isCompile || isBytecode || isAST ? argv[2] : argv[1];

  FILE *source = fopen(filename, "r");

  if (!source) {
    char name[PATH_MAX];
    realpath(filename, name);
    printf("Error: Cannot open file at `%s`\n", name);
    return EXIT_FAILURE;
  }

  fseek(source, 0, SEEK_END);
  size_t sourceSize = ftell(source);
  fseek(source, 0, SEEK_SET);

  char *input = (char *)malloc(sourceSize + 1);
  fread(input, 1, sourceSize, source);
  input[sourceSize] = '\0';

  fclose(source);

  if (isBytecode) {
    Verve::VM vm((uint8_t *)input, sourceSize, true);
    vm.execute();
    free(input);
    return EXIT_SUCCESS;
  }

  Verve::Lexer lexer(filename, input);
  Verve::Parser parser(lexer, dirname(filename));
  Verve::AST::ProgramPtr ast = parser.parse();

  if (isAST) {
    Verve::AST::Printer::dump(ast);
    return EXIT_SUCCESS;
  }

  std::stringstream bytecode;
  Verve::Generator::generate(ast, !isDebug && !isCompile, &bytecode);

  if (isDebug) {
    Verve::Disassembler disassembler(bytecode);
    disassembler.dump();
  } else if (isCompile) {
    std::ofstream output(argv[3], std::ios_base::binary);
    output << bytecode.str();
  } else {
    auto bc = bytecode.str();
    Verve::VM vm((uint8_t *)bc.data(), bc.size());
    vm.execute();
  }

  free(input);
  return EXIT_SUCCESS;
}
