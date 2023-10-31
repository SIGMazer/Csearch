#include <stdint.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>


#define TEXT_CAP 1024 
#define TOKEN_CAP 256
#define PUTBACK_LEN 32
#define ARRLEN(arr) (sizeof arr / sizeof arr[0])

typedef struct {
    char content[TEXT_CAP];
    size_t idx;
} Text;

typedef struct{
    char *token;
    size_t count;
} Token;

typedef struct{
    Token token;
    size_t value;
    bool ocuppied;
} Tokenfreq;

typedef struct{
    char path[256];
} Path;

typedef struct{
    Path *items;
    size_t count;
    size_t capacity;
} Dires;
typedef struct{
    Tokenfreq *items;
    size_t count;
    size_t capacity;
} Tokenfreqs;

#define DA_INIT_CAP 256
#define da_append(da, item)                                                          \
    do {                                                                                 \
        if ((da)->count >= (da)->capacity) {                                             \
            (da)->capacity = (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity*2;   \
            (da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items)); \
            assert((da)->items != NULL);                       \
        }                                                                                \
                                                                                         \
        (da)->items[(da)->count++] = (item);                                             \
    } while (0)



bool token_eq(Token a, Token b){
    if(a.count != b.count) return false;
    else{
       return memcmp(a.token, b.token, a.count) == 0;
    }
}

uint32_t djb2(uint8_t *buf, size_t buf_size)
{
    uint32_t hash = 5381;

    for (size_t i = 0; i < buf_size; ++i) {
        hash = ((hash << 5) + hash) + (uint32_t)buf[i]; /* hash * 33 + c */
    }

    return hash;
}





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
                text.content[text.idx]= '\0';
                return 0; // return 0 if end tag
            }
        }
        else if(!in_tag){
            if(!isspace(c) || !left_space){
                if(left_space) left_space = false;
                
                text.content[text.idx]= c;
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


Token read_alpha(){
    int c = text.content[clen];
    Token token = {.count = 0};
    char* tmp = malloc(sizeof(char)*TOKEN_CAP);
    while(isalpha(c) || isdigit(c) || c == '_'){
        tmp[token.count] = toupper(c);
        assert(token.count < TOKEN_CAP);
        token.count++;
        clen++;
        c = text.content[clen];
    }
    token.token = tmp;
    return  token ;
}

Token read_number(){
    int c = text.content[clen];
    Token token = {.count = 0};
    char* tmp = malloc(sizeof(char)*TOKEN_CAP);
    while(isdigit(c)){
        tmp[token.count] = c;
        assert(token.count < TOKEN_CAP);
        token.count++;
        clen++;
        c = text.content[clen];
        if(c == '.'){
            if(!isdigit(text.content[clen+1]))
                continue;
            tmp[token.count] = c;
            assert(token.count < TOKEN_CAP);
            token.count++;
            clen++;
            c = text.content[clen];
        }
    }
    token.token = tmp;
    return token;
}

bool hm_append(Tokenfreqs *map, Token token){
    // gorw map if needed 
    if(map->count >= map->capacity * 0.75){
        map->capacity = map->capacity == 0 ? DA_INIT_CAP : map->capacity*2;
        map->items = realloc(map->items, map->capacity*sizeof(*map->items));
        assert(map->items != NULL);
        memset(map->items, 0, map->capacity*sizeof(*map->items));
    }
    uint32_t h = djb2((uint8_t*)token.token, token.count)% map->capacity;;
    for(size_t i = 0; i< map->capacity && map->items[h].ocuppied && !token_eq(map->items[h].token, token); i++){
        h = (h+1)% map->capacity;
    }
    Tokenfreq *slot = &map->items[h];
    if(slot->ocuppied){
        if(!token_eq(slot->token, token)){
            fprintf(stderr, "table overflow\n");
            return false;
        }
        slot->value +=1;
    }else{
        slot->token = token;
        slot->value = 1;
        slot->ocuppied = true;
    }
    return true;
}

Tokenfreqs map = {0};

void lexer(FILE *file){
    char c= text.content[clen]; 
    Token token = {.count =0};
    int a;
    while((a = read_next_tag(file)) != EOF){
        while(text.idx > clen){
            c = text.content[clen]; 
            while(isspace(c)){
                clen++;
                c = text.content[clen];
            }

            if(isalpha(c)) {
                token = read_alpha();
            }
            else if(isdigit(c)){
                token = read_number();
            }
            else clen++;

            if(token.count > 0){
                hm_append(&map, token);
                token.count = 0;
            }

        }
        memset(text.content, 0, sizeof text.content); 
        text.idx = 0;
        clen = 0;
    }
}

int compare_tokenfreq(const void *a, const void *b){
    Tokenfreq *ta = (Tokenfreq*)a;
    Tokenfreq *tb = (Tokenfreq*)b;
    return (int)tb->value - (int)ta->value;
}

void file_dump(FILE *file){
    lexer(file);
    Tokenfreqs freq = {0};
    for(size_t i =0 ; i <map.capacity; i++){
        if(map.items[i].ocuppied){
            da_append(&freq, map.items[i]);
        }
    }
    qsort(freq.items, freq.count, sizeof(Tokenfreq),compare_tokenfreq);
    for(size_t i =0 ; i <freq.count; i++){
        printf("%s: %zu\n", freq.items[i].token.token, freq.items[i].value);
    }
}
char *join_path(const char *path, char *name){
    char *result = malloc(strlen(path) + strlen(name) + 2);
    strcpy(result, path);
    strcat(result, "/");
    strcat(result, name);
    return result;
}

Dires dires = {0};
// open dire and put all pathes in dires recursively
void read_entire_dir(const char *path){
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (path)) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            if(ent->d_type == DT_DIR){
                char *new_path = join_path(path, ent->d_name);
                read_entire_dir(new_path);
                free(new_path);
            }else{
                char *new_path = join_path(path, ent->d_name);
                Path path = {0} ;
                strcpy(path.path, new_path);
                da_append(&dires, path);
            }
        }
        closedir (dir);
    } else {
        /* could not open directory */
        perror ("");
        return;
    }
}

int main(void){
    FILE *file = fopen("./docs.gl/el3/textureProjGradOffset.xhtml", "r");  
    read_entire_dir("./docs.gl");
    for(size_t i = 0; i < dires.count; i++){
        printf("%s\n", dires.items[i].path);
    }
    fclose(file);
    return 0;
}
