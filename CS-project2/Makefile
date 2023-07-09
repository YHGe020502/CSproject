CC=cc
RPC_SYSTEM=rpc.o

.PHONY: format all

RPC_SYSTEM_A=rpc.a

all: $(RPC_SYSTEM) $(RPC_SYSTEM_A) rpc-server rpc-client

$(RPC_SYSTEM): rpc.c rpc.h
	$(CC) -c -o $@ $< -Wall

$(RPC_SYSTEM_A): rpc.o
	ar rcs $(RPC_SYSTEM_A) $(RPC_SYSTEM)

LDFLAGS=-L. rpc.a

rpc-server:
	$(CC) -o rpc-server server.c $(RPC_SYSTEM) -Wall $(LDFLAGS)

rpc-client:
	$(CC) -o rpc-client client.c $(RPC_SYSTEM) -Wall $(LDFLAGS)

format:
	clang-format -style=file -i *.c *.h

clean:
	rm -f *.o *.a rpc-server rpc-client
