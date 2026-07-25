#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <math.h>

#define ROTATE32(x, n) (((x) << (n)) | ((x) >> (32-n)))
#define uint8 uint8_t
#define uint32 uint32_t
#define uint64 uint64_t

const uint32 NUM_ROUNDS = 20;
uint32 CONSTANTS[4] = {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574};

void quarterRound(uint32 *a, uint32 *b, uint32 *c, uint32 *d){
    *b ^= ROTATE32((*a + *d), 7);
    *c ^= ROTATE32((*b + *a), 9);
    *d ^= ROTATE32((*c + *b), 13);
    *a ^= ROTATE32((*d + *c), 18);
}

uint32* buildInternState(uint32* constants, uint32* key, uint32* nonce, uint32* counter){
    uint32* intern_state = (uint32*) malloc (sizeof(uint32) * 16);
    if(intern_state == NULL){
        printf("ERRO: Erro ao alocar memoria para a variavel 'intern_state'\n");
        exit(1);
    }

    intern_state[0] = constants[0];
    memcpy(&intern_state[1], key, sizeof(uint32) * 4);
    intern_state[5] = constants[1];
    memcpy(&intern_state[6], nonce, sizeof(uint32) * 2);
    memcpy(&intern_state[8], counter, sizeof(uint32) * 2);
    intern_state[10] = constants[2];
    memcpy(&intern_state[11], &key[4], sizeof(uint32) * 4);
    intern_state[15] = constants[3];

    return intern_state;
}

// Retorna a keystream
uint8* salsa(uint32* constants, uint32* key, uint64 nonce, uint64 counter, uint32 num_rounds){
    uint32 *intern_state = buildInternState(constants, key, (uint32*) &nonce, (uint32*) &counter);
    uint32 *intern_state_copy = (uint32*) malloc (sizeof(uint32) * 16);
    if(intern_state_copy == NULL){
        printf("ERRO: Erro ao alocar memoria para a variavel 'intern_state_copy'\n");
        exit(1);
    }
    memcpy(intern_state_copy, intern_state, sizeof(uint32) * 16);


    for(uint32 round = 1; round <= num_rounds; ++round){
        if(round & 1){
            for(uint32 q = 0; q < 4; ++q){
                quarterRound(&intern_state[q * 5], 
                             &intern_state[(q * 5 + 4) % 16], 
                             &intern_state[(q * 5 + 8) % 16],
                             &intern_state[(q * 5 + 12) % 16]);
            }
        } else {
            for(uint32 q = 0; q < 4; ++q){
                quarterRound(&intern_state[q * 5],
                             &intern_state[(q * 5 + 1) % 16],
                             &intern_state[(q * 5 + 2) % 16],
                             &intern_state[(q * 5 + 3) % 16]);
            }
        }
    }

    uint32 *keystream = (uint32*) calloc (16, sizeof(uint32));
    if(keystream == NULL){
        printf("ERRO: Erro ao alocar memoria para a variavel 'keystream'\n");
        free(intern_state);
        free(intern_state_copy);
        exit(1);
    }

    for(uint32 i = 0; i < 16; ++i){
        keystream[i] = intern_state[i] + intern_state_copy[i];
    }

    free(intern_state);
    free(intern_state_copy);

    return (uint8*) keystream;
}

uint64 generateNonce(void){
    uint64 nonce = 0;
    for(uint32 i = 0; i < 8; ++i){
        nonce |= (rand() % 257) << (i * 8); // mod 1 byte + 1
    }
    return nonce;
}

unsigned char* generateKey(void){
    unsigned char *key = (unsigned char*) malloc (sizeof(unsigned char) * 32); // 8 palavras * 4 bytes/palavra
    if(key == NULL){
        printf("ERRO: Erro ao alocar memoria para a variavel 'key'\n");
        exit(1);
    }

    for(uint32 i = 0; i < 32; ++i){
        key[i] = (unsigned char) (rand() % 257); // mod 1 byte + 1
    }

    return key;
}

