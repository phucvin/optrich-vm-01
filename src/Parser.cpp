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
        } else if (fieldName.text == "type") {
            consume();
            mod.types.push_back(parseType());
        } else if (fieldName.text == "table") {
            consume();
            mod.tables.push_back(parseTable());
        } else if (fieldName.text == "elem") {
            consume();
            mod.elements.push_back(parseElem());
        } else if (fieldName.text == "string") {
            consume();
            StringDefinition strDef;
            if (peek().type == TokenType::IDENTIFIER) {
                std::string raw = consume().text;
                if (!raw.empty() && raw[0] == '$') raw = raw.substr(1);
                strDef.name = raw;
            } else {
                throw std::runtime_error("Expected identifier for string definition");
            }
            if (peek().type == TokenType::STRING) {
                strDef.value = consume().text;
            } else {
                throw std::runtime_error("Expected string value for string definition");
            }
            mod.strings.push_back(strDef);
            expect(TokenType::RPAREN);
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
            // Only S-expressions supported
            throw std::runtime_error("Flat instructions are not supported. Found token: " + peek().text);
        }
    }
    expect(TokenType::RPAREN);
    return func;
}

Type Parser::parseType() {
    Type t;
    if (peek().type == TokenType::IDENTIFIER) {
        std::string raw = consume().text;
        if (!raw.empty() && raw[0] == '$') raw = raw.substr(1);
        t.name = raw;
    }

    expect(TokenType::LPAREN);
    if (consume().text != "func") throw std::runtime_error("Expected func in type definition");

    while (peek().type == TokenType::LPAREN) {
        consume(); // (
        Token inner = consume();
        if (inner.text == "param") {
             while (peek().type != TokenType::RPAREN) {
                 if (peek().type == TokenType::IDENTIFIER) {
                     consume(); // skip name
                 }
                 if (peek().type == TokenType::KEYWORD) {
                     t.paramTypes.push_back(consume().text);
                 }
             }
        } else if (inner.text == "result") {
             while (peek().type != TokenType::RPAREN) {
                 t.resultTypes.push_back(consume().text);
             }
        } else {
             throw std::runtime_error("Unexpected token in type definition");
        }
        expect(TokenType::RPAREN);
    }
    expect(TokenType::RPAREN); // End func
    expect(TokenType::RPAREN); // End type
    return t;
}

Table Parser::parseTable() {
    Table tbl;
    // (table $name? min max? funcref)
    if (peek().type == TokenType::IDENTIFIER) {
        std::string raw = consume().text;
        if (!raw.empty() && raw[0] == '$') raw = raw.substr(1);
        tbl.name = raw;
    }

    Token t1 = consume();
    if (t1.type != TokenType::INTEGER) throw std::runtime_error("Expected min size for table");
    tbl.min = std::stoi(t1.text);

    if (peek().type == TokenType::INTEGER) {
        tbl.max = std::stoi(consume().text);
    } else {
        tbl.max = tbl.min;
    }

    if (consume().text != "funcref") throw std::runtime_error("Expected funcref");
    expect(TokenType::RPAREN);
    return tbl;
}

ElementSegment Parser::parseElem() {
    ElementSegment elem;
    elem.tableIndex = 0; // Default

    // (elem (i32.const offset) $f1 ...)
    expect(TokenType::LPAREN);
    // The offset must be i32.const
    if (consume().text != "i32.const") throw std::runtime_error("Expected i32.const in elem offset");
    Token off = consume();
    elem.offset = Instruction(Opcode::I32_CONST, (int32_t)std::stoi(off.text));
    expect(TokenType::RPAREN);

    while (peek().type != TokenType::RPAREN) {
        Token funcName = consume();
        std::string name = funcName.text;
        if (!name.empty() && name[0] == '$') name = name.substr(1);
        elem.functionNames.push_back(name);
    }
    expect(TokenType::RPAREN);
    return elem;
}

