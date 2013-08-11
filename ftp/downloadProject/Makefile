main:main_download.o ftp.o  download.o
	gcc main_download.o  ftp.o   download.o -o  main -lpthread

ftp.o: ftp.c
	gcc -c ftp.c -o ftp.o


download.o: download.c
	gcc -c download.c -o download.o


main_download.o: main_download.c
	gcc -c main_download.c -o main_download.o

clean:
	rm -f main *.o
