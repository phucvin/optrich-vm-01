#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <optional>

enum class TokenType {
    LPAREN,     // (
    RPAREN,     // )
    KEYWORD,    // module, func, etc. (or instructions like i32.add)
    INTEGER,    // 123
    FLOAT,      // 1.23
    STRING,     // "hello"
    IDENTIFIER, // $foo
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string text;
};

class Lexer {
public:
    explicit Lexer(const std::string& input);

    Token next();
    std::vector<Token> tokenize();

private:
    std::string source;
    size_t pos;

    void skipWhitespace();
    Token readString();
    Token readIdentifier();
    Token readNumber();
    Token readKeyword();
};