void Parser::parseInstruction(std::vector<Instruction>& out) {
    if (peek().type == TokenType::LPAREN) {
        // Folded: (opcode arg1 arg2)
        consume(); // (
        Token opToken = consume();
        Opcode op = mapOpcode(opToken.text);

        // Special handling for Block/Loop to preserve structure
        if (op == Opcode::BLOCK || op == Opcode::LOOP) {
             Instruction instr = parseImmediate(op);
             out.push_back(instr); // START

             // Nested instructions
             while (peek().type != TokenType::RPAREN) {
                 parseInstruction(out);
             }
             expect(TokenType::RPAREN);
             out.push_back(Instruction(Opcode::END)); // END
        }
        else if (op == Opcode::CALL_INDIRECT) {
            // (call_indirect (type $t) (arg1) ... (index))
            // This is folded. In RPN: arg1 ... index call_indirect $t
            // We need to parse immediate (type) which is usually (type $t)
            Instruction instr(Opcode::CALL_INDIRECT, std::string(""));

            // Parse type annotation
            if (peek().type == TokenType::LPAREN) {
                // Expect (type $t)
                consume();
                if (consume().text != "type") throw std::runtime_error("Expected type in call_indirect");
                Token typeName = consume();
                std::string val = typeName.text;
                if (!val.empty() && val[0] == '$') val = val.substr(1);
                instr.operand = val;
                expect(TokenType::RPAREN);
            } else {
                 throw std::runtime_error("Expected type annotation for call_indirect");
            }

            // Parse args + index
            while (peek().type != TokenType::RPAREN) {
                parseInstruction(out);
            }
            expect(TokenType::RPAREN);
            out.push_back(instr);
        }
        // Standard RPN for other instructions with immediates/operands
        else if (takesImmediate(op)) {
            // The immediate is the next token(s)
            Instruction instr = parseImmediate(op);

            // Any subsequent tokens are nested instructions (operands)
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
        throw std::runtime_error("Flat instructions are not supported. Found token: " + peek().text);
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
        case Opcode::BLOCK:
        case Opcode::LOOP:
        case Opcode::STRING_CONST:
            return true;
        default:
            return false;
    }
}

Instruction Parser::parseImmediate(Opcode op) {
    Token t = consume();
    if (op == Opcode::I32_CONST) return Instruction(op, (int32_t)std::stoi(t.text));
    if (op == Opcode::I64_CONST) return Instruction(op, (int64_t)std::stoll(t.text));
    if (op == Opcode::F32_CONST) return Instruction(op, std::stof(t.text));
    if (op == Opcode::F64_CONST) return Instruction(op, std::stod(t.text));

    // Identifiers or indices
    if (t.type == TokenType::IDENTIFIER || t.type == TokenType::INTEGER) {
        std::string val = t.text;
        if (!val.empty() && val[0] == '$') val = val.substr(1);
        return Instruction(op, val); // Store as string, resolve later
    }

    throw std::runtime_error("Invalid immediate for opcode: " + t.text);
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
        {"call_indirect", Opcode::CALL_INDIRECT},
        {"return", Opcode::RETURN},
        {"block", Opcode::BLOCK},
        {"loop", Opcode::LOOP},
        {"br", Opcode::BR},
        {"br_if", Opcode::BR_IF},
        {"end", Opcode::END},
        {"i32.eq", Opcode::I32_EQ},
        {"i32.ne", Opcode::I32_NE},
        {"i32.lt_s", Opcode::I32_LT_S},
        {"i32.gt_s", Opcode::I32_GT_S},
        {"i32.le_s", Opcode::I32_LE_S},
        {"i32.ge_s", Opcode::I32_GE_S},
        {"string.const", Opcode::STRING_CONST},
    };
    if (map.count(txt)) return map.at(txt);
    if (txt.find("store") != std::string::npos || txt.find("load") != std::string::npos) {
            throw std::runtime_error("Memory instructions not supported");
    }
    return Opcode::NOP;
}

void Parser::skipSExpr() {
    int depth = 1;
    while (depth > 0 && pos < tokens.size()) {
        Token t = consume();
        if (t.type == TokenType::LPAREN) depth++;
        if (t.type == TokenType::RPAREN) depth--;
    }
}

Import Parser::parseImport() {
    Import imp;
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

    if (peek().type == TokenType::IDENTIFIER) {
        std::string val = consume().text;
        if (!val.empty() && val[0] == '$') val = val.substr(1);
        imp.alias = val;
    }

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
    expect(TokenType::RPAREN);
    expect(TokenType::RPAREN);
    return imp;
}
