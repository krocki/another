.SUFFIXES:
TARGETS=disasm gl_canvas gl_poly vm

CC=gcc
LD=gcc
CFLAGS=-g -Wfatal-errors -O0 -Wall
LFLAGS=-g -lm -lc
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
vm: vm.c polygon.c defs.h glutils.h polygon.h
	gcc vm.c polygon.c -o $@ $(CFLAGS) $(GL_FLAGS)
#
#%.o: %.c $(DEPS)
#	$(CC) $(CFLAGS) -c $< -o $@
#
#%: %.o
#	$(LD) -o $@ $^ $(LFLAGS)
#
#gl_%: gl_%.o
#	$(LD) $^ $(LFLAGS) -o $@
#
clean:
	rm -rf *.o $(TARGETS)
