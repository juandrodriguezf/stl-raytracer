CC = gcc
CFLAGS ?= -std=c99 -Wall -Wextra -pedantic -O2
LDLIBS ?= -lm

ifeq ($(OS),Windows_NT)
RM = del /Q /F
endif

TARGET = raytracer.exe
OBJ = main.o stl.o bvh.o render.o vector.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDLIBS)

main.o: main.c stl.h bvh.h render.h vector.h
stl.o: stl.c stl.h vector.h
bvh.o: bvh.c bvh.h stl.h vector.h
render.o: render.c render.h bvh.h stl.h vector.h
vector.o: vector.c vector.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(TARGET) $(OBJ)
