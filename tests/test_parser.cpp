#include <iostream>
#include "Parser.h"

void printInstr(const Instruction& instr) {
    std::cout << "Op: " << (int)instr.opcode;
    if (std::holds_alternative<int32_t>(instr.operand)) std::cout << " Val: " << std::get<int32_t>(instr.operand);
    if (std::holds_alternative<std::string>(instr.operand)) std::cout << " Val: " << std::get<std::string>(instr.operand);
    std::cout << std::endl;
}

int main() {
    std::string code = R"(
        (module
            (func $test (param $x i32) (result i32)
                (i32.add (local.get $x) (i32.const 42))
            )
        )
    )";

    Lexer lexer(code);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    Module mod = parser.parse();

    std::cout << "Parsed " << mod.functions.size() << " functions." << std::endl;
    for (const auto& func : mod.functions) {
        std::cout << "Func: " << func.name << std::endl;
        for (const auto& instr : func.body) {
            printInstr(instr);
        }
    }

    return 0;
}
