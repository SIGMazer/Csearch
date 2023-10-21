#include <stdint.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#define TEXT_CAP 1024 
#define TOKEN_CAP 256
#define PUTBACK_LEN 32
#define ARRLEN(arr) (sizeof arr / sizeof arr[0])


typedef struct {
    char contant[TEXT_CAP];
    size_t idx;
} Text;

typedef struct{
    unsigned char token[TOKEN_CAP];
    size_t idx;
} Token;

Text text = {.idx = 0};
size_t clen = 0;
int parse_xhtml(FILE *file){
    char c;
    bool in_tag = false;
    bool left_space = true;
    bool end_tag = false;
    while((c = fgetc(file)) != EOF){
        if(c == '<'){
            in_tag = true;
        }
        else if(c == '/'){
            end_tag = true;
        }
        else if(c == '>'){
            in_tag = false;
            if (end_tag){
                text.contant[text.idx]= '\0';
                return 0; // return 0 if end tag
            }
        }
        else if(!in_tag){
            if(!isspace(c) || !left_space){
                if(left_space) left_space = false;
                
                text.contant[text.idx]= c;
                assert(text.idx < TEXT_CAP);
                text.idx++;
            }
        }
    }
    return EOF; // return EOF if no more char
}

int read_next_tag(FILE *file){
    int c = 0;
    do{
        c = parse_xhtml(file);
    }while(c != EOF && text.idx == 0);
    return c; // if no more tags 
}

char putback[PUTBACK_LEN] = {0};
size_t putback_idx = 0;

Token read_alpha(){
    int c = text.contant[clen];
    Token token = {.idx = 0};
    while(isalpha(c) || isdigit(c) || c == '_'){
        token.token[token.idx] = c;
        assert(token.idx < TOKEN_CAP);
        token.idx++;
        clen++;
        c = text.contant[clen];
    }
    return token;
}

Token read_number(){
    int c = text.contant[clen];
    Token token = {.idx = 0};
    while(isdigit(c)){
        token.token[token.idx] = c;
        assert(token.idx < TOKEN_CAP);
        token.idx++;
        clen++;
        c = text.contant[clen];
        if(c == '.'){
            if(!isdigit(text.contant[clen+1]))
                continue;
            token.token[token.idx] = c;
            assert(token.idx < TOKEN_CAP);
            token.idx++;
            clen++;
            c = text.contant[clen];
        }
    }
    return token;
}

Token lexer(){
    uint8_t c= text.contant[clen]; 
    Token token = {.idx =0};

    while(isspace(c)){
        clen++;
        c = text.contant[clen];
    }

    if(isalpha(c)) token = read_alpha();
    else if(isdigit(c)) token = read_number();
    else {
        token.token[token.idx] = c;
        assert(token.idx < TOKEN_CAP);
        token.idx++;
        clen++;
    }

    return token;
}


int main(void){
    FILE *file = fopen("./docs.gl/el3/cos.xhtml", "r");  
    int c; 
    while((c = read_next_tag(file)) != EOF){
        Token token = lexer();
        printf("%s\n", token.token);
        memset(token.token, 0, sizeof token.token);
        memset(text.contant, 0, sizeof text.contant);
        token.idx = 0;
        text.idx = 0;
        clen = 0;
    }

    fclose(file);
    return 0;
}
