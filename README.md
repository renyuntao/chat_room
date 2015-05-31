# chat_room
A simple multiplayer chat room
# Synopsis
    This is a simple multiplayer chat room
# How to use the program?
You can compile the server program as      
**$ gcc chat_server.c -lpthread -o server**  
then run as     
**$ ./server \<port\>**  
Next,compile the client program as      
**$ gcc chat_clnt.c -lpthread -o client**   
then run as  
**$ ./client 127.0.0.1 \<IP\> \<Port\> \<YourName\>**
