
main:main.o upload.o ftp.o
	gcc main.o upload.o ftp.o  -o  main -lpthread

ftp.o: ftp.c
	gcc -c ftp.c -o ftp.o


download.o: download.c
	gcc -c download.c -o download.o

upload.o: upload.c
	gcc -c upload.c -o upload.o


main.o: main.c
	gcc -c main.c -o main.o 





clean:
	rm -f  *.o
