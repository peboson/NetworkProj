all:ftps ftpc tcpd timer driver

ftps : ftps.c functions.c
	gcc -o ftps ftps.c functions.c

ftpc : ftpc.c functions.c
	gcc -o ftpc ftpc.c functions.c

tcpd : tcpd.c
	gcc -o tcpd tcpd.c

timer : timer.c
	gcc -o timer timer.c

driver : driver.c
	gcc -o driver driver.c
