#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <vector>

#include "types.h"
#include "array.h"
#include "simple_string.h"
#include "parser.h"
#include "expect.h"

#define BITS_IN_BYTE 8
#define BITS_SHORT sizeof(uint16)*BITS_IN_BYTE
#define MEMORY_SIZE (1 << BITS_SHORT)

#define Bytes(N) N
#define KiloBytes(N) Bytes(N*1024)
#define MegaBytes(N) KiloBytes(N*1024)
#define GigaBytes(N) MegaBytes(N*1024)

String ReadEntireFileIntoMemory(Array<byte> *memory, char * path) {
    String result = {};

    FILE* platform_file = fopen(path, "r");
    fseek(platform_file, 0, SEEK_END);
    result.length = ftell(platform_file);
    fseek(platform_file, 0, SEEK_SET);
    result.data = AllocBytes(memory, result.length);
    fread(result.data, result.length, 1, platform_file);
    fclose(platform_file);

    return result;
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
    String input = ReadEntireFileIntoMemory(&byte_array, "./main.sbasm");
    Array<LineOfCode> lines = CreateArray<LineOfCode>(1000);
    Tokenizer tokenizer = { input.data };
    byte * end = input.data + input.length;
    
    while (tokenizer.at < end) {
        LineOfCode line = {};
        line.start = tokenizer.at;
        line.end = FindEndOfLine(&tokenizer);
        tokenizer.at += 1;
        line.line_number_in_file = lines.count + 1;
        AppendItem(&lines, line);
    }


    Array<Token> token_array = CreateArray<Token>(KiloBytes(1)); 

    for (uint32 line_number = 0; line_number < lines.count; line_number++) {

        LineOfCode * line = lines[line_number];
        static uint64 last_token_index_on_line = 0;
        line->token_index_from = last_token_index_on_line;

        Tokenizer tokenizer = { line->start };
        if (!IsNewLine(tokenizer.at)) {
            while(tokenizer.at < line->end) {
                EatAllWhiteSpace(&tokenizer);
                Token found = ParseToken(&tokenizer, *line);
                AppendItem(&token_array, found);
                last_token_index_on_line += 1;
            }
        }

        line->token_index_to = last_token_index_on_line;
    }

    for (uint32 line_number = 0;
         line_number < lines.count;
         line_number++) {
        LineOfCode * line = lines[line_number];
        
        if (line->token_index_to > line->token_index_from) {
            
            for (uint32 token_index = line->token_index_from;
                 token_index < line->token_index_to;
                 token_index++)
            {
                Token * current = token_array[token_index];
                assert(current->string.length < 20);
                switch (current->type)
                {
                    case TOKEN_TYPE_SECTION:
                        {
                            Token * section_identifer = token_array[token_index+1];

                            Expect(section_identifer->type == TOKEN_TYPE_DATA_SECTION || section_identifer->type == TOKEN_TYPE_CODE_SECTION,
                                "Expected section identifier to be either 'data' or 'code' instead got '%.*s (%d:%d-%d)'\n",
                                section_identifer->string.length,
                                section_identifer->string.data,
                                section_identifer->line_number_in_file,
                                section_identifer->column_number_in_file,
                                section_identifer->column_number_in_file + section_identifer->string.length
                            );

                            if (section_identifer->type == TOKEN_TYPE_DATA_SECTION) {

                                Expect(!State.data_section_declared,
                                    "You have already delcared a data section at (%d:%d-%d) remove duplicate data section at (%d:%d-%d)",
                                    State.data_section_token.line_number_in_file,
                                    State.data_section_token.column_number_in_file,
                                    State.data_section_token.column_number_in_file + State.data_section_token.string.length
                                );
                                
                                State.data_section_token = *section_identifer;
                                State.data_section_declared = true;
                                State.active_section = SECTION_TYPE_DATA;

                            } else if (section_identifer->type == TOKEN_TYPE_CODE_SECTION) {

                                Expect(!State.code_section_declared,
                                    "You have already delcared a code section at (%d:%d-%d) remove duplicate code section at (%d:%d-%d)",
                                    State.code_section_token.line_number_in_file,
                                    State.code_section_token.column_number_in_file,
                                    State.code_section_token.column_number_in_file + State.code_section_token.string.length
                                );

                                State.code_section_token = *section_identifer;
                                State.code_section_declared = true;
                                State.active_section = SECTION_TYPE_CODE;
                            }
                        }
                    break;
                    case '@':
                        {
                            Token * identifer = token_array[token_index+1];

                            if (State.active_section == SECTION_TYPE_DATA) 
                            {
                                Expect(identifer->type == TOKEN_TYPE_IDENTIFIER,
                                    "Expected %s after @ but got %s\n",
                                    GetTokenTypeName(TOKEN_TYPE_IDENTIFIER),
                                    GetTokenTypeName(identifer->type)
                                );
                            } 
                            else if (State.active_section == SECTION_TYPE_CODE) 
                            {
                            }
                        }
                    break;
                    case ':':
                        {
                            Token * label_identifier = token_array[token_index-1];

                            
                        }
                    break;
                    default:
                        {
                            {}
                        }
                    break;
                }
            }
        }
    }

    DestoryArray(&token_array);
    DestoryArray(&byte_array);
    DestoryArray(&lines);

    return 0;
}
