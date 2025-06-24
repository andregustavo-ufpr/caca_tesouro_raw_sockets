#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../transmissor/transmissor.h"
#include <unistd.h>
#include<sys/wait.h>

// #define USER "user"
// #define UID 1000

int currentXPos = 0;
int currentYPos = 0;

#define MAX_TIMEOUT 2048
long long timeout = 1;

void display_image(char* filename) {
    char command[256];
    sprintf(command, "sudo -u %s DISPLAY=:0 XAUTHORITY=/home/%s/.Xauthority XDG_RUNTIME_DIR=/run/user/%d firefox %s", USER, USER, UID, filename);
    system(command);
}
void display_video(char* filename) {
    display_image(filename);
}
void display_text(char* filename) {
    if (fork() == 0) {
        char* args[] = {"cat", filename, NULL};
        execve("/bin/cat", args, NULL);
    } else {
        wait(NULL);
    }
}

void receive_file(int socket, message* m, void (*display) (char*)) {
    message ack = create_message(0, 0, TYPE_ACK, NULL);
    message nack = create_message(0, 0, TYPE_NACK, NULL);
    message response;
    int bytes_received = 0;
    int last_sequence_received;
    int expected_sequence;
    char filename[64];

    memcpy(filename, m->data, 5);
 
    // receives size message
    do {
        message_send_and_receive(socket, &ack, &response);
    } while (response.type != TYPE_SIZE);
    int file_size = *(int*) response.data;
    unsigned char* data = malloc(file_size);
    message_send_and_receive(socket, &ack, &response);

    // receives data
    last_sequence_received = -1;
    while (response.type == TYPE_DATA) {
        expected_sequence = (last_sequence_received + 1) % 32;
        if (expected_sequence == response.sequence) {
            last_sequence_received = response.sequence;
            memcpy(&data[bytes_received], response.data, response.size);
            bytes_received += response.size;
            ack = create_message(0, response.sequence, TYPE_ACK, NULL);
        } else {
            ack = create_message(0, last_sequence_received, TYPE_ACK, NULL);
        }
        message_send_and_receive(socket, &ack, &response);
    }

    if (response.type == TYPE_ENDOFFILE) {
        message_send(socket, ack);
    }

    FILE* f = fopen(filename, "wb");
    fwrite(data, file_size, 1, f);
    fclose(f);

    display(filename);
}





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
            message_send_and_receive(socket, &m, &received);
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
                case TYPE_VIDEOACKNAME:
                    printf("Receiving video\n");
                    receive_file(socket, &received, display_video);
                    currentXPos = newXPos;
                    currentYPos = newYPos;
                    message_all_ok = 1;
                    break;
                case TYPE_IMAGEACKNAME:
                    printf("Receiving image\n");
                    receive_file(socket, &received, display_image);
                    currentXPos = newXPos;
                    currentYPos = newYPos;
                    message_all_ok = 1;
                    break;
                case TYPE_TEXTACKNAME:
                    printf("Receiving text\n");
                    receive_file(socket, &received, display_text);
                    currentXPos = newXPos;
                    currentYPos = newYPos;
                    message_all_ok = 1;
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
