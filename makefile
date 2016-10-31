.PHONY:all
.PHONY:clean

all: bsb_svr bsb_cli
bsb_svr: servidor.o
	gcc -o bsb_svr servidor.o

bsb_cli: client.o
	gcc -o bsb_cli client.o

servidor.o: servidor.c
	gcc -c servidor.c

client.o: client.c
	gcc -c client.c