.SUFFIXES:
TARGETS=disasm gl_canvas vm

CC=gcc
LD=gcc
CFLAGS=-g3 -Wfatal-errors -O0 -Wall
LFLAGS=-g3 -lm -lc
DEPS:=$(wildcard *.h) Makefile

# GL
# Mac OS
GL_FLAGS=-lglfw -framework Cocoa -framework OpenGL -lpthread
CFLAGS:=$(CFLAGS) -DAPPLE -DGL_SILENCE_DEPRECATION
# Linux
#GL_FLAGS=-lglfw -lGL -lpthread

LFLAGS:=$(LFLAGS) $(GL_FLAGS)

#########

all: $(TARGETS)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

%: %.o
	$(LD) -o $@ $^ $(LFLAGS)

gl_%: gl_%.o
	$(LD) $^ $(LFLAGS) -o $@

clean:
	rm -rf *.o $(TARGETS)
