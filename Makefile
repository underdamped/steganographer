################################
## Makefile for steganographer
##
## Javier Lombillo <javier@asymptotic.org>
## October 2015
##

CC	   = gcc
CFLAGS = -W -Wall
LFLAGS = -lm

SRCS 	= bitmap.c file_io.c helpers.c main.c memory.c pcm.c stego.c
OBJECTS = $(SRCS:.c=.o)
EXE 	= steganographer

all: $(EXE)

$(EXE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EXE) $(LFLAGS)
	@strip $(EXE)
	@echo "Build complete."

bitmap.c:  steganographer.h
file_io.o: steganographer.h
helpers.o: steganographer.h
main.o:	   steganographer.h
memory.o:  steganographer.h
pcm.o:     steganographer.h
stego.o:   steganographer.h

.PHONY: clean mrproper rebuild

# clean up object files
clean:
	rm -f $(OBJECTS)

# sparkly clean
mrproper: clean
	rm -f steganographer

# erase everyhing and start over
rebuild: mrproper all
