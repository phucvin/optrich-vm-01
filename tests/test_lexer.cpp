#include <iostream>
#include "Lexer.h"

void printToken(const Token& t) {
    switch(t.type) {
        case TokenType::LPAREN: std::cout << "LPAREN" << std::endl; break;
        case TokenType::RPAREN: std::cout << "RPAREN" << std::endl; break;
        case TokenType::KEYWORD: std::cout << "KW: " << t.text << std::endl; break;
        case TokenType::INTEGER: std::cout << "INT: " << t.text << std::endl; break;
        case TokenType::FLOAT: std::cout << "FLOAT: " << t.text << std::endl; break;
        case TokenType::STRING: std::cout << "STR: " << t.text << std::endl; break;
        case TokenType::IDENTIFIER: std::cout << "ID: " << t.text << std::endl; break;
        case TokenType::END_OF_FILE: std::cout << "EOF" << std::endl; break;
    }
}

int main() {
    std::string code = R"(
        (module
            (func $test (param $x i32) (result i32)
                ;; this is a comment
                (i32.add (local.get $x) (i32.const 42))
            )
        )
    )";

    Lexer lexer(code);
    auto tokens = lexer.tokenize();

    for (const auto& t : tokens) {
        printToken(t);
    }
    return 0;
}
