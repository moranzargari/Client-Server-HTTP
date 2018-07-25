# Client-Server-HTTP

## HTTP Client
this programming assignment is to write HTTP client. This is not a full implement of HTTP specification, but only a very
limited subset of it.

This program is an HTTP client that constructs an HTTP request based on user’s command line input, sends the request to a Web server, receives the reply from the server, and displays the reply message on screen.It support only IPv4 connections.

how to use it:
*  Compile the client: gcc –o client client.c ,client is the executable file.
*  ./client [–h] [–d time-interval] [URL]: The flags and the url can come at any order, the only limitation is that the time interval should come right after the flag –d.
*  The URL format is http://hostname[:port]/filepath. 

what the program does?
1. Parse the <URL> given in the command line.
2. Connect to the server.
3. Construct an HTTP request based on the options specified in the command line.
4. Send the HTTP request to server.
5. Receive an HTTP response.
6. Display the response on the screen. 
  
Additional functions for client.c:

* urlExists - the method check if the entire input contain url.
* input - the method go over each argument in the input -for checking and handleing.
* Time - the method handle the time input.
* modifyTime - construct the time format for the "If-Modified-Since".
* Url - the method handle the url input.
* findPort - the method extract the port, and check input.
* chkString - the method handle all string failures.
* chkFlags - the method handle duplicate input.
* createRequest - the method bulid the request to the web server.
* getSizeRequest - the method calculate the client request length.
* connectToServer - the method create connection to the web server and send/receiv data.
* killRequest - the method free the struct request variables.
* wrongCommand - the method notify when receive wrong input.
* freeAll - the method free all allocated memory.
* error - the method handle's system call errors. 

defines:
* SUCCESS - equal to 0, use to Announce success.
* FAIL - equal to -1, use to Announce failure.
* RESPONSE_SIZE - equal to 4069, set's response.
* TIMEBUF_SIZE -equal to 128
* case_0 to case_5 - equal to 1-5, for switch case.
  

## HTTP Server
In this programming assignment is to write HTTP server. This is not a full implement of HTTP specification, but only a very limited subset of it. 

the following HTTP server:
- Constructs an HTTP response based on client's request.
- Sends the response to the client. 

Program Description:
there is two source files, server and threadpool.
The server should handle the connections with the clients. when using TCP, a server creates a socket for each client it talks to. In other words, there is always one socket where the server listens to connections and for each client connection request, the server opens another socket. In order to enable multithreaded program, the server should create threads that handle the connections with
the clients. Since, the server should maintain a limited number of threads, it constructs a thread pool. In other words, the server creates the pool of threads in advanced and each time it needs a thread to handle a client connection, it takes one from the pool or enqueue the request if there is no available thread in the pool.

how to use it:
*  Compile the server with the –lpthread flag
*  run ./server [port] ]pool-size] [max-number-of-request], (Port is the port number your server will listen on, pool-size is the number of threads in the pool and number-of-request is the maximum number of request your server will handle before it destroys the
pool.)

what the program does?
1. Read request from socket
2. Check's input: The request first line should contain method, path and protocol. Here, you only
have to check that there are 3 tokens and that the last one is one of the http versions, other
checks on the method and the path will be checked later. In case the request is wrong, send 400 "Bad Request" respond, as in file 400.txt.
3. support only the GET method, if you get another method, return error message "501 not supported", as in file 501.txt
4. return error message "404 Not Found" if the requested path does not exist, as in file 404.txt. The requested path is absolute, i.e. the path from the server root directory.
5. return 302 Found response if path is directory but it does not end with a '/', as in 302.txt. Note that the location header contains the original path + '/'. Real browser will automatically look for the new path. 
6. If path is directory and it ends with a '/', search for index.html.
    *   If there is such file, return it.
    *   Else, return the contents of the directory in the format as in file dir_content.txt.
7. If the path is a file
    *  if the file is not regular or the caller has no 'read' permissions, send 403 Forbidden response, as in file 403.txt. The file has        to have read permission for everyone and if the file is in some directory, all the directories in the path have to have
        executing permissions.
    *  otherwise, return the file, format in file file.txt 
    
    
    
Additional functions for threadpool.c:
* Dequeue-this method remove the first element from the jobs queue.
* enqueue-this method add new job to the jobs queue.
* error-this method handle all failure and free memory.

functions in server.c:
* checkArgv-check the server input.
* hError-this method handle all failure before connecting to server.
* freeAll-this method free all memory.
* extractLine-this method extract the first line frome the client request.
* negofunction-the main negotiation function, the thread job.
* checkLine-this method check the client request first line and choose the correct response.
* countWords-this method check the client request first line.
* extractPath-this method extract the path from the client request first line.
* isDirectory- called in case the path is a directory.
* response- this method build the server response.
* handleStatus- this method manage the correct response.
* readFromFile- when the request is a file this method read and write the file data.
* returnErrorBody- this method build the error response body.
* createDirContent-this method build the dicontent file format to the current directory.
* searchIndexHtml- check if the directory contain file name "index.html".
* EntityList- this method sent an directory entity list.
* OK- this method handle all "200 OK" responses.
* modifyDate- this method return the current date and time.
* lastModified-this method return file the last modified date and time.
* getFileSize- this method return the file size.
* readPermission- this method check for directory/file read/execute permmissions.
* spaces-this method check for directories and files that contain spaces.

Thank you!

 



