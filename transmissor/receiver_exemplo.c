#include "transmissor.h"
#include <stdio.h>

int main() {
  unsigned char b[500];
  int socket = cria_raw_socket("lo");
  message m;
  do {
  if (message_receive(socket, &m, 1000) == -1) {
    printf("timeout\n");
    continue;
  }

  printf("received:\n%d\n%d\n%d\n%d\n%d\n", m.size, m.sequence, m.type, m.checksum, m.data[0]);
  
  }while (1);
}
