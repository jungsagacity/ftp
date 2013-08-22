main_upload:main_upload.o upload.o download.o ftp.o utility.o  upload.o
	gcc main_upload.o  ftp.o download.o utility.o  upload.o -o  main_upload -lpthread

#main_download:main_download.o upload.o download.o ftp.o utility.o  upload.o
#	gcc main_download.o  ftp.o download.o utility.o  upload.o -o  main_download -lpthread

ftp.o: ftp.c
	gcc -c ftp.c -o ftp.o

utility.o:utility.c
	gcc -c utility.c -o utility.o

upload.o: upload.c
	gcc -c upload.c -o upload.o

download.o: download.c
	gcc -c download.c -o download.o

main_upload.o: main_upload.c
	gcc -c main_upload.c -o main_upload.o

main_download.o: main_download.c
	gcc -c main_download.c -o main_download.o

clean:
	rm -f main_upload main_download *.o
