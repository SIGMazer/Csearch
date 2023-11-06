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
#include <math.h>
#include <wchar.h>


#define TEXT_CAP 1024 *5
#define TOKEN_CAP 256
#define PUTBACK_LEN 32
#define ARRLEN(arr) (sizeof arr / sizeof arr[0])

typedef struct {
    char content[TEXT_CAP];
    size_t count;
} Text;

typedef struct{
    Text *items;
    size_t count;
    size_t capacity;
} Text_builder;

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
    Tokenfreq *items;
    size_t count;
    size_t capacity;
} Tokenfreqs;
typedef struct{
    char path[256];
} Path;

typedef struct{
    Path *items;
    size_t count;
    size_t capacity;
} Dires;

typedef struct{
    Path path;
    Tokenfreqs freq;
    bool ocuppied;
} Index;

typedef struct{
    Index *items;
    size_t count;
    size_t capacity;
} Indexs;
typedef struct{
    Path path;
    double score;
} Rank;
typedef struct{
    Rank *items;
    size_t count;
    size_t capacity;
} Ranks;

#define DA_INIT_CAP (256) 
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


#define da_free(da) \
    do {               \
        free((da)->items); \
        (da)->items = NULL; \
        (da)->count = (da)->capacity = 0; \
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





int parse_xhtml(FILE *file, Text *output){
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
                output->content[output->count]= '\0';
                return 0; // return 0 if end tag
            }
        }
        else if(!in_tag){
            if(!isspace(c) || !left_space){
                if(left_space) left_space = false;
                
                output->content[output->count]= c;
                assert(output->count < TEXT_CAP);
                output->count++;
            }
        }
    }
    return EOF; // return EOF if no more char
}

void read_entire_file(FILE *file, Text_builder *tb){
    int c = 0;
    Text tmp = {0};
    do{
        c = parse_xhtml(file, &tmp);
        if(c != EOF){
            da_append(tb, tmp);
            tmp.count = 0;
        }

    }while(c != EOF );
}

void freq_get(Tokenfreqs *map, Token token, size_t *value){
    uint32_t h = djb2((uint8_t*)token.token, token.count)% map->capacity;;
    for(size_t i = 0; i< map->capacity && map->items[h].ocuppied && !token_eq(map->items[h].token, token); i++){
        h = (h+1)% map->capacity;
    }
    Tokenfreq *slot = &map->items[h];
    if(slot->ocuppied){
        if(!token_eq(slot->token, token)){
            fprintf(stderr, "table overflow\n");
            return;
        }
        *value = slot->value;
    }else{
        *value = 0;
    }
}
bool freq_append(Tokenfreqs *map, Token token);
void rehash(Tokenfreqs *map, size_t new_capacity) {
    Tokenfreq *old_items = map->items;
    size_t old_capacity = map->capacity;

    //copy data from the old hash map to the new one
    Tokenfreqs tmp = {0};
    tmp.capacity = new_capacity;
    tmp.items = malloc(sizeof(*tmp.items) * tmp.capacity);
    assert(tmp.items != NULL);
    memset(tmp.items, 0, sizeof(*tmp.items) * tmp.capacity);
    for (size_t i = 0; i < old_capacity && map->count >0; ++i) {
        Tokenfreq *slot = &old_items[i];
        if (slot->ocuppied) {
            freq_append(&tmp, slot->token);
        }
    }
    
    free(old_items);
    *map = tmp;
}
#define INIT_HM(hm,cap) \
    do { \
        (hm)->capacity = (cap); \
        (hm)->items = malloc(sizeof(*(hm)->items) * (hm)->capacity); \
        assert((hm)->items != NULL); \
        memset((hm)->items, 0, sizeof(*(hm)->items) * (hm)->capacity); \
    } while (0)