unsigned char* encode(char *plain_text, uint32* key){
    uint32 plain_text_size = strlen(plain_text);
    uint32 blocks_needed = ceil(plain_text_size / 64.0f);

    unsigned char *ciphered_text = (unsigned char*) malloc (sizeof(unsigned char) * (plain_text_size + 1));
    if(ciphered_text == NULL){
        printf("ERRO: Erro ao alocar memoria para a variavel 'ciphered_text'\n");
        exit(1);
    } 

    unsigned char *nonces = (unsigned char*) malloc (sizeof(unsigned char) * blocks_needed * sizeof(uint64)); 
    if(nonces == NULL){
        printf("ERRO: Erro ao alocar memoria para a variavel 'nonces'\n");
        free(ciphered_text);
        exit(1);
    }

    for(uint64 counter = 0; counter < blocks_needed; ++counter){
        uint64 nonce = generateNonce(); 
        memcpy(&nonces[counter * sizeof(uint64)], &nonce, sizeof(uint64));

        uint8 *keystream = salsa(CONSTANTS, key, nonce, counter, NUM_ROUNDS);

        for(uint32 i = 0, j = counter * 64; i < 64 && j < plain_text_size; ++i, ++j){
            ciphered_text[j] = (uint8) keystream[i] ^ (uint8) plain_text[j];
        }

        free(keystream);
    }

    FILE *nonces_file = fopen("data/nonces.txt", "wb");
    if(nonces_file == NULL){
        printf("ERRO: Erro ao abrir o arquivo 'nonces.txt'\n");
        free(ciphered_text);
        free(nonces);
        exit(1);
    }
    fwrite(nonces, sizeof(unsigned char), sizeof(unsigned char) * blocks_needed * sizeof(uint64), nonces_file);
    free(nonces);
    fclose(nonces_file);

    ciphered_text[plain_text_size] = '\0';
    return ciphered_text;
}

unsigned char* decode(char* ciphered_text, uint32* key, uint64* nonces, uint32 ciphered_text_size){
    uint32 blocks_needed = ceil(ciphered_text_size / 64.0f);

    unsigned char *decoded_text = (unsigned char*) malloc (sizeof(unsigned char) * (ciphered_text_size + 1));
    if(decoded_text == NULL){
        printf("ERRO: Erro ao alocar memoria para a variavel 'decoded_text'\n");
        exit(1);
    }
    for(uint64 counter = 0; counter < blocks_needed; ++counter){
        uint64 nonce = nonces[counter];

        uint8 *keystream = salsa(CONSTANTS, key, nonce, counter, NUM_ROUNDS);

        for(uint32 i = 0, j = counter * 64; i < 64 && j < ciphered_text_size; ++i, ++j){
            decoded_text[j] = (uint8) keystream[i] ^ (uint8) ciphered_text[j];
        }

        free(keystream);
    }

    decoded_text[ciphered_text_size] = '\0';
    return decoded_text;
}

