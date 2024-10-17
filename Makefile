CC = gcc        # Compiler
CFLAGS = -Wall  # Compiler flags

# For Make all
all: TFTP_server.out

# For Server: required PORT
tftp: TFTP_server.out
	./TFTP_server.out $(IP) $(PORT) 

TFTP_server.out: TFTP_server.c
	$(CC) $(CFLAGS) -o TFTP_server.out TFTP_server.c

# for Make clean
clean:
	rm -f TFTP_server.out