bool freq_append(Tokenfreqs *map, Token token){
    // gorw map if needed
    if(map->capacity == 0){
        INIT_HM(map, DA_INIT_CAP);
    }
    if(map->count >= map->capacity * 0.75){
        rehash(map, map->capacity*2);
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
    map->count++;
    return true;
}



void lexer(Text_builder tb, Tokenfreqs *map){
    for(size_t i = 0; i < tb.count; i++){
        Text *text = &tb.items[i];
        char *p = text->content;
        char *end = p + text->count;
        while(p < end){
            char *start = p;
            while(p < end && !isspace(*p)){
                p++;
            }
            Token token = {0};
            token.token = malloc(p - start + 1);
            token.count = p - start;
            memcpy(token.token, start, token.count);
            token.token[token.count] = '\0';
            freq_append(map, token);
            while(p < end && isspace(*p)){
                p++;
            }
        }
    }
    da_free(&tb);
}


int compare_tokenfreq(const void *a, const void *b){
    Tokenfreq *ta = (Tokenfreq*)a;
    Tokenfreq *tb = (Tokenfreq*)b;
    return (int)tb->value - (int)ta->value;
}

char *join_path(const char *path, char *name){
    char *result = malloc(strlen(path) + strlen(name) + 2);
    strcpy(result, path);
    strcat(result, "/");
    strcat(result, name);
    return result;
}

// open dire and put all pathes in dires recursively
void read_entire_dir(const char *path, Dires *dires){
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (path)) != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            if(ent->d_type == DT_DIR){
                char *new_path = join_path(path, ent->d_name);
                read_entire_dir(new_path, dires);
                free(new_path);
            }else{
                char *new_path = join_path(path, ent->d_name);
                Path path = {0} ;
                strcpy(path.path, new_path);
                free(new_path);
                da_append(dires, path);
            }
        }
        closedir (dir);
    } else {
        /* could not open directory */
        perror ("");
        return;
    }
}

void index_get(Indexs* map, Path path, Tokenfreqs *value){
    uint32_t h = djb2((uint8_t*)path.path, strlen(path.path))% map->capacity;;
    for(size_t i = 0; i< map->capacity && map->items[h].freq.count != 0 && memcmp(map->items[h].path.path, path.path, strlen(path.path)) != 0; i++){
        h = (h+1)% map->capacity;
    }
    Index *slot = &map->items[h];
    if(slot->freq.count != 0){
        if(strcmp(slot->path.path, path.path) != 0){
            fprintf(stderr, "table overflow\n");
            return;
        }
        *value = slot->freq;
    }else{
        value->count = 0;
    }
}

void index_append(Indexs *map, Index index){
    // gorw map if needed 
    if(map->count >= map->capacity * 0.75){
        map->capacity = map->capacity == 0 ? DA_INIT_CAP : map->capacity*2;
        map->items = realloc(map->items, map->capacity*sizeof(*map->items));
        assert(map->items != NULL);
    }
    uint32_t h = djb2((uint8_t*)index.path.path, strlen(index.path.path))% map->capacity;;
    for(size_t i = 0; i< map->capacity && map->items[h].freq.count != 0 && memcmp(map->items[h].path.path, index.path.path, strlen(index.path.path)) != 0; i++){
        h = (h+1)% map->capacity;
    }
    Index *slot = &map->items[h];
    if(slot->freq.count != 0){
        if(strcmp(slot->path.path, index.path.path) != 0){
            fprintf(stderr, "table overflow\n");
            return;
        }
        slot->freq = index.freq;
    }else{
        slot->path = index.path;
        slot->freq = index.freq;
    }
    map->count++;
}

