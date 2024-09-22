CC := g++

OBJS = obj/main.o obj/regex.o

CFLAGS = -g
LDFLAGS = 

mygrep: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@


.PHONY: all
all: mygrep

.PHONY: clean
clean:
	- rm $(OBJS)
	- rm mygrep


obj/%.o: src/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@
