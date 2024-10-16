# ECEN-602-MP-3-File-Transfer
# Group#8 (Shubham Santosh & Ibrahim Shahbaz )

This project implements the Trivial File Transfer Protocol (TFTP) server. TFTP is tested on transfrerring different files between two a server and a client using the User Datagram Protocol (UDP). We have implemented the server side only, while for the client side we installed the tftp package (sudo apt-get install tftp-hpa) and used it to create client instances to perfom both Read Request (RRQ) and Write Request (WRQ) functions.

# Architecture

**The Architecure we followed for MP3 is shown below**
![MP3 Architecture](MP3_architecture.jpg)


# Steps to run the file:
 
1. To generate .out file, enter
make all
 
2. To execute TFTP server code:
make tftp <IP> <PORT>
e.g. make tftp 127.0.0.1 1200
 
 
On the client side, switch to a different directory and enter the following for **RRQ**:
1. tftp 127.0.0.1 1200 (in case of the example used above)
2. tftp> get <file_name>
Available file names: bin_2047.bin, bin_2048.bin, binary_file_34MB.bin, two_cr.txt, lf.txt(long text file),etc.

On the client side, switch to a different directory and enter the following for **WRQ**:
1. tftp 127.0.0.1 1200 (in case of the example used above)
2. tftp> mode {binary|netascii}
3. tftp> put <file_name_on_client_directory> <file_name_on_client_directory>
Available file names: bin_2047.bin, bin_2048.bin, binary_file_34MB.bin, two_cr.txt, lf.txt(long text file),etc.


## Test Case Execution

1. **Transfer a binary file of size=2048 Bytes and check that it matches the source file.**
 
In this test case, a file of size 2048 Bytes is transfred from server to client directory and we check that it matches the source file.

**Client Side Terminal**
![Test Case 1 client terminal screenshot](Screenshots/TS1_client_terminal.png)

**Server Side Terminal**
![Test Case 1 server terminal screenshot](Screenshots/TS1_server_terminal.png)

**Client Side Directory Before & After**
![Test Case 1 client directory screenshot](Screenshots/TS1_client_Directory.png)

**Files Comparison**
![Test Case 1 files comparison screenshot](Screenshots/TS1_client_files_comparison.png)

2. **Transfer a binary file of size=2047 Bytes**
 
In this test case, a file of size 2047 Bytes is transfred from server to client directory and we check that it matches the source file.

**Client Side Terminal**
![Test Case 2 client terminal screenshot](Screenshots/TS2_client_terminal.png)

**Server Side Terminal**
![Test Case 2 server terminal screenshot](Screenshots/TS2_server_terminal.png)

**Client Side Directory Before & After**
![Test Case 2 client directory screenshot](Screenshots/TS2_client_Directory.png)

**Files Comparison**
![Test Case 2 files comparison screenshot](Screenshots/TS2_client_files_comparison.png)


3. **Transfer a netascii file that includes two CR’s**
 
In this test case, a netascii file that includes two CR’s is transfered from server to client, and we check that the resulting file matches the input file.

**Client Side Terminal**
![Test Case 3 client terminal screenshot](Screenshots/TS3_client_terminal.png)

**Server Side Terminal**
![Test Case 3 server terminal screenshot](Screenshots/TS3_server_terminal.png)

**Client Side Directory Before & After**
![Test Case 3 client directory screenshot](Screenshots/TS3_client_Directory.png)

**Files Comparison**
![Test Case 3 files comparison screenshot](Screenshots/TS3_client_files_comparison.png)


4. **Transfer a binary file of size 34MB**
In this test case, we transfer a binary file of 34MB and see if block number wrap-around works.

**Client Side Terminal**
![Test Case 4 client terminal screenshot](Screenshots/TS4_client_terminal.png)

**Server Side Terminal**
![Test Case 4 server terminal screenshot](Screenshots/TS4_server_terminal.png)

**Client Side Directory After Transfer**
![Test Case  client directory screenshot](Screenshots/TS4_client_Directory.png)


5. **Try to transfer an invalid file**
In this test case, we check that we receive an error message if we try to transfer a file that does not exist and that our server cleans up and the child process exits.

**Client Side Terminal**
![Test Case 5 client terminal screenshot](Screenshots/TS5_client_terminal.png)

**Server Side Terminal**
![Test Case 5 server terminal screenshot](Screenshots/TS5_server_terminal.png)

6. **Transfer a file to three client simultaneously**
In this test case, we check that we receive the same file on three different client directories simultaneously.


**Client Side Terminal**
![Test Case 6 client terminal screenshot](Screenshots/TS6_client_terminal.png)


**Server Side Terminal**
![Test Case 6 server terminal screenshot](Screenshots/TS6_server_terminal.png)

**Client#1 Side Directory After Transferring Files Siultanously**
![Test Case 6 client directory screenshot](Screenshots/TS6_client1_Directory.png)
**Client#2 Side Directory After Transferring Files Siultanously**
![Test Case 6 client directory screenshot](Screenshots/TS6_client2_Directory.png)
**Client#3 Side Directory After Transferring Files Siultanously**
![Test Case 6 client directory screenshot](Screenshots/TS6_client3_Directory.png)



7. **Terminate a client while recieving a file**
In this test case, we terminate the TFTP client in the middle of a transfer and see if our TFTP server realizes that the client got dissconnected after 10 timeouts.

**Client Side Terminal**
![Test Case 7 client terminal screenshot](Screenshots/TS7_client_terminal.png)

**Server Side Terminal**
![Test Case 7 server terminal screenshot](Screenshots/TS7_server_terminal.png)

8. **Bonus Feature: WRQ for a Binary file**
In this test case, we implement the WRQ bonus feature on both binary and netascii files.


**Client Side Terminal**
![Test Case 8 client terminal screenshot](Screenshots/TS8_client_terminal.png)

**Server Side Terminal**
![Test Case 8 server terminal screenshot](Screenshots/TS8_server_terminal.png)

**Client Side Directory Before & After**
![Test Case 8 client directory screenshot](Screenshots/TS8_client_Directory.png)

**Server Side Directory Before & After**
![Test Case 8 server directory screenshot](Screenshots/TS8_server_Directory.png)

**Binary Files Comparison**
![Test Case 8 binary files comparison screenshot](Screenshots/TS8_binary_files_comparison.png)

**Netascii Files Comparison**
![Test Case 8 netascii files comparison screenshot](Screenshots/TS8_netascii_files_comparison.png)
