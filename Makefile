COMPILER = gcc
FLAGS = -Wall -Wextra -g
BINARYDIR = binary
EXENAME = salsa 
SRCDIR = ./src

SRCS = $(filter-out $(SRCDIR)/salsa.c, $(wildcard $(SRCDIR)/$*.c))
OBJS = $(patsubst $(SRCDIR)/%.c, $(BINARYDIR)/%.o, $(SRCS))

.PHONY: all clean run

all: $(BINARYDIR) $(EXENAME)

$(EXENAME): $(OBJS) $(BINARYDIR)/salsa.o
	$(COMPILER) $(FLAGS) $^ -o $@ -lm

$(BINARYDIR)/salsa.o: $(SRCDIR)/salsa.c
	$(COMPILER) $(FLAGS) -c $< -o $@ 

$(BINARYDIR)/%.o: $(SRCDIR)/%.c
	$(COMPILER) $(FLAGS) -c $< -o $@ 

$(BINARYDIR):
	mkdir -p $(BINARYDIR)

run:
	./$(EXENAME)

clean:
	rm -rf $(BINARYDIR) $(EXENAME)