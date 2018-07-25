# Client-Server-HTTP

## HTTP Client
this programming assignment is to write HTTP client. This is not a full implement of HTTP specification, but only a very
limited subset of it.

This program is an HTTP client that constructs an HTTP request based on user’s command line input, sends the request to a Web server, receives the reply from the server, and displays the reply message on screen.It support only IPv4 connections.

how to use it:
*  Compile the client: gcc –o client client.c : client is the executable file
*  ./client [–h] [–d <time-interval>] <URL> : The flags and the url can come at any order, the only limitation is that the time interval should come right after the flag –d.
*  The URL format is http://hostname[:port]/filepath. 

what the program does?
1. Parse the <URL> given in the command line.
2. Connect to the server.
3. Construct an HTTP request based on the options specified in the command line.
4. Send the HTTP request to server.
5. Receive an HTTP response.
6. Display the response on the screen. 
  



