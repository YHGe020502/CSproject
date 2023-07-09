EXE=allocate

SRC=main.c queue.c scheduler.c allocation.c
OBJ=$(SRC:.c=.o)

$(EXE): $(OBJ)
	cc -Wall -O2 -o $(EXE) $(OBJ)

%.o: %.c
	cc -Wall -O2 -c $<

format:
	clang-format -style=file -i *.c

clean:
	rm -f $(EXE) $(OBJ)  process