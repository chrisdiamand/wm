
#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>

typedef enum
{
    TOK_NAME,
    TOK_INT,
    TOK_STRING,
    TOK_EQUALS,
    TOK_COMMA,
    TOK_EOF
} TokenType;

typedef struct
{
    TokenType type;
    char *value;
} Token;

typedef struct
{
    FILE *fp;
    char *filename;
    char *str;
    unsigned long int pos, line_number;
} ScannerInput;

char *TokenName(TokenType);

ScannerInput *ScannerInputFile(FILE *);
ScannerInput *ScannerInputString(char *);

Token ScanToken(ScannerInput *);

#endif

