#include "../transmissor/transmissor.h"
#include "../coordinates/coordinates.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dirent.h>

#define MAX_TREASURES 8
#define CHUNK_SIZE 127
#define USINGLOOPBACK 

Coord treasures[MAX_TREASURES];

int storedClientXPos = 0;
int storedClientYPos = 0;

int r_socket;

#define MAX_TIMEOUT 2048
long long timeout = 1;

void send_file_in_chunks(unsigned char **file_chunks, int size, int batches, const char *file_path) {
    message response;
    size_t bytes_sent = 0;

    int type;
    if (strstr(file_path, "txt"))
        type = TYPE_TEXTACKNAME;
    else if (strstr(file_path, "mp4"))
        type = TYPE_VIDEOACKNAME;
    else
        type = TYPE_IMAGEACKNAME;

    message start_message = create_message(5, 0, type, (unsigned char *) file_path);
    do {
        message_send_and_receive(r_socket, &start_message, &response);
    } while (response.type != TYPE_ACK);


    message size_message = create_message(sizeof(size), 0, TYPE_SIZE,  (unsigned char*)&size);
    do {
        message_send_and_receive(r_socket, &size_message, &response);
    } while (response.type != TYPE_ACK);
    
    size_t i = 0;
    while (i < batches){
        size_t bytes_to_send = size - bytes_sent;
        message file_piece = create_message(bytes_to_send >= CHUNK_SIZE ? 127 : bytes_to_send, i%32, TYPE_DATA, file_chunks[i]);
        do {
            do {
                message_send_and_receive(r_socket, &file_piece, &response);
            } while(response.type != TYPE_ACK);

            if (response.sequence == i%32) {
                break;
            }
            else {
                continue;
            }

        } while (1);
        bytes_sent += file_piece.size;
        i += 1;
    }

    message end_message = create_message(0, 0, TYPE_ENDOFFILE, NULL);
    do {
        message_send_and_receive(r_socket, &end_message, &response);
    } while (response.type != TYPE_ACK);

    printf("envio de arquivo completo\n");
    return;
}

void find_object(int n, char* out_path){
    DIR* directory = opendir("objects");
    if (!directory) {
        fprintf(stderr, "Falha ao abrir objects\n");
        exit(1);
    }

    struct dirent* entry;
    char prefix[8];
    snprintf(prefix, sizeof(prefix), "%d.", n);

    while ((entry = readdir(directory)) != NULL) {
        if (strncmp(entry->d_name, prefix, strlen(prefix)) == 0) {
            strcpy(out_path, entry->d_name);
            
            char full_path[64] = "objects/";
            sprintf(full_path + strlen(full_path), "%s", out_path);
            int file_size = 0;
            int batches_qtt = 0;
            unsigned char **separated_file;
            
            separated_file = split_file(full_path, &file_size, &batches_qtt);
            send_file_in_chunks(separated_file, file_size, batches_qtt, out_path);

            closedir(directory);
            return;
        }
    }
}

int move_character(int* positionX, int* positionY, int type){
    int newX = *positionX;
    int newY = *positionY;
    switch (type) {
        case TYPE_MOVEDOWN:
            newY++;
            break;
        case TYPE_MOVEUP:
            newY--;
            break;
        case TYPE_MOVERIGHT:
            newX++;
            break;
        case TYPE_MOVELEFT:
            newX--;
            break;
    }

    if (newX < 0 || newX > 7
        || newY < 0 || newY > 7) {
        fprintf(stderr, "Movimento invalido\n");
        return -1;
    } else {
        *positionX = newX;
        *positionY = newY;
    }

    printf("Movimento para %d %d\n", *positionX, *positionY);
    return 0;
}

int message_handler(message* m){
    switch (m->type){
        case TYPE_MOVERIGHT:
        case TYPE_MOVELEFT:
        case TYPE_MOVEUP:
        case TYPE_MOVEDOWN:
            if(move_character(&storedClientXPos, &storedClientYPos, m->type) == -1){
                // In case of an error inside move_character function
                return -2;
            }

            for (int i = 0; i < MAX_TREASURES; ++i) {
                if (treasures[i].x == storedClientXPos && treasures[i].y == storedClientYPos) {
                    return i + 1;
                }
            }
            
            return 0; 
        default:
            fprintf(stderr, "Mensagem de tipo inesperado %d\n", m->type);
            return -1;
    }

    return -1;
}

void choose_random_coordinates(Coord coords[MAX_TREASURES]) {
    int used[9][9] = {0};
    srand(time(NULL));
    int count = 0;
    while (count < MAX_TREASURES) {
        int x = rand() % 8;
        int y = rand() % 8;
        if (!used[x][y]) {
            coords[count].x = x;
            coords[count].y = y;
            used[x][y] = 1;
            count++;
        }
    }
}

int main(int argc, char** argv){
    if (argc < 2) {
        fprintf(stderr, "Especifique uma interface\n");
        exit(1);
    }
    r_socket = cria_raw_socket(argv[1]); // Make connection with client
    message recieved_message;
    // memset(&recieved_message, 0, sizeof(message));


    choose_random_coordinates(treasures);
    printf("Locais de tesouro:\n");
    for (int i = 0; i < MAX_TREASURES; ++i) {
        printf("%d %d    ", treasures[i].x, treasures[i].y);
    }
    printf("\n");

    while(1){
        int recieved = message_receive(r_socket, &recieved_message, TIMEOUT);


        if(recieved != -1){
            if (compute_checksum(&recieved_message) != recieved_message.checksum) {
                continue;
            }

            printf("received type %d\n", recieved_message.type);
            int computed = message_handler(&recieved_message);
            
            if (computed > 0) {
                char object_name[64];
                find_object(computed, object_name);
                printf("Tesouro %s enviado\n", object_name);
            }
            else if (computed == 0) {
                message ok_msg = create_message(0, 0, TYPE_OKACK, NULL);

                message_send(r_socket, ok_msg);
            } else if (computed == -2) {
                message err_msg = create_message(0, 0, TYPE_ERROR, NULL);

                message_send(r_socket, err_msg);
                exit(1);
            }
        }
    }
}
