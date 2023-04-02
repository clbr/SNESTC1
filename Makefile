.PHONY: all clean copy

FILES = $(wildcard *.c)
OBJS = $(FILES:.c=.o)

CFLAGS += -Wall -Wextra -ffunction-sections -fdata-sections
LDFLAGS += -Wl,-O1 -Wl,-gc-sections
LIBS = -lpng

NAME = snestc1

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) -o $(NAME) $(LDFLAGS) $(CFLAGS) $(LIBS) $(OBJS)

$(OBJS): $(wildcard *.h)

clean:
	rm -f $(NAME) *.o
