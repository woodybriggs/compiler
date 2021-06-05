#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "types.hh"
#include "array.hh"

#define BITS_IN_BYTE 8
#define BITS_SHORT sizeof(uint16)*BITS_IN_BYTE
#define MEMORY_SIZE (1 << BITS_SHORT)

#define Bytes(N) N
#define KiloBytes(N) Bytes(N*1024)
#define MegaBytes(N) KiloBytes(N*1024)
#define GigaBytes(N) MegaBytes(N*1024)

struct Buffer {
    byte * data;
    uint32 length;

    bool operator==(const char * comparison_string) {
        bool result = true;

        uint32 comparison_string_length = strlen(comparison_string);
        if (length != comparison_string_length) result = false;
        else {
            uint32 c = 0;
            while (c < length) {
                if (data[c] != comparison_string[c]) result = false;
                c += 1;
            }
        }
        return result;
    }
};

enum TokenType {
    // ... reserved for ASCII Token Types
    TOKEN_TYPE_UNKNOWN = 256,
    TOKEN_TYPE_INTEGER_LITERAL,
    TOKEN_TYPE_IDENTIFIER,
};

const char * GetTokenTypeName(TokenType type) {
    if (type == TOKEN_TYPE_INTEGER_LITERAL) return "Integer";
    else if (type == TOKEN_TYPE_IDENTIFIER) return "Identifier";
    else if ((char) type == '.')            return "Section Decleration";
    else if ((char) type == '@')            return "Memory Constant Decleration";
    else if ((char) type == ':')            return "Label Body";
    else return (const char *) type;
}

TokenType GetTokenType(byte * c) {
    if (*c >= '0' && *c <= '9')      return TOKEN_TYPE_INTEGER_LITERAL;
    else if (*c >= 'A' && *c <= 'z') return TOKEN_TYPE_IDENTIFIER;
    else return (TokenType) *c;
}

struct LineOfCode {
    uint32 line_number_in_file;
    byte * start;
    byte * end;
};

struct Token {
    TokenType type;
    Buffer string;
    uint32 line_number_in_file;
    uint32 column_number_in_file;
};

struct Tokenizer {
    byte * at;
};

Buffer ReadEntireFileIntoMemory(Array<byte> *memory, char * path) {
    Buffer result = {};

    FILE* platform_file = fopen(path, "r");
    fseek(platform_file, 0, SEEK_END);
    result.length = ftell(platform_file);
    fseek(platform_file, 0, SEEK_SET);
    result.data = AllocBytes(memory, result.length);
    fread(result.data, result.length, 1, platform_file);
    fclose(platform_file);

    return result;
}


bool32 IsWhiteSpace(byte * to_test) {
         if (*to_test == ' ')  return true;
    else if (*to_test == '\t') return true;
    else if (*to_test == '\r') return true;
    else if (*to_test == '\n') return true;
    else                       return false;
}

void EatAllWhiteSpace(Tokenizer * tokenizer) {
    while (IsWhiteSpace(tokenizer->at)) {
        tokenizer->at += 1;
    }
}

byte * FindEndOfToken(byte * token_start, TokenType token_type) {
    byte * token_end = token_start + 1;
    while (GetTokenType(token_end) == token_type && (char) *token_end != '\0') {
        token_end++;
    }
    return token_end;
}

Token ParseToken(Tokenizer * tokenizer, LineOfCode line) {
    Token result = {};
    byte * token_start = tokenizer->at;
    result.type = GetTokenType(tokenizer->at);
    result.line_number_in_file = line.line_number_in_file;
    result.column_number_in_file = token_start - line.start;

    switch (result.type)
    {
        case '.': 
        {
            result.string.data = token_start;
            result.string.length = 1;
            tokenizer->at += 1;
        } break;
        case ':': 
        {
            result.string.data = token_start;
            result.string.length = 1;
            tokenizer->at += 1;
        } break;
        case '@': 
        {
            result.string.data = token_start;
            result.string.length = 1;
            tokenizer->at += 1;
        } break;
        case TOKEN_TYPE_INTEGER_LITERAL:
        {
            result.string.data = tokenizer->at;
            result.string.length = FindEndOfToken(tokenizer->at, TOKEN_TYPE_INTEGER_LITERAL) - token_start;
            tokenizer->at += result.string.length;
        } break;
        case TOKEN_TYPE_IDENTIFIER:
        {
            result.string.data = tokenizer->at;
            result.string.length = FindEndOfToken(tokenizer->at, TOKEN_TYPE_IDENTIFIER) - token_start;
            tokenizer->at += result.string.length;
        } break;
        default:
        {
            {}
        }
    }

    return result;
}

void Expect(bool assertion, const char * format_string, ...) {
    if (!assertion) {
        va_list args;
        va_start(args, format_string);
        vfprintf(stderr, format_string, args);
        va_end(args);
        exit(SIGTERM);
    }
}

inline bool isNewLine(byte * start) {
    if (*start == '\n') return true;
    else return false;
}

inline bool isNullTerminator(byte * start) {
    if (*start == '\0') return true;
    else return false;
}


