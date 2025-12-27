#pragma once

#include "Lexer.hpp"
#include "AST.hpp"
#include <map>
#include <stdexcept>
#include <unordered_map>
#include <functional>

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    Module parse();

private:
    const std::vector<Token>& tokens;
    size_t pos;

    Token peek();
    Token consume();
    Token expect(TokenType type);

    void parseTopLevel(Module& mod);
    Function parseFunc();
    void parseInstruction(std::vector<Instruction>& out);

    bool takesImmediate(Opcode op);
    Instruction parseImmediate(Opcode op);
    Opcode mapOpcode(const std::string& txt);
    void skipSExpr();
};
