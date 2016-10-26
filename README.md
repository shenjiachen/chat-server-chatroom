#ECEN 602 Project 1<br>
**Instruction:**<br>
**1.**This project achieves the basic function of SBCP, including JOIN SEND and FWD.<br>
**2.**This project add the bonus part, as well as ACK, NAK, ONLINE, OFFLINE and IDLE.<br>
**3.**This project does not include the ipv6 function.<br> 

**Server:**<br>
**1.**Server gets the user input arguments ./server server_IP server_port max_client.<br>
**2.**Server creates a socket and binds it to the IP address and PORT and sets maximum clients.<br>
**3.**The server uses the SELECT method to select activate socket filedescriptor.<br>
**4.**When the server receives a new connection, it receives JOIN and sends ACK/NAK back.<br> 
**5.**If server sends ACK back, it adds the client socket descriptor into the read_fds set, and sends ONLINE to other client in read_fds.<br>
**6.**If server sends NAK back, it means that the client number has reached the maximum.<br>
**7.**The LinkedList struct users is used to store Socket File Descriptor and Username.<br>
**8.**When the server receives message from the socked filedescriptor in read_fd set, it recieves SEND/IDLE and FWD it to other client.<br>
**9.**When the server receives 0 bytes it sends OFFLINE message to all other clients.<br>

**Client:**<br>
**1.**Client gets the user input arguments ./client username server_IP server_port.<br>
**2.**Client connects to the server using the CONNECT system call.<br>
**3.**Client sends JOIN to server and receives ACK/NAK;<br>
**4.**If Client receives ACK, it prints the client number online and who they are.<br>
**5.**If Client receives NAK, it prints the REASON for that.<br>
**6.**Client use while loop and SELECT function to listen between STDIN and server socket.<br>
**7.**if STDIN is actived, client send SEND.<br>
**8.**if server socket filedescriptor is activated, client recieve FWD/ONLINE/OFFLINE/IDLE.<br>

**Useage**<br>
**1.**This project should be compiled by using make first.<br>
**2.**The server should be invoked by using ./server erver_IP server_port max_client.<br>
**3.**The client should be invoked by using ./client username server_IP server_port.<br>
**4.**Start the server first, then connect each client. Type message or command into the terminal and press enter to send.<br>


