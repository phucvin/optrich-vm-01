#include "Parser.h"

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

Module Parser::parse() {
    Module module;
    while (pos < tokens.size() && tokens[pos].type != TokenType::END_OF_FILE) {
        parseTopLevel(module);
    }
    return module;
}

Token Parser::peek() {
    if (pos >= tokens.size()) return {TokenType::END_OF_FILE, ""};
    return tokens[pos];
}

Token Parser::consume() {
    return tokens[pos++];
}

Token Parser::expect(TokenType type) {
    Token t = consume();
    if (t.type != type) {
        throw std::runtime_error("Unexpected token: " + t.text + " expected type " + std::to_string((int)type));
    }
    return t;
}

void Parser::parseTopLevel(Module& mod) {
    expect(TokenType::LPAREN);
    Token t = consume();
    if (t.text != "module") {
            // Treat as module anyway? No, strictly require (module ...
            throw std::runtime_error("Expected module");
    }

    while (peek().type == TokenType::LPAREN) {
        size_t checkpoint = pos;
        consume(); // (
        Token fieldName = peek();
        if (fieldName.text == "func") {
            consume();
            mod.functions.push_back(parseFunc());
        } else if (fieldName.text == "import") {
            consume();
            mod.imports.push_back(parseImport());
        } else {
            pos = checkpoint + 1;
            skipSExpr();
        }
    }
    expect(TokenType::RPAREN);
}

Function Parser::parseFunc() {
    Function func;

    if (peek().type == TokenType::IDENTIFIER) {
        std::string raw = consume().text;
        if (!raw.empty() && raw[0] == '$') raw = raw.substr(1);
        func.name = raw;
    }

    while (peek().type != TokenType::RPAREN) {
        if (peek().type == TokenType::LPAREN) {
            size_t checkpoint = pos;
            consume(); // (
            Token inner = peek();
            if (inner.text == "param") {
                consume();
                // (param $x i32) or (param i32)
                while (peek().type != TokenType::RPAREN) {
                    std::string name = "";
                    if (peek().type == TokenType::IDENTIFIER) {
                        name = consume().text;
                        if (!name.empty() && name[0] == '$') name = name.substr(1);
                    }
                    if (peek().type == TokenType::KEYWORD) {
                        func.paramTypes.push_back(consume().text);
                        func.paramNames.push_back(name);
                    }
                }
                expect(TokenType::RPAREN);
            } else if (inner.text == "result") {
                consume();
                while (peek().type != TokenType::RPAREN) {
                    func.resultTypes.push_back(consume().text);
                }
                expect(TokenType::RPAREN);
            } else if (inner.text == "local") {
                consume();
                while (peek().type != TokenType::RPAREN) {
                        std::string name = "";
                        if (peek().type == TokenType::IDENTIFIER) {
                            name = consume().text;
                            if (!name.empty() && name[0] == '$') name = name.substr(1);
                        }
                        if (peek().type == TokenType::KEYWORD) {
                            func.localTypes.push_back(consume().text);
                            func.localNames.push_back(name);
                        }
                }
                expect(TokenType::RPAREN);
            } else {
                // Instruction
                pos = checkpoint; // Go back to (
                parseInstruction(func.body);
            }
        } else {
            // Flat instruction
            parseInstruction(func.body);
        }
    }
    expect(TokenType::RPAREN);
    // Implicit return?
    // func.body.push_back(Instruction(Opcode::END)); // Optional
    return func;
}

void Parser::parseInstruction(std::vector<Instruction>& out) {
    if (peek().type == TokenType::LPAREN) {
        // Folded: (opcode arg1 arg2)
        consume(); // (
        Token opToken = consume();
        Opcode op = mapOpcode(opToken.text);

        // Check if opcode takes immediate
        if (takesImmediate(op)) {
            // The immediate is the next token(s)
            // e.g. (i32.const 42) or (br $label)
            Instruction instr = parseImmediate(op);

            // Any subsequent tokens are nested instructions (operands)
            // e.g. (br_if $label (i32.eq ...))
            while (peek().type != TokenType::RPAREN) {
                parseInstruction(out);
            }
            expect(TokenType::RPAREN);
            out.push_back(instr);
        } else {
            // No immediate, just nested instructions
            while (peek().type != TokenType::RPAREN) {
                parseInstruction(out);
            }
            expect(TokenType::RPAREN);
            out.push_back(Instruction(op));
        }
    } else {
        // Flat
        Token opToken = consume();
        Opcode op = mapOpcode(opToken.text);

        if (takesImmediate(op)) {
            out.push_back(parseImmediate(op));
        } else {
            out.push_back(Instruction(op));
        }
    }
}

