#include "Lexer.h"
#include <cctype>

Lexer::Lexer(const std::string& input) : source(input), pos(0) {}

Token Lexer::next() {
    skipWhitespace();
    if (pos >= source.length()) return {TokenType::END_OF_FILE, ""};

    char c = source[pos];

    if (c == '(') { pos++; return {TokenType::LPAREN, "("}; }
    if (c == ')') { pos++; return {TokenType::RPAREN, ")"}; }

    if (c == '"') return readString();
    if (c == '$') return readIdentifier();
    if (isdigit(c) || c == '-') return readNumber();
    if (isalpha(c)) return readKeyword();

    pos++;
    return {TokenType::KEYWORD, std::string(1, c)};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        Token t = next();
        tokens.push_back(t);
        if (t.type == TokenType::END_OF_FILE) break;
    }
    return tokens;
}

void Lexer::skipWhitespace() {
    while (pos < source.length()) {
        if (source.compare(pos, 2, ";;") == 0) {
            while (pos < source.length() && source[pos] != '\n') pos++;
            continue;
        }
        if (isspace(source[pos])) {
            pos++;
        } else {
            break;
        }
    }
}

Token Lexer::readString() {
    pos++;
    std::string res;
    while (pos < source.length() && source[pos] != '"') {
        if (source[pos] == '\\') {
            pos++;
        }
        res += source[pos++];
    }
    if (pos < source.length()) pos++;
    return {TokenType::STRING, res};
}

Token Lexer::readIdentifier() {
    size_t start = pos;
    pos++;
    while (pos < source.length() && (isalnum(source[pos]) || source[pos] == '_' || source[pos] == '.')) {
        pos++;
    }
    return {TokenType::IDENTIFIER, source.substr(start, pos - start)};
}

Token Lexer::readNumber() {
    size_t start = pos;
    bool isFloat = false;
    if (source[pos] == '-') pos++;

    while (pos < source.length() && (isdigit(source[pos]) || source[pos] == '.' || source[pos] == 'e' || source[pos] == 'E' || source[pos] == 'x' || (source[pos] >= 'a' && source[pos] <= 'f') || (source[pos] >= 'A' && source[pos] <= 'F'))) {
        if (source[pos] == '.') isFloat = true;
        pos++;
    }
    return {isFloat ? TokenType::FLOAT : TokenType::INTEGER, source.substr(start, pos - start)};
}

Token Lexer::readKeyword() {
    size_t start = pos;
    while (pos < source.length() && (isalnum(source[pos]) || source[pos] == '.' || source[pos] == '_')) {
        pos++;
    }
    return {TokenType::KEYWORD, source.substr(start, pos - start)};
}
