.PHONY: all clean

CFLAGS = -t sim65c02 -Oisr
SRC = $(wildcard *.c *.s)
CC = cl65

NAME = snestc1-sim

all: $(NAME)

$(NAME): $(SRC)
	$(CC) -o $(NAME) $(CFLAGS) $(SRC)

clean:
	rm -f $(NAME) *.o