bool Parser::takesImmediate(Opcode op) {
    switch (op) {
        case Opcode::I32_CONST:
        case Opcode::I64_CONST:
        case Opcode::F32_CONST:
        case Opcode::F64_CONST:
        case Opcode::LOCAL_GET:
        case Opcode::LOCAL_SET:
        case Opcode::LOCAL_TEE:
        case Opcode::GLOBAL_GET:
        case Opcode::GLOBAL_SET:
        case Opcode::BR:
        case Opcode::BR_IF:
        case Opcode::CALL:
            return true;
        default:
            return false;
    }
}

Instruction Parser::parseImmediate(Opcode op) {
    Token t = consume();
    if (op == Opcode::I32_CONST) return Instruction(op, (int32_t)std::stoi(t.text));
    if (op == Opcode::I64_CONST) return Instruction(op, (int64_t)std::stoll(t.text));
    // f32/f64 ...
    if (op == Opcode::F32_CONST) return Instruction(op, std::stof(t.text));
    if (op == Opcode::F64_CONST) return Instruction(op, std::stod(t.text));

    // Identifiers or indices
    if (t.type == TokenType::IDENTIFIER || t.type == TokenType::INTEGER) {
        std::string val = t.text;
        if (!val.empty() && val[0] == '$') val = val.substr(1);
        return Instruction(op, val); // Store as string, resolve later
    }

    throw std::runtime_error("Invalid immediate for opcode");
}

Opcode Parser::mapOpcode(const std::string& txt) {
    static const std::unordered_map<std::string, Opcode> map = {
        {"i32.const", Opcode::I32_CONST},
        {"i32.add", Opcode::I32_ADD},
        {"i32.sub", Opcode::I32_SUB},
        {"i32.mul", Opcode::I32_MUL},
        {"f64.const", Opcode::F64_CONST},
        {"f64.add", Opcode::F64_ADD},
        {"f64.sub", Opcode::F64_SUB},
        {"f64.mul", Opcode::F64_MUL},
        {"f64.div", Opcode::F64_DIV},
        {"local.get", Opcode::LOCAL_GET},
        {"local.set", Opcode::LOCAL_SET},
        {"call", Opcode::CALL},
        {"return", Opcode::RETURN},
        {"block", Opcode::BLOCK},
        {"loop", Opcode::LOOP},
        {"br", Opcode::BR},
        {"br_if", Opcode::BR_IF},
        // Add more as needed
    };
    if (map.count(txt)) return map.at(txt);
    // Fallback or ignore
    if (txt.find("store") != std::string::npos || txt.find("load") != std::string::npos) {
            // For strict correctness based on prompt "doesn't support linear memory"
            // But maybe "load" could be used if I mapped my ObjectStore to "memory" ...
            // No, prompt said "host functions".
            throw std::runtime_error("Memory instructions not supported");
    }
    return Opcode::NOP;
}

void Parser::skipSExpr() {
    int depth = 0; // We are called AFTER consuming the first ( ?
    // My logic in parseTopLevel: consume(, check, then if unknown skipSExpr.
    // I need to balance.
    // Let's assume skipSExpr starts AT the first token inside.
    // Actually, let's just make skipSExpr consume until balanced )
    // If I called it after consuming `(`, depth=1.
    depth = 1;
    while (depth > 0 && pos < tokens.size()) {
        Token t = consume();
        if (t.type == TokenType::LPAREN) depth++;
        if (t.type == TokenType::RPAREN) depth--;
    }
}
Import Parser::parseImport() {
    Import imp;
    // We already consumed 'import'
    // Format: "mod" "field" (func $alias ...
    Token modToken = consume();
    if (modToken.type != TokenType::STRING) throw std::runtime_error("Expected module name string");
    imp.module = modToken.text;

    Token fieldToken = consume();
    if (fieldToken.type != TokenType::STRING) throw std::runtime_error("Expected field name string");
    imp.field = fieldToken.text;

    expect(TokenType::LPAREN);
    Token kind = consume();
    if (kind.text != "func") {
        throw std::runtime_error("Only func imports are supported");
    }

    // Optional identifier
    if (peek().type == TokenType::IDENTIFIER) {
        std::string val = consume().text;
        if (!val.empty() && val[0] == '$') val = val.substr(1);
        imp.alias = val;
    }

    // Params and Results
    while (peek().type != TokenType::RPAREN) {
        expect(TokenType::LPAREN);
        Token inner = consume();
        if (inner.text == "param") {
            while (peek().type != TokenType::RPAREN) {
                 if (peek().type == TokenType::IDENTIFIER) {
                     consume(); // skip param name
                 }
                 if (peek().type == TokenType::KEYWORD) {
                     imp.paramTypes.push_back(consume().text);
                 }
            }
            expect(TokenType::RPAREN);
        } else if (inner.text == "result") {
            while (peek().type != TokenType::RPAREN) {
                imp.resultTypes.push_back(consume().text);
            }
            expect(TokenType::RPAREN);
        } else {
             throw std::runtime_error("Unexpected token in import func type");
        }
    }
    expect(TokenType::RPAREN); // close (func ...
    expect(TokenType::RPAREN); // close (import ...
    return imp;
}
