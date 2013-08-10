main:main_download.o upload.o ftp.o log.o  msem.o
	gcc main_download.o upload.o ftp.o log.o  msem.o -o  main -lpthread

ftp.o: ftp.c
	gcc -c ftp.c -o ftp.o

log.o:log.c
	gcc -c log.c -o log.o


upload.o: upload.c
	gcc -c upload.c -o upload.o


main_download.o: main_download.c
	gcc -c main_download.c -o main_download.o

clean:
	rm -f main *.o
