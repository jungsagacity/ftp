main:main.o upload.o ftp.o log.o  msem.o
	gcc main.o upload.o ftp.o log.o  msem.o -o  main

ftp.o: ftp.c
	gcc -c ftp.c -o ftp.o

log.o:log.c
	gcc -c log.c -o log.o


upload.o: upload.c
	gcc -c upload.c -o upload.o

msem.o: msem.c
	gcc -c msem.c -o msem.o

main.o: main.c
	gcc -c main.c -o main.o

clean:
	rm -f main *.o
