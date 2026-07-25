COMPILER = gcc
FLAGS = -Wall -Wextra -g
BINARYDIR = binary
EXENAME = salsa 

SRCS = $(filter-out salsa.c, $(wildcard *.c))
OBJS = $(SRCS:%.c=$(BINARYDIR)/%.o)

.PHONY: all clean

all: $(BINARYDIR) $(EXENAME)

$(EXENAME): $(OBJS) $(BINARYDIR)/salsa.o
		$(COMPILER) $(FLAGS) $^ -o $@ 

$(BINARYDIR)/salsa.o: salsa.c
		$(COMPILER) $(FLAGS) -c $< -o $@ 

$(BINARYDIR):
	test ! -d $(BINARYDIR) && mkdir $(BINARYDIR)

run:
	./$(EXENAME)

clean:
	rm -rf $(BINARYDIR)/* $(EXENAME)