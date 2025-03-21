all: p2

p2: main.o event.o manager.o resource.o system.o
	gcc -o p2 main.o event.o manager.o resource.o system.o

main.o: main.c defs.h
	gcc -c main.c

event.o: event.c defs.h
	gcc -c event.c

manager.o: manager.c defs.h
	gcc -c manager.c

resource.o: resource.c defs.h
	gcc -c resource.c

system.o: system.c defs.h
	gcc -c system.c

clean:
	rm -f p2 main.o event.o manager.o resource.o system.o