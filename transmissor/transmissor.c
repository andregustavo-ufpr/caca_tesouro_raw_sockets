#include "transmissor.h"
#include <sys/stat.h>

unsigned char compute_checksum(message *msg) {
  unsigned int sum = 0;

  // Include the fields in the checksum calculation
  sum += msg->size;
  sum += msg->sequence;
  sum += msg->type;

  // Only use the relevant part of data: size indicates the actual data length
  // (max 127)
  for (int i = 0; i < msg->size; i++) {
    sum += msg->data[i];
  }

  // Return the result modulo 256 (fit into one byte)
  return (unsigned char)(sum % 256);
}

message create_message(unsigned char size, unsigned char sequence,
                       unsigned char type, unsigned char *data) {
  message msg;

  msg.size = size;
  msg.sequence = sequence;
  msg.type = type;
  memcpy(msg.data, data, size);
  msg.checksum = compute_checksum(&msg);

  return msg;
}

int cria_raw_socket(char *nome_interface_rede) {
  // Cria arquivo para o socket sem qualquer protocolo
  int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (soquete == -1) {
    fprintf(stderr, "Erro ao criar socket: Verifique se você é root!\n");
    exit(-1);
  }

  int ifindex = if_nametoindex(nome_interface_rede);

  struct sockaddr_ll endereco = {0};
  endereco.sll_family = AF_PACKET;
  endereco.sll_protocol = htons(ETH_P_ALL);
  endereco.sll_ifindex = ifindex;
  // Inicializa socket
  if (bind(soquete, (struct sockaddr *)&endereco, sizeof(endereco)) == -1) {
    fprintf(stderr, "Erro ao fazer bind no socket\n");
    exit(-1);
  }

  struct packet_mreq mr = {0};
  mr.mr_ifindex = ifindex;
  mr.mr_type = PACKET_MR_PROMISC;
  // Não joga fora o que identifica como lixo: Modo promíscuo
  if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) ==
      -1) {
    fprintf(
        stderr,
        "Erro ao fazer setsockopt: "
        "Verifique se a interface de rede foi especificada corretamente.\n");
    exit(-1);
  }

  // configura timeout
  const int timeoutMillis = TIMEOUT; // 300 milisegundos de timeout por exemplo
  struct timeval timeout = {.tv_sec = timeoutMillis / 1000,
                            .tv_usec = (timeoutMillis % 1000) * 1000};
  setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
             sizeof(timeout));

  return soquete;
}

long long timestamp() {
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

// TODO:
// adicionar possibilidade de mensagens terem pedacos em mais de um buffer
// recebido atualmente a funcao assume que a mensagem esta contida inteira em um
// buffer
int message_receive(int socket, message *m, long long timeout) {
#define BUFFERN 500
  long long start_time = timestamp();
  unsigned char buffer[BUFFERN];

  do {
    int buffer_n = recv(socket, buffer, BUFFERN, 0);
    if (buffer_n == -1)
      continue;
    // printf("received buffer of size: %d\n", buffer_n);

    // search for start symbol
    int start_index = -1;
    for (int i = 0; i < buffer_n; ++i)
      if (buffer[i] == 0b01111110) {
        start_index = i;
        break;
      }
    if (start_index == -1)
      continue;
    // printf("start symbol found at %d\n", start_index);

    unsigned char m_size = (buffer[start_index + 1] >> 1) & 0x7f;
    unsigned char m_sequence = (buffer[start_index + 1] & 0xfe) |
                               ((buffer[start_index + 2] & 0xf0) >> 4);
    unsigned char m_type = buffer[start_index + 2] & 0x0f;

    unsigned char m_checksum = buffer[start_index + 3];

    for (int i = 0; i < m_size; ++i)
      m->data[i] = buffer[start_index + 4 + i];

    m->size = m_size;
    m->sequence = m_sequence;
    m->type = m_type;
    m->checksum = m_checksum;
    return 0;
  } while (timestamp() - start_time < timeout);

  return -1;
}

int message_send(int socket, message m) {
  unsigned char buffer[1 + 4 + 128];
  buffer[0] = 0b01111110;
  buffer[1] = (m.size << 1) | ((m.sequence >> 4) & 0x01);
  buffer[2] = ((m.sequence & 0x0F) << 4) | (m.type & 0x0F);
  buffer[3] = m.checksum;
  for (int i = 0; i < m.size; ++i)
    buffer[4 + i] = m.data[i];

  int padding_size = (10 - m.size) > 0 ? 10 - m.size : 0;
  if (send(socket, buffer, 4 + m.size + padding_size, 0) == -1) {
    fprintf(stderr, "send() failed: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

unsigned char **split_file(char *filename, int *bytes, int *count) {
  struct stat info;
  if (stat(filename, &info) == -1) {
    fprintf(stderr, "failed to read file size\n");
    exit(1);
  }
  *bytes = info.st_size;
  int lcount = (*bytes / 127) + 1;

  FILE *f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "Couldnt open file %s\n", filename);
    exit(1);
  }

  unsigned char **array = malloc(sizeof(unsigned char *) * lcount);
  if (!array) {
    fprintf(stderr, "failed to allocate memory\n");
    exit(1);
  }

  for (int i = 0; i < lcount; ++i) {
    array[i] = malloc(127);
    if (!array[i]) {
      fprintf(stderr, "failed to allocate memory\n");
      exit(1);
    }
  }

  for (int i = 0; i < lcount; ++i) {
    fread(array[i], 1, 127, f);
  }

  *count = lcount;
  return array;
}
