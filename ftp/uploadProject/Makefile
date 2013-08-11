main:main_upload.o upload.o ftp.o log.o  upload.o
	gcc main_upload.o  ftp.o log.o  upload.o -o  main -lpthread

ftp.o: ftp.c
	gcc -c ftp.c -o ftp.o

log.o:log.c
	gcc -c log.c -o log.o


upload.o: upload.c
	gcc -c upload.c -o upload.o


main_upload.o: main_upload.c
	gcc -c main_upload.c -o main_upload.o

clean:
	rm -f main *.o
