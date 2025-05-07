TARGET = client_machine
CC = gcc
CFLAGS = -Wall -g

$(TARGET): cliente.o transmissor.o
	$(CC) $(CFLAGS) -o $(TARGET) cliente.o transmissor.o

cliente.o: cliente/cliente.c transmissor/transmissor.h
	$(CC) $(CFLAGS) -c cliente/cliente.c

transmissor.o: transmissor/transmissor.c transmissor/transmissor.h
	$(CC) $(CFLAGS) -c transmissor/transmissor.c

clean:
	rm -f $(TARGET) *.o