void indexing(Dires dires, Indexs *indexs){
    for(size_t i = 0; i < dires.count; i++){
        FILE *file = fopen(dires.items[i].path, "r");  
        printf("[INFO] indexing %s\n", dires.items[i].path);
        Text_builder tb = {0};
        Tokenfreqs map = {0};
        read_entire_file(file, &tb);
        lexer(tb, &map);
        Index index = {0};
        index.path = dires.items[i];
        index.freq = map;
        index.ocuppied = true;
        index_append(indexs, index);
        fclose(file);
    }
}
double tf(Tokenfreqs freqs, size_t freq){
    return (double)freq/(double)freqs.count;
}
double idf(char* term,Dires dires,  Indexs indexs){
    size_t termCount =0;
    for(size_t i = 0; i < dires.count; i++){
        Tokenfreqs freq = {0};
        index_get(&indexs, dires.items[i], &freq);
        if(freq.count == 0) continue;
        size_t value = 0;
        freq_get(&freq, (Token){.token = term,.count=strlen(term)}, &value);
        if(value > 0) termCount += 1;
    }
    return log((double)dires.count/(double)termCount);
}
int compare_score(const void *a, const void *b){
    Rank *ta = (Rank*)a;
    Rank *tb = (Rank*)b;
    if(ta->score > tb->score) return -1;
    else if(ta->score < tb->score) return 1;
    else return 0;
}

void searchandrankdoc(Tokenfreqs *query, Dires dires, Indexs indexs, Ranks *result){
    for(size_t i = 0; i < dires.count; i++){
        Tokenfreqs freq = {0};
        index_get(&indexs, dires.items[i], &freq);
        if(freq.count == 0) continue;
        double score = 0;
        for(size_t j = 0; j < query->count; j++){
            size_t value = 0;
            freq_get(&freq, query->items[j].token, &value);
            if(value > 0){
                double tf_ = tf(freq, value);
                double idf_ = idf(query->items[j].token.token,dires, indexs);
                double tfidf = tf_ * idf_;
                score += tfidf;
                Rank rank = {0};
                rank.path = dires.items[i];
                rank.score = score;
                da_append(result, rank);
            }
        }
    }
    qsort(result->items, result->count, sizeof(Rank), compare_score);
}

int main(int argc, char **argv){
    
    if(argc != 2){
        fprintf(stderr, "usage: %s <dir>\n", argv[0]);
        return 1;
    }
    char *dir_path = argv[1];

    Dires dires = {0};
    read_entire_dir(dir_path, &dires);
    Indexs indexs = {0};
    indexing(dires, &indexs);

    puts("indexing done");
    while(true){
        Tokenfreqs query = {0};
        Text_builder tb = {0};
        Text tmp = {0};
        char buf[256]={0};
        Ranks ranks = {0};

        puts("++++++++++++++++++++++++++++++++++++++++++++++++");

        printf("search: ");
        fgets(buf, ARRLEN(buf), stdin);
        strcpy(tmp.content, buf);
        tmp.count = strlen(tmp.content);
        da_append(&tb, tmp);
        lexer(tb, &query);


        Tokenfreqs query_freq = {0};
        for(size_t i = 0; i < query.capacity; i++){
            if(query.items[i].ocuppied){
                Tokenfreq freq = {0};
                freq.token = query.items[i].token;
                freq.value = 1;
                freq.ocuppied = true;
                da_append(&query_freq, freq);
            }
        }

        puts("++++++++++++++++++++++++++++++++++++++++++++++++");
        puts("search result:");
        searchandrankdoc(&query_freq, dires, indexs, &ranks);
        for(size_t i = 0; i < ranks.count; i++){
            if(ranks.items[i].score > 0)
                printf("\t%s\n", ranks.items[i].path.path );
            else break;
        }

    }
    // for(size_t i = 0; i < dires.count; i++){
    //     printf("%s\n", dires.items[i].path);
    // }
    // Text_builder tb = {0};
    //
    // Tokenfreqs map = {0};
    //
    // Tokenfreqs freq = {0};
    // // read_entire_file(file, &tb);
    // // lexer(tb, &map);
    // // size_t value = 0;
    // // hm_get(&map, (Token){.token = "the",.count=3}, &value);
    // // printf("%zu\n", value);
    // for(size_t j =0; j < map.capacity; j++){
    //     if(map.items[j].ocuppied)
    //         da_append(&freq, map.items[j]);
    // }
    // qsort(freq.items, freq.count, sizeof(Tokenfreq),compare_tokenfreq);
    // for(size_t i =0 ; i <freq.count; i++){
    //     printf("%s: %zu\n", freq.items[i].token.token, freq.items[i].value);
    // }

    return 0;
}
