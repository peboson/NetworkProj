NAME
    ftps ftpc

AUTHOR
    Jason Song.988

INSTALL
    make

UNINSTALL
    make clean

USAGE
    ftps [local_port]
    ftpc remote_IP remote_port local_file_to_transfer

DESCRIPTION
    This is the Lab2 for course CSE 5462 Network Programming

    ftps will receive the file that ftpc sends
    ftps will listen on local_port, receive multiple files until stop manually by ctrl-C.
    ftpc will send local_file_to_transfer to remote_IP:remote_port. One file will be sent during each run with one connection.

FILES
    ftpc.c
    This is the source file for ftpc, which contains the logic to send one file to server.

    ftps.c
    This is the source file for ftps, which contains the server logic to receive files.

TEST
    To run built-in test, simply compile all first with
        make
    And then start the server with
        make testsrv
    This will start a server listen on port 1040. Finally run
        make testcli1
    This will send client/1.jpg to localhost:1040, you will see 1.jpg in server directory.

    To test manually, compile first with
        make
    Then start server with
        cd server && ./ftps local_port
    Finally send file in client directory, run the following command on another computer
        cd client && ./ftpc remote_IP remote_port local_file_to_transfer
    The file will be sent to server directory.