byte * FindEndOfLine(Tokenizer * tokenizer) {
    while (!isNewLine(tokenizer->at) && !isNullTerminator(tokenizer->at)){
        tokenizer->at += 1;
    }
    return tokenizer->at;
}


enum SectionType {
    SECTION_TYPE_DATA,
    SECTION_TYPE_CODE
};

struct ParseState {
    bool data_section_declared;
    Token data_section_token;
    bool code_section_declared;
    Token code_section_token;
    SectionType active_section;
} State = {};

int main(int argc, char** argv) {

    uint16 memory[MEMORY_SIZE] = {};
    memory[0xFFFF] = 1;

    Array<byte> byte_array = CreateArray<byte>(MegaBytes(4));
    Buffer input = ReadEntireFileIntoMemory(&byte_array, "./main.sbasm");
    Array<LineOfCode> lines = CreateArray<LineOfCode>(1000);
    Tokenizer tokenizer = { input.data };
    byte * end = input.data + input.length;
    
    while (tokenizer.at < end) {
        LineOfCode line = {};
        line.start = tokenizer.at;
        line.end = FindEndOfLine(&tokenizer);
        tokenizer.at += 1;
        line.line_number_in_file = lines.count+1;
        AppendItem(&lines, line);
    }


    Array<Token> token_array = CreateArray<Token>(KiloBytes(1));

    for (uint32 line_number = 0; line_number < lines.count; line_number++) {

        LineOfCode line = lines[line_number];

        Tokenizer tokenizer = { line.start };
        if (!isNewLine(tokenizer.at)) {
            while(tokenizer.at < line.end) {
                EatAllWhiteSpace(&tokenizer);
                Token found = ParseToken(&tokenizer, line);
                AppendItem(&token_array, found);
            }
        }
    }

    uint32 token_index = 0;
    while (token_index < token_array.count) {

        Token current = token_array[token_index];

        switch (current.type)
        {
            case '.':
                {
                    Token section_identifer = token_array[token_index+1];

                    Expect(section_identifer.type == TOKEN_TYPE_IDENTIFIER, 
                        "Expected %s after section decleration (.) instead got %s (%d:%d-%d)\n",
                        GetTokenTypeName(TOKEN_TYPE_IDENTIFIER),
                        GetTokenTypeName(section_identifer.type),
                        section_identifer.line_number_in_file,
                        section_identifer.column_number_in_file,
                        section_identifer.column_number_in_file + section_identifer.string.length
                    );

                    Expect((section_identifer.string == "data" || section_identifer.string == "code"),
                        "Expected section identifier to be either 'data' or 'code' instead got '%.*s (%d:%d-%d)'\n",
                        section_identifer.string.length,
                        section_identifer.string.data,
                        section_identifer.line_number_in_file,
                        section_identifer.column_number_in_file,
                        section_identifer.column_number_in_file + section_identifer.string.length
                    );

                    if (section_identifer.string == "data") {

                        Expect(!State.data_section_declared,
                            "You have already delcared a data section at (%d:%d-%d) remove duplicate data section at (%d:%d-%d)",
                            State.data_section_token.line_number_in_file,
                            State.data_section_token.column_number_in_file,
                            State.data_section_token.column_number_in_file + State.data_section_token.string.length
                        );
                        
                        State.data_section_token = section_identifer;
                        State.data_section_declared = true;
                        State.active_section = SECTION_TYPE_DATA;
                    } else if (section_identifer.string == "code") {

                        Expect(!State.code_section_declared,
                            "You have already delcared a code section at (%d:%d-%d) remove duplicate code section at (%d:%d-%d)",
                            State.code_section_token.line_number_in_file,
                            State.code_section_token.column_number_in_file,
                            State.code_section_token.column_number_in_file + State.code_section_token.string.length
                        );

                        State.code_section_token = section_identifer;
                        State.code_section_declared = true;
                        State.active_section = SECTION_TYPE_CODE;
                    }
                } 
            break;
            case '@': 
                {
                    Token identifer = token_array[token_index+1];

                    if (State.active_section == SECTION_TYPE_DATA) {
                        Expect(identifer.type == TOKEN_TYPE_IDENTIFIER, 
                            "Expected %s after @ but got %s\n", 
                            GetTokenTypeName(TOKEN_TYPE_IDENTIFIER),
                            GetTokenTypeName(identifer.type)
                        );

                        Token integer_literal = token_array[token_index+2];
                        Expect(integer_literal.type == TOKEN_TYPE_INTEGER_LITERAL,
                            "Expected %s after '@%.*s' but got %s '%.*s'\n",
                            GetTokenTypeName(TOKEN_TYPE_INTEGER_LITERAL),
                            identifer.string.length,
                            identifer.string.data,
                            GetTokenTypeName(integer_literal.type),
                            integer_literal.string.length,
                            integer_literal.string.data
                        );
                    } else if (State.active_section == SECTION_TYPE_CODE) {
                        
                    }
                } 
            break;
            case ':':
                {
                    Token label_identifier = token_array[token_index-1];

                    
                }
            break;
            default:
                {
                    {}
                }
            break;
        }

        token_index++;
    }

    DestoryArray(&token_array);
    DestoryArray(&byte_array);
    DestoryArray(&lines);

    return 0;
}