CC := g++

OBJS = obj/main.o obj/regex.o obj/regex_nfa.o obj/regex_parse.o obj/regex_dfa.o

CFLAGS = -Wall -g
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
