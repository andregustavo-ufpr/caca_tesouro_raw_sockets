#ifndef TRANSMISSOR_H
#define TRANSMISSOR_H

#include <arpa/inet.h>
#include <errno.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>

#define TIMEOUT 300 // tempo pra timeout em milisegundos'

#define TYPE_ACK 0
#define TYPE_NACK 1
#define TYPE_OKACK 2
#define TYPE_SIZE 4
#define TYPE_DATA 5
#define TYPE_TEXTACKNAME 6
#define TYPE_VIDEOACKNAME 7
#define TYPE_IMAGEACKNAME 8
#define TYPE_ENDOFFILE 9
#define TYPE_MOVERIGHT 10
#define TYPE_MOVEUP 11
#define TYPE_MOVEDOWN 12
#define TYPE_MOVELEFT 13
#define TYPE_ERROR 15

// mensagem no formato do protocolo
typedef struct {
  unsigned char size;      // 7 bits
  unsigned char sequence;  // 5 bits
  unsigned char type;      // 4 bits
  unsigned char checksum;  // 8 bits (byte)
  unsigned char data[128]; // tamanho maximo (127) + 1 para caber \0
} message;

unsigned char compute_checksum(message *msg);

message create_message(unsigned char size, unsigned char sequence,
                       unsigned char type, unsigned char *data);

// returns array of buffers, each with 127 bytes of the file
// the last buffer may not be fully filled.
// stores the number of bytes read in _bytes_
// stores the number of buffers in _count_
unsigned char **split_file(char *filename, int *bytes, int *count);

int cria_raw_socket(char *nome_interface_rede);

// return 0 on succes -1 on timeout
int message_receive(int socket, message *m, long long timeout);

int message_send(int socket, message m);

#endif
