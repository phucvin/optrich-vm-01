#pragma once

#include "Lexer.h"
#include "AST.h"
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
    Import parseImport();
    Function parseFunc();
    Type parseType();
    Table parseTable();
    ElementSegment parseElem();
    void parseInstruction(std::vector<Instruction>& out);

    bool takesImmediate(Opcode op);
    Instruction parseImmediate(Opcode op);
    Opcode mapOpcode(const std::string& txt);
    void skipSExpr();
};
