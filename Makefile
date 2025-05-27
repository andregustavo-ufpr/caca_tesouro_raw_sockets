CLIENT_TARGET = client
SERVER_TARGET = server
CC = gcc
CFLAGS = -Wall -g -Iservidor -Itransmissor -Icliente
OBJ = $(SRC:.c=.o)

CLIENT_SRC = cliente/cliente.c transmissor/transmissor.c
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

SERVER_SRC = servidor/servidor.c transmissor/transmissor.c
SERVER_OBJ = $(SERVER_SRC:.c=.o)

# Rules
all: $(CLIENT_TARGET) $(SERVER_TARGET)

$(CLIENT_TARGET): $(CLIENT_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

$(SERVER_TARGET): $(SERVER_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET) *.o transmissor/*.o cliente/*.o servidor/*.o