#include "transmissor.h"
#include <stdio.h>
#include <unistd.h>

int main() {
  int socket = cria_raw_socket("lo");
  do {
  message m;
  m.size = 1;
  m.sequence = 14;
  m.type = 8;
  m.checksum = 12;
  m.data[0] = 1;
  message_send(socket, m);
  printf("sending...\n");
  if (message_send(socket, m) == -1) {
    printf("failed to send\n");
    sleep(2);
    continue;
  }
  sleep(5);
  } while (1);
}
