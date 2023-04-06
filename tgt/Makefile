.PHONY: all clean

CFLAGS = -t sim65c02 -Oisr -Ln labs
SRC = $(wildcard *.c *.s)
CC = cl65

NAME = snestc1-sim

all: $(NAME)

$(NAME): $(SRC)
	$(CC) -o $(NAME) $(CFLAGS) $(SRC)

clean:
	rm -f $(NAME) *.o