int main() {
    srand(time(NULL));
         
    FILE *input_file = fopen("data/plain.txt", "rb");
    if(input_file == NULL){
        printf("ERRO: Erro ao abrir o arquivo de entrada.\n");
        return 1;
    }
    // Move o ponteiro para o final do arquivo para descobrir o tamanho
    // e logo volta para o inicio do arquivo
    fseek(input_file, 0, SEEK_END);
    uint64 input_file_size = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);

    char *plain_text = (char*) malloc (sizeof(char) * (input_file_size + 1));
    if(plain_text == NULL){
        printf("ERRO: Falha ao alocar memoria para a variavel 'plain_text'\n");
        fclose(input_file);
        return 1;
    }
    uint64 read_size = fread(plain_text, sizeof(char), input_file_size, input_file);
    if(input_file_size != read_size){
        printf("ERRO: Falha ao ler o arquivo de entrada 'input_file'.\n");
        fclose(input_file);
        free(plain_text);
        return 1;
    }
    plain_text[input_file_size] = '\0';
    fclose(input_file);

    unsigned char *key = generateKey();
    FILE *key_file = fopen("data/key.txt", "wb");
    if(key_file == NULL){
        printf("ERRO: Erro ao abrir o arquivo de chave.\n");
        return 1;
    }
    fwrite(key, sizeof(unsigned char), sizeof(unsigned char) * 32, key_file);
    fclose(key_file); 

    unsigned char *ciphered_text = encode(plain_text, (uint32*) key);
    free(key); 

    FILE *ciphered_file = fopen("data/encoded.txt", "wb");
    if(ciphered_file == NULL){
        printf("ERRO: Erro ao abrir o arquivo de saida.\n");
        return 1;
    }
    fwrite(ciphered_text, sizeof(unsigned char), input_file_size, ciphered_file);
    fclose(ciphered_file);

    // Decodificacao
    FILE *nonces_file = fopen("data/nonces.txt", "rb");
    if(nonces_file == NULL){
        printf("ERRO: Erro ao abrir o arquivo 'nonces_file'\n");
        free(ciphered_text);
        return 1;
    }
    // Move o ponteiro para o final do arquivo para descobrir o tamanho
    // e logo volta para o inicio do arquivo
    fseek(nonces_file, 0, SEEK_END);
    uint64 nonces_file_size = ftell(nonces_file);
    fseek(nonces_file, 0, SEEK_SET);
    
    uint8 *nonces = (uint8*) malloc (sizeof(uint8) * nonces_file_size);
    if(nonces == NULL){
        printf("ERRO: Erro ao alocar memoria para a variavel 'nonces'\n");
        free(ciphered_text);
        fclose(nonces_file);
        return 1;
    }
    read_size = fread(nonces, sizeof(uint8), nonces_file_size, nonces_file);
    if(read_size != nonces_file_size){
        printf("ERRO: Falha ao ler o arquivo 'nonces.txt'\n");
        free(ciphered_text);
        fclose(nonces_file);
        return 1;
    }
    fclose(nonces_file);

    key_file = fopen("data/key.txt", "r");
    if(key_file == NULL){
        printf("ERRO: Erro ao abrir o arquivo 'key.txt'\n");
        free(ciphered_text);
        free(nonces);
        return 1;
    }
    key = (unsigned char*) malloc (sizeof(unsigned char) * 32);
    if(key == NULL){
        printf("ERRO: Erro ao alocar memoria para a variavel 'key'\n");
        free(ciphered_text);
        free(nonces);
        fclose(key_file);
        return 1;
    }
    read_size = fread(key, sizeof(unsigned char), sizeof(unsigned char) * 32, key_file);
    if(read_size != sizeof(unsigned char) * 32){
        printf("ERRO: Falha ao ler o arquivo 'key.txt'\n");
        free(ciphered_text);
        free(nonces);
        fclose(key_file);
        return 1;
    }
    fclose(key_file);

    char* decoded_text = (char*) decode((char*) ciphered_text, (uint32*) key, (uint64*) nonces, input_file_size);
    free(ciphered_text);
    free(nonces);
    free(key);

    if(!strcmp((char*) decoded_text, (char*) plain_text)){
        printf("A codificacao e a decodificacao utilizando a cifra Salsa ocorreram com sucesso!\n");
        printf("\nMensagem decodificada:\n");
        printf("%s\n", decoded_text);
    } else {
        printf("ERRO: Ocorreu um erro na codificacao/decodificacao. O texto decodificado nao e o mesmo do texto original.\n");
        printf("Diff: %d\n", (uint32) strcmp((char*) decoded_text, (char*) plain_text));
        return 1;
    }

    free(decoded_text);
    free(plain_text);
    return 0;
}
