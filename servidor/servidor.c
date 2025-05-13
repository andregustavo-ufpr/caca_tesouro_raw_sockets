#include "../transmissor/transmissor.h"
#include <sys/socket.h>

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Forneca nome para interface de rede\n");
    return 1;
  }

  int socket = cria_raw_socket(argv[1]);
  // configura timeout do socket
  const int timeoutMillis = TIMEOUT; 
  struct timeval timeout = { .tv_sec = timeoutMillis / 1000, .tv_usec = (timeoutMillis % 1000) * 1000 };
  setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout));
}
