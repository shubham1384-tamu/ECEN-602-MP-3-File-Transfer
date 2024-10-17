# ECEN-602-MP-3-File-Transfer

This project implements the Trivial File Transfer Protocol (TFTP) server. TFTP is tested on transfrerring different files between two a server and a client using the User Datagram Protocol (UDP). We have implemented the server side only, while for the client side we installed the tftp package (sudo apt-get install tftp-hpa) and used it to create client instances to perfom both Read Request (RRQ) and Write Request (WRQ) functions.


## Steps for execution



## Test Case Execution

1. Transfer a binary file of size=2048 Bytes and check that it matches the source file.
 
In this test case, 


2. Transfer a binary file of size=2047 Bytes and check that it matches the source file.
 
In this test case, three clients are connected to the server. The server is able to broadcast messages sent by clients alongside their usernames.

Client Side Terminal
![Test Case 2 client terminal screenshot](Screenshots/TS2_client_terminal.png)

Server Side Terminal
![Test Case 2 server terminal screenshot](Screenshots/TS2_server_terminal.png)

Client Side Directory Before & After
![Test Case 2 client directory screenshot](Screenshots/TS2_client_Directory.png)

Files Comparison
![Test Case 2 files comparison screenshot](Screenshots/TS2_client_files_comparison.png)




