#include <stdio.h>
#include "../transmissor/transmissor.h"

// #define USINGLOOPBACK

int currentXPos = 0;
int currentYPos = 0;

#define MAX_TIMEOUT 2048
long long timeout = 1;

void display_video(char* filename) {}
void display_text(char* filename) {}
void display_image(char* filename) {}
void receive_file(message* m, void (*display) (char*)) {}

void printGrid() {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            printf("+---");
        }
        printf("+\n");
        for (int j = 0; j < 8; j++) {
            if(i == currentYPos && j == currentXPos){
                printf("| & ");
            } 
            else {
                printf("|   ");
            }
        }
        printf("|\n");
    }
    for (int j = 0; j < 8; j++) {
        printf("+---");
    }
    printf("+\n");
}

int main(int argc, char** argv){
    if (argc < 2) {
        fprintf(stderr, "Especifique uma interface\n");
        exit(1);
    }
    int socket = cria_raw_socket(argv[1]);
    
    while(1){
        fflush(stdout);
        printGrid();

        int newXPos = currentXPos;
        int newYPos = currentYPos;

        message received;

        #ifdef USINGLOOPBACK
        message dummy;
        #endif
    
        char movedir;
        printf("Para onde gostaria de se mover? use w a s d\n");
        scanf(" %c", &movedir);

        int movement_type;
        switch (movedir) {
            case 'a':
                movement_type = TYPE_MOVELEFT;
                newXPos--;
                break;
            case 's':
                movement_type = TYPE_MOVEDOWN;
                newYPos++;
                break;
            case 'w':
                movement_type = TYPE_MOVEUP;
                newYPos--;
                break;
            case 'd':
                movement_type = TYPE_MOVERIGHT;
                newXPos++;
                break;
            default:
                fprintf(stderr, "Tecla invalida\n");
                exit(1);
            
        }
        message m = create_message(0, 0, movement_type, NULL);
        int message_all_ok = 0;

        // Send message to server
        while (!message_all_ok) {
            message_send(socket, m);
            #ifdef USINGLOOPBACK
            message_receive(socket, &dummy, 300);
            #endif
            // Await for server response and check for OK type
            // TODO: add proper exponential timeout
            #ifdef USINGLOOPBACK
            message_receive(socket, &dummy, 300);
            #endif
            while (message_receive(socket, &received, timeout) == -1) {
                printf("timeout: %lld\n", timeout);
                timeout *= 2;
                if (timeout > MAX_TIMEOUT) {
                    fprintf(stderr, "Resposta nao recebida. Reenviando\n");
                    timeout = 1;
                    continue;
                }
            }
            timeout = 1;

            if (compute_checksum(&received) != received.checksum) {
                printf("checksum fail\n");
                continue;
            }
            switch (received.type) {
                case TYPE_OKACK:
                case TYPE_ACK:
                    currentXPos = newXPos;
                    currentYPos = newYPos;
                    message_all_ok = 1;
                    break;
                case TYPE_NACK:
                    continue; // send again
                    break;
                case TYPE_ERROR:
                    fprintf(stderr, "Movimento invalido\n");
                    exit(1);
                    break;
                // TODO: get files
                case TYPE_VIDEOACKNAME:
                    printf("Receiving video\n");
                    receive_file(&received, display_video);
                    break;
                case TYPE_IMAGEACKNAME:
                    printf("Receiving image\n");
                    receive_file(&received, display_image);
                    break;
                case TYPE_TEXTACKNAME:
                    printf("Receiving text\n");
                    receive_file(&received, display_text);
                    break;
                default:
                    fprintf(stderr, "Mensagem de tipo inesperado: %d\n",
                            received.type);
                    continue;
            }
        }
    }
    return 0;
}
