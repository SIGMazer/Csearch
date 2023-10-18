#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define TEXT_CAP 10000
#define ARRLEN(arr) (sizeof arr / sizeof arr[0])


char text[TEXT_CAP] = {0};
size_t text_i = 0;


typedef struct {
    char token[256];
}token;

static int next(void){
    return text[text_i++];
}

static int skip(void){
    uint8_t c;
    c = next();
    while(c >= 127 || isspace(c))
        c= next();

    return c;
}
static void scantoken(uint8_t c, char *buf){
    size_t i = 0 ;
    while(isspace(c))
        c = next();
    while((isdigit(c) || isalpha(c))){
        if(256 < i){
            fprintf(stderr,"Token too long\n");
            exit(1);
        }else{
            buf[i++] = c;
        }

        c = next();
    }
    buf[i] = '\0';
}


void parse_xhtml(FILE *file, char *text){
    char c;
    int i = 0;
    while((c = fgetc(file)) != EOF){
        if(c == '<'){
            while((c = fgetc(file)) != '>'){
                if(c == EOF){
                    break;
                }
            }
        }
        else{
            text[i] = c;
            i++;
        }
    }
    text[i] = '\0';
}

void parse_xhtml_trim(FILE *file, char *text){
    char c;
    int i = 0;
    int space = 0;
    int empty = 0;
    while((c = fgetc(file)) != EOF){
        if(c == '<'){
            while((c = fgetc(file)) != '>'){
                if(c == EOF){
                    break;
                }
            }
        }
        else{
            if(c == '\n'){
                if(empty == 0){
                    text[i] = c;
                    i++;
                    empty = 1;
                }
            }
            else if(c == ' '){
                if(space == 0){
                    text[i] = c;
                    i++;
                    space = 1;
                }
            }
            else{
                text[i] = c;
                i++;
                space = 0;
                empty = 0;
            }
        }
    }
    text[i] = '\0';
}

void token_dump(){
    uint8_t c  = 0;
    char buf[256] ;
    size_t t = ARRLEN(text);
    while(text_i < t){
        c = skip();
        scantoken(c, buf);
        puts(buf);
        memset(buf, 0, 256);
    }
}


int main(void){

    FILE *file = fopen("./docs.gl/el3/cos.xhtml", "r");  

    parse_xhtml_trim(file, text);
    
    token_dump();


    return 0;
}
