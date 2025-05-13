#ifndef TRANSMISSOR_H
#define TRANSMISSOR_H

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>

#define TIMEOUT 300 // tempo pra timeout em milisegundos

// mensagem no formato do protocolo
typedef struct {
  unsigned char size; // 7 bits
  unsigned char sequence; // 5 bits
  unsigned char type; // 4 bits
  unsigned char checksum; // 8 bits (byte)
  unsigned char data[128]; // tamanho maximo (127) + 1 para caber \0
} message;

int cria_raw_socket(char* nome_interface_rede);

//return 0 on succes -1 on timeout
int message_receive(int socket, message* m, long long timeout);

int message_send(int socket, message m);

#endif
