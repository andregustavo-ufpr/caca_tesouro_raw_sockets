#include "transmissor.h"
#include <linux/limits.h>
#include <stdio.h>
#include <sys/stat.h>

void message_debug_print(message* m) {
  printf("checksum = %d\n", m->checksum);
  printf("sequence = %d\n", m->sequence);
  printf("size = %d\n", m->size);
  printf("type = %d\n", m->type);

  for (int i = 0; i < m->size; ++i) {
    printf("%X ", m->data[i]);
  }
  puts("\n");
}

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

void message_send_and_receive(int socket, message *send, message *receive) {
  int all_ok = 0;
  static int max_timeout = 2048;
  int timeout = 128;

  message_send(socket, *send);

  while (!all_ok) {
    while (message_receive(socket, receive, timeout) == -1) {
      if (timeout <= max_timeout) {
        timeout *= 2;
      }
      fprintf(stderr, "Resposta nao recebida, reenviando\n");
      message_send(socket, *send);
    }


    int sum = compute_checksum(receive);
    if (sum != receive->checksum) {
      if (send->type != TYPE_NACK)
        message_send(socket, *send);
      continue;
    }
    all_ok = 1;
  }
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


#define BUFFERN 1500 
static unsigned char buffer[BUFFERN];
static int data_in_buffer = 0; 
int message_receive(int socket, message *m, long long timeout) {
    long long start_time = timestamp();

    while (1) {
        // find a start marker
        if (data_in_buffer > 0) {
            int start_index = -1;
            for (int i = 0; i < data_in_buffer; i++) {
                if (buffer[i] == 0b01111110) {
                    start_index = i;
                    break;
                }
            }

            if (start_index != -1) {
                // discard garbage before the start marker
                if (start_index > 0) {
                    memmove(buffer, &buffer[start_index], data_in_buffer - start_index);
                    data_in_buffer -= start_index;
                }

                unsigned char clean_frame[BUFFERN];
                int clean_len = 0;
                int raw_len = 1; // start after the 0x7E marker
                int logical_size = -1;
                int total_logical_len = -1;

                while (raw_len < data_in_buffer) {
                    // check for stuffing 
                    if ((buffer[raw_len] == 0x88 || buffer[raw_len] == 0x81) && (raw_len + 1 < data_in_buffer) && buffer[raw_len + 1] == 0xFF) {
                        clean_frame[clean_len++] = buffer[raw_len];
                        raw_len += 2; 
                    } else {
                        clean_frame[clean_len++] = buffer[raw_len];
                        raw_len += 1;
                    }

                    // parse message length
                    if (logical_size == -1 && clean_len >= 3) {
                        logical_size = (clean_frame[0] >> 1) & 0x7F;
                        total_logical_len = 3 + logical_size; // 3 header bytes + data
                    }

                    if (total_logical_len != -1 && clean_len >= total_logical_len) {
                        break; // exit unstuffing loop
                    }
                }

                // check if successfully unstuffed a FULL frame
                if (total_logical_len != -1 && clean_len >= total_logical_len) {
                    m->size = logical_size;
                    m->sequence = ((clean_frame[0] & 0x01) << 4) | ((clean_frame[1] & 0xF0) >> 4);
                    m->type = clean_frame[1] & 0x0F;
                    m->checksum = clean_frame[2];
                    memcpy(m->data, &clean_frame[3], m->size);

                    // remove the consumed raw bytes from the buffer
                    memmove(buffer, &buffer[raw_len], data_in_buffer - raw_len);
                    data_in_buffer -= raw_len;

                    return 0;
                }
                // if we are here, it means we ran out of data in the buffer before
                // building a full frame
            } else {
                // no start marker found, the whole buffer is garbage
                data_in_buffer = 0;
            }
        }

        // if we need more data, check for timeout and receive
        if (timestamp() - start_time >= timeout) return -1;

        if (data_in_buffer >= BUFFERN) data_in_buffer = 0;

        int bytes_read = recv(socket, &buffer[data_in_buffer], BUFFERN - data_in_buffer, 0);
        if (bytes_read > 0) {
            data_in_buffer += bytes_read;
        } else if (bytes_read == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("recv");
            return -1;
        }
    }
}

int message_send(int socket, message m) {
    // original buffer beofre stuffing
    unsigned char original_buffer[3 + 128]; // 3 for header, 127 for data
    original_buffer[0] = (m.size << 1) | ((m.sequence >> 4) & 0x01);
    original_buffer[1] = ((m.sequence & 0x0F) << 4) | (m.type & 0x0F);
    original_buffer[2] = m.checksum;
    memcpy(&original_buffer[3], m.data, m.size);
    int original_len = 3 + m.size;

    // final buffer with start marker and stuffing
    unsigned char final_buffer[1 + (sizeof(original_buffer) * 2)];
    final_buffer[0] = 0b01111110;
    int final_len = 1;

    // byte stuffing
    for (int i = 0; i < original_len; i++) {
        unsigned char byte = original_buffer[i];
        if (byte == 0x88 || byte == 0x81) {
            final_buffer[final_len++] = byte;
            final_buffer[final_len++] = 0xFF; 
        } else {
            final_buffer[final_len++] = byte;
        }
    }

    if (send(socket, final_buffer, final_len >= 14 ? final_len : 14, 0) == -1) {
        fprintf(stderr, "send() failed: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

