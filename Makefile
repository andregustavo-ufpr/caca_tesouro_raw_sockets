TARGET = client_machine
CC = gcc
CFLAGS = -Wall -g -Iservidor -Itransmissor -Icliente
SRC = cliente/cliente.c servidor/servidor.c transmissor/transmissor.c
OBJ = $(SRC:.c=.o)

# Rules
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) *.o transmissor/*.o cliente/*.o servidor/*.o