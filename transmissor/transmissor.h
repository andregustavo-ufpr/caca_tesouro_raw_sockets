#ifndef TRANSMISSOR_H
#define TRANSMISSOR_H

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>

int cria_raw_socket(char* nome_interface_rede);

#endif