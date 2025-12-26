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
    // Potentially line/col info
};

class Lexer {
public:
    explicit Lexer(const std::string& input) : source(input), pos(0) {}

    Token next() {
        skipWhitespace();
        if (pos >= source.length()) return {TokenType::END_OF_FILE, ""};

        char c = source[pos];

        if (c == '(') { pos++; return {TokenType::LPAREN, "("}; }
        if (c == ')') { pos++; return {TokenType::RPAREN, ")"}; }

        if (c == '"') return readString();
        if (c == '$') return readIdentifier();
        if (isdigit(c) || c == '-') return readNumber(); // simplified check, - can be start of number
        if (isalpha(c)) return readKeyword(); // instructions start with letters usually

        // Fallback for weird chars or strict errors?
        // Let's just consume one char as unknown if we want, or throw.
        // For now, assume well-formed-ish.
        pos++;
        return {TokenType::KEYWORD, std::string(1, c)}; // Should verify
    }

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (true) {
            Token t = next();
            tokens.push_back(t);
            if (t.type == TokenType::END_OF_FILE) break;
        }
        return tokens;
    }

private:
    std::string source;
    size_t pos;

    void skipWhitespace() {
        while (pos < source.length()) {
            // Handle comments ;;
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

    Token readString() {
        // Skip opening quote
        pos++;
        std::string res;
        while (pos < source.length() && source[pos] != '"') {
            if (source[pos] == '\\') {
                pos++;
                // Handle escapes if needed
            }
            res += source[pos++];
        }
        if (pos < source.length()) pos++; // Skip closing quote
        return {TokenType::STRING, res};
    }

    Token readIdentifier() {
        // Starts with $
        size_t start = pos;
        pos++; // skip $
        while (pos < source.length() && (isalnum(source[pos]) || source[pos] == '_' || source[pos] == '.')) {
            pos++;
        }
        return {TokenType::IDENTIFIER, source.substr(start, pos - start)};
    }

    Token readNumber() {
        size_t start = pos;
        bool isFloat = false;
        if (source[pos] == '-') pos++;

        while (pos < source.length() && (isdigit(source[pos]) || source[pos] == '.' || source[pos] == 'e' || source[pos] == 'E' || source[pos] == 'x' || (source[pos] >= 'a' && source[pos] <= 'f') || (source[pos] >= 'A' && source[pos] <= 'F'))) {
            // Very permissive number parsing to catch hex etc.
            // But we need to distinguish 1 vs 1.0 vs i32.const? No, i32.const is keyword.
            if (source[pos] == '.') isFloat = true;
            pos++;
        }
        // Refinement: if it looks like a number but followed by char, it might be weird.
        return {isFloat ? TokenType::FLOAT : TokenType::INTEGER, source.substr(start, pos - start)};
    }

    Token readKeyword() {
        size_t start = pos;
        while (pos < source.length() && (isalnum(source[pos]) || source[pos] == '.' || source[pos] == '_')) {
            pos++;
        }
        return {TokenType::KEYWORD, source.substr(start, pos - start)};
    }
};
