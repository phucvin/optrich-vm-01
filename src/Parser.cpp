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
            // (import "mod" "name" (func $name ...))
            // Very basic skipper for now, or partial parser
            // Actually, we need to handle imports to register host functions
            // But for the prototype, maybe we can ignore explicit import declarations
            // and just assume `call $name` works if registered?
            // WAT usually requires imports to be declared.
            // Let's skip and assume user writes correct WAT, but we need to track import indexes?
            // For simplicity, I'll skip parsing the details but skip the block.
            // The prompt says "input is in WAT format".
            // I'll skip imports for now to focus on the "runtime" logic.
            pos = checkpoint + 1; // +1 to skip (
            skipSExpr();
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
        func.name = consume().text;
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
                    if (peek().type == TokenType::IDENTIFIER) name = consume().text;
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
                        if (peek().type == TokenType::IDENTIFIER) name = consume().text;
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
        return Instruction(op, t.text); // Store as string, resolve later
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
