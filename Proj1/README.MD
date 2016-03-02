# CSE 5462 Network Programming
# NAME
    ftps ftpc tcpd timer driver

# AUTHOR
    Jason Song.988
    Nathan Turner.1019

# INSTALL
    make

# UNINSTALL
    make clean

# USAGE
    tcpd server|client|port
    ftps [local_port]
    ftpc remote_IP remote_port local_file_to_transfer
    timer
    driver

# DESCRIPTION
    This is the Project1 for course CSE 5462 Network Programming

    ftps will receive the file that ftpc sends
    ftps will listen on local_port, receive multiple files until stop manually by ctrl-C.
    ftpc will send local_file_to_transfer to remote_IP:remote_port. One file will be sent during each run with one connection.

    By default, the ftps must depend on a running tcpd deamon on its system. The ftpc must depend on both running tcpd deamon and troll simulator.

    timer will listen on a local port, and receive start and cancel timer commands from driver.
    drivr will send test commands to timer on a local port.

# FILES
    ftpc.c
    This is the source file for ftpc, which contains the logic to send one file to server.

    ftps.c
    This is the source file for ftps, which contains the server logic to receive files.

    tcpd.c
    This is the source file for tcp deamon, which receives signal of ftpc and ftps, and send data through troll.

    tcpudp.h
    This is the header file for tcp interface, which used by ftp client and ftp server.

    tcpudp.c
    This is the implementation of tcp interface. It sends and receive signal and data to tcp deamon process.

    crc.c
    This is the implementation of CRC32 algorithm.

    timer.c
    This is the source file for timer process, which contains logic to make delta time nodes for a doubly linked list.

    driver.c
    This is the source file for the driver process, which sends start and stop timer commands to the timer process.

# TEST
    To run built-in test, simply compile all first with
        make
    Then start the server deamon on server (beta) with
        make tsrvtcpd
    And 
        make tsrvftps
    in a sperate session of server. This will start a server listen on port 8000. Next run
        make tclitcpd
    on client to start deamon and run
        make ttroll
        >trace
    to start the troll network simulator with trace log. Finally run
        make tcli1
    This will send client/1.jpg to beta:8000, you will see 1.jpg in server directory.

    To test manually, compile first with
        make
    Then start server with
        cd server && ./tcpd server
        cd server && ./ftps local_port
    To start client, run
        cd client && ./tcpd client
        cd Troll && ./troll 8081
        cd client && ./ftpc remote_IP remote_port local_file_to_transfer
    The file will be sent to server directory.

    To start timer, run
	cd client && ./timer
    Then start the driver with
	cd client && ./driver
    The driver will then send commands on a hard-coded local port.

# OTHERS
[a-zA-Z0-9\-\>]*->data
\((struct TimerNodeStruct \*\)($1))
