CLIENT_TARGET = client
SERVER_TARGET = server
CC = gcc
OBJ = $(SRC:.c=.o)
USER := $(shell whoami)
UID := $(shell id -u)
CFLAGS = -Wall -g -Iservidor -Itransmissor -Icliente -DUSER=\"$(USER)\" -DUID=$(UID)

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
	echo user $(USER) uid $(UID)

clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET) *.o transmissor/*.o cliente/*.o servidor/*.o
