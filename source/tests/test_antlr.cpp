

#include <iostream>

#include "libs/antlr4/antlr4-runtime.h"
#include "meta/ParserCpp14/CPP14Lexer.h"
#include "meta/ParserCpp14/CPP14Parser.h"
#include "meta/ParserCpp14/CPP14ParserBaseVisitor.h"

using namespace MetaEngine::StaticRefl;

using namespace antlr4;

int main(int argc, const char* argv[]) {

    ANTLRInputStream input(R"(
namespace A::B {
  struct [[meta("hello world")]] Cmpt{
  };
  enum class [[info]] Color {
    RED [[test]],
    GREEN,
    BLUE
  };
}
)");
    CPP14Lexer lexer(&input);
    CommonTokenStream tokens(&lexer);

    CPP14ParserBaseVisitor visitor;

    CPP14Parser parser(&tokens);
    tree::ParseTree* tree = parser.translationUnit();
    tree->accept(&visitor);
    std::string s = tree->toStringTree(&parser);

    // OutputDebugString(s.data()); // Only works properly since VS 2015.
    std::cout << "Parse Tree: " << s << std::endl;  // Unicode output in the console is very limited.

    return 0;
}
