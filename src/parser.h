
#include "types.h"
#include "simple_string.h"
#include "expect.h"

#ifndef TOKENS_H
#define TOKENS_H

struct Tokenizer {
    char * at;
};

struct LineOfCode {
    uint64 line_number_in_file;
    byte * start;
    byte * end;
    uint64 token_index_from;
    uint64 token_index_to;
};

enum TokenType {
    // ... reserved for ASCII Token Types
    TOKEN_TYPE_UNKNOWN = 256,

    // Basic Token Types
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_NUMERIC,

    // Typed Token Types
    TOKEN_TYPE_BINARY_NUMBER,
    TOKEN_TYPE_HEX_NUMBER,
    TOKEN_TYPE_INTEGER_NUMBER,
    TOKEN_TYPE_FLOAT_NUMBER,
    
    // Reserved Syntax Words Token Types
    TOKEN_TYPE_SECTION,
    TOKEN_TYPE_DATA_SECTION,
    TOKEN_TYPE_CODE_SECTION
};

struct Token {
    TokenType type;
    String string;
    uint64 line_number_in_file;
    uint64 column_number_in_file;
};

TokenType GetTokenType(char * c) {
         if (*c >= '0' && *c <= '9') return TOKEN_TYPE_NUMERIC;
    else if (*c >= 'A' && *c <= 'z') return TOKEN_TYPE_IDENTIFIER;
    else                             return (TokenType) *c;
}

const char * GetTokenTypeName(TokenType type) {
         if (type == TOKEN_TYPE_NUMERIC)         return "Numeric";
    else if (type == TOKEN_TYPE_IDENTIFIER)      return "Identifier";
    else if ((char) type == '@')                 return "Memory Constant Decleration";
    else if ((char) type == ':')                 return "Label Body";
    else                                         return (const char *) type;
}

bool IsWhiteSpace(char * to_test) {
         if (*to_test == ' ')  return true;
    else if (*to_test == '\t') return true;
    else if (*to_test == '\r') return true;
    else if (*to_test == '\n') return true;
    else                       return false;
}

inline bool IsNumber(char * to_test) {
    if (*to_test >= '0' && *to_test <= '9') return true;
    else return false;
}

inline bool IsAlpha(char * to_test) {
    if (*to_test >= 'A' && *to_test <= 'z') return true;
    else return false;
}

inline bool IsAlphaNumeric(char * to_test) {
    if (IsAlpha(to_test) || IsNumber(to_test)) return true;
    else return false;
}

inline bool IsDecimalPoint(char * to_test) {
    if (*to_test == '.') return true;
    else return false;
}

inline bool IsBinary(char * to_test) {
    if (*to_test == '0' || *to_test == '1') return true;
    else return false;
}

inline bool IsHex(char * to_test) {
    if ((*to_test >= 'A' && *to_test <= 'F') || IsNumber(to_test)) return true;
    else return false; 
}

void EatAllWhiteSpace(Tokenizer * tokenizer) {
    while (IsWhiteSpace(tokenizer->at)) {
        tokenizer->at += 1;
    }
}

char * FindEndOfToken(char * token_start, TokenType token_type) {
    char * token_end = token_start + 1;
    while (GetTokenType(token_end) == token_type && (char) *token_end != '\0') {
        token_end++;
    }
    return token_end;
}

inline bool IsNewLine(char * start) {
    if (*start == '\n') return true;
    else return false;
}

inline bool IsNullTerminator(char * start) {
    if (*start == '\0') return true;
    else return false;
}


char * FindEndOfLine(Tokenizer * tokenizer) {
    while (!IsNewLine(tokenizer->at) && !IsNullTerminator(tokenizer->at)){
        tokenizer->at += 1;
    }
    return tokenizer->at;
}

Token ParseIdentifier(Tokenizer *tokenizer, LineOfCode line) {
    Token identifer = {};
    identifer.line_number_in_file = line.line_number_in_file;
    identifer.column_number_in_file = tokenizer->at - line.start;

    char * token_start = tokenizer->at += 1;
    while(true)
    {
        if (IsAlphaNumeric(tokenizer->at)) tokenizer->at += 1;
        else if (IsNewLine(tokenizer->at) || IsNullTerminator(tokenizer->at)) break;
        else break;
    }
    identifer.string.data = token_start;
    identifer.string.length = tokenizer->at - token_start;

    if (identifer.string == "section") identifer.type = TOKEN_TYPE_SECTION;
    if (identifer.string == "data")    identifer.type = TOKEN_TYPE_DATA_SECTION;
    if (identifer.string == "code")    identifer.type = TOKEN_TYPE_CODE_SECTION;
    return identifer;
}

Token ParseNumeric(Tokenizer *tokenizer, LineOfCode line) {

    Token numeric = {};
    numeric.line_number_in_file = line.line_number_in_file;
    numeric.column_number_in_file = tokenizer->at - line.start;

    char * token_start = tokenizer->at;

    if (token_start[0] == '0' && token_start[1] == 'b')
    {
        numeric.type = TOKEN_TYPE_BINARY_NUMBER;
        tokenizer->at += 2;
        while (IsBinary(tokenizer->at)) tokenizer->at += 1;
    }
    else if (token_start[0] == '0' && token_start[1] == 'x')
    {
        numeric.type = TOKEN_TYPE_HEX_NUMBER;
        tokenizer->at += 2;
        while (IsHex(tokenizer->at)) tokenizer->at += 1;
    }
    else 
    {
        bool parsing_int_or_float = true;
        bool is_float = false;
        while (parsing_int_or_float)
        {
            if (IsNumber(tokenizer->at)) tokenizer->at += 1;
            else if (IsDecimalPoint(tokenizer->at))
            {
                Expect(!is_float, 
                    "Whoops you seem to have too many decimal points at (%d:%d-%d) '%.*s'\n",
                    numeric.line_number_in_file,
                    numeric.column_number_in_file,
                    numeric.column_number_in_file + (tokenizer->at - token_start),
                    (tokenizer->at - token_start),
                    token_start
                );
                is_float = true;
                tokenizer->at += 1;
            }
            else if (IsWhiteSpace(tokenizer->at) || IsNullTerminator(tokenizer->at)) {
                parsing_int_or_float = false;
            }
        }

        numeric.type = is_float ? TOKEN_TYPE_FLOAT_NUMBER : TOKEN_TYPE_INTEGER_NUMBER;
    }

    numeric.string.data = token_start;
    numeric.string.length = (tokenizer->at - token_start);
    return numeric;
}

Token ParseToken(Tokenizer * tokenizer, LineOfCode line) {
    Token result = {};
    byte * token_start = tokenizer->at;

    if (IsAlpha(tokenizer->at)) result = ParseIdentifier(tokenizer, line);
    else if (IsNumber(tokenizer->at)) result = ParseNumeric(tokenizer, line);
    else {
        result.string.data = token_start;
        result.string.length = 1;
        result.line_number_in_file = line.line_number_in_file;
        result.column_number_in_file = token_start - line.start;
        tokenizer->at += 1;
    }

    return result;
}

#endif
