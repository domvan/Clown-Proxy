// ClownProxy.cpp
// CPSC 441 W22 - Assignment 1
// Written by: Dominic Vandekerkhove (30091829)
// Code snippets taken from Bardia Abhardi's T02/T03 tutorial notes
// Submitted: Jan 28 2022


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>

//Global Variables
int mysocket1, mysocket2, wsSocket; // Sockets
int PROXYPORT = 6789; // port number for proxy
int BUF_SIZE = 1024; // maximum request size (1Kb)
char CLOWN[] = "GET http://pages.cpsc.ucalgary.ca/~carey/CPSC441/ass1/clown1.png HTTP/1.1"; 

void receiveWebResponse();
void replaceImages(char* line);
void makeGetRequest(char* servReq, char* line, char* host);
void closeSockets(int sig);
void handleText(char* servRes);
void handleHTML(char* servRes);

int main(){
    struct sockaddr_in proxyAddr; // proxy socket address
    char cliReq[BUF_SIZE]; // Client Requests
    char servReq[BUF_SIZE]; // Server Requests
    char host[BUF_SIZE], url[BUF_SIZE], path[BUF_SIZE]; // For parsing requests
    
    // Close sockets on exit
    signal(SIGTERM, closeSockets);
    signal(SIGINT, closeSockets);

    //create listening socket
    mysocket1 = socket(AF_INET, SOCK_STREAM, 0); 
    if (mysocket1 == -1){
        fprintf(stderr, "Failed to create socket\n");
        return 1;
    }

    //Server address initialization
    memset(&proxyAddr, 0, sizeof(proxyAddr));
    proxyAddr.sin_family = AF_INET; //IPv4 domain
    proxyAddr.sin_port = htons(PROXYPORT); // connect to specified port
    proxyAddr.sin_addr.s_addr = htonl(INADDR_ANY); // local host (127.0.0.1)

    //Bind listening socket to server address
    int bindStatus;
    bindStatus = bind(mysocket1, (struct sockaddr*) &proxyAddr, sizeof(struct sockaddr_in));
    if (bindStatus == -1){
        fprintf(stderr, "Failed to bind\n");
        return 1;
    }
    
    // Start listening on socket
    int listenStatus;
    listenStatus = listen(mysocket1, 100); //100 active participants can wait for connection
    if (listenStatus == -1){
        fprintf(stderr, "Failed to start listening\n");
        return 1;
    }

    // continuously listen for requests
    while(1){

        //Accept connection request
        mysocket2 = accept(mysocket1, NULL, NULL); //blocking
        if (mysocket2 == -1){
            fprintf(stderr, "Failed to accept connection\n");
            return 1;
        }
        
        // Receive browser message
        bzero(cliReq, BUF_SIZE); // Free client request
        int recStatus;
        recStatus = recv(mysocket2, cliReq, sizeof(cliReq), 0);
        if (recStatus == -1){
            fprintf(stderr, "Failed to receive data\n");
            return 1;
        }

        // Retrieve request 
        char* line = strtok(cliReq, "\r\n");

        // Locate and replace image(s)
        replaceImages(line);
        
        //Establish url
        printf("HTTP request:%s\n", line);
        sscanf(line,"GET http://%s", url);

        // Loop through url to copy hostname
        int index;
        for (index = 0; index < strlen(url); index++){
            if (url[index] == '/'){
                strncpy(host, url, index); // copy hostname
                host[index] = '\0'; // space at end of host
                break;
            }
        }
      
        //Establish Path
        bzero(path, BUF_SIZE); // Free path 
        strcat(path, &url[index]);

        // Make GET request
        makeGetRequest(servReq, line, host);

        // Store web server address
        struct sockaddr_in servAddr;
        struct hostent* serv;
        serv = gethostbyname(host);
        if (serv == NULL){
            fprintf(stderr, "Failed to get host name\n");
        }

        // Establish web server socket
        bzero( (char*) &servAddr, sizeof(servAddr)); // free server address
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(80);
        bcopy( (char*) serv->h_addr, (char*) &servAddr.sin_addr.s_addr, serv->h_length); 

        //Create web server socket
        wsSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (wsSocket == -1){
            fprintf(stderr, "Failed to create web server socket\n");
            return 1;
        }
        
        //Connect web server socket
        int connStatus;
        connStatus = connect(wsSocket, (struct sockaddr *) &servAddr, sizeof(servAddr));
        if (connStatus == -1){
            fprintf(stderr, "Failed to connect to web server socket\n");
            return 1;
        }

        // Send request from client to server
        int sendStatus;
        sendStatus = send(wsSocket, servReq, sizeof(servReq), 0);
        if (sendStatus == -1){
            fprintf(stderr, "Failed to send request to server\n");
            return 1;
        }
        
        // Receive response from web
        receiveWebResponse();
    }

    return 0;
}

void receiveWebResponse(){
    int bytes; // number of transmitted bytes 
    char cliRes[BUF_SIZE*10]; // Client Response
    char servRes[BUF_SIZE*10]; // Server Response
    char reqType[BUF_SIZE]; // Parsing HTTP response type

    while ( (bytes = recv(wsSocket, servRes, sizeof(servRes), 0)) > 0){
        //Determine content type
        char* typePtr = strstr(servRes, "Content-Type: ");
        if (typePtr != NULL){
            strncpy(reqType, typePtr, strcspn(typePtr, ";"));
        }
        
        // Determine how to handle request
        if (strcmp(reqType, "Content-Type: text/html") == 0){
            // Handle HTML request
            handleHTML(servRes);
        }
        else if (strcmp(reqType, "Content-Type: text/plain") == 0){
            // Handle plain text request
            handleText(servRes);
        }

        bcopy(servRes, cliRes, bytes);
        if (send(mysocket2, cliRes, bytes, 0) == -1){
            fprintf(stderr, "Failed to send response to client");
        }

        // Free memory
        bzero(cliRes, 10 * BUF_SIZE);
        bzero(servRes, 10 * BUF_SIZE);
        bzero(reqType, BUF_SIZE);
    }
}

void replaceImages(char* line){
    int len = strlen(line);
    char* image = strstr(line, ".jpg");
    if (image != NULL){
        strncpy(line, CLOWN, len); // Replace image with clown
    }
}

void makeGetRequest(char* servReq, char* line, char* host){
    bzero(servReq, BUF_SIZE);
    strcpy(servReq, line);
    strcat(servReq, "\r\n");
    strcat(servReq, "Host: ");
    strcat(servReq, host);
    strcat(servReq, "\r\n\r\n");
}

void closeSockets(int sig){
    close(mysocket1);
    close(mysocket2);
    close(wsSocket);
    exit(0);
}

void handleText(char* servRes){
    char* ptr = strstr(servRes, "Happy");

    // Replace all occurences of "Happy" with "Silly"
    while(ptr != NULL){
        strncpy(ptr, "Silly", 5);
        ptr = strstr(servRes, "Happy");
    }

    // Replace all occurences of "happy" with "silly"
    ptr = strstr(servRes, "happy");
    while (ptr!= NULL){
        strncpy(ptr, "silly", 5);
        ptr = strstr(servRes, "happy");
    }
}

void handleHTML(char* servRes){
    char* iter = servRes;

    // Iterate through response and replace "Happy" with "Silly"
    while (iter != NULL){
        char* image = strstr(iter, "<img src=\""); // next image tag
        char* link = strstr(iter, "<a href=\""); // next link tag
        char* end = image; // end set to image

        // Set end to link if link comes before image
        if (link < image){
            end = link;
        }

        // Ensure end is not NULL unless link and image are null
        if (link == NULL && image == NULL){
            end = NULL;
        }
        else if (link == NULL){
            end = image;
        }
        else{
            end = link;
        }

        // Last image has been found
        if (end == NULL){
            char* ptr = strstr(servRes, "Happy");

            // Replace all occurences of "Happy" with "Silly"
            while(ptr != NULL){
                strncpy(ptr, "Silly", 5);
                ptr = strstr(servRes, "Happy");
            }

            ptr = strstr(servRes, "happy");

            // Replace all occurences of "happy" with "silly"
            while(ptr != NULL){
                strncpy(ptr, "silly", 5);
                ptr = strstr(servRes, "happy");
            }
            break;
        }
        // More images in file
        else{
            // // Determine length of image tag
            int length = strcspn(end, ">");
            
            // Replace all occurences (before image) of "Happy" with "Silly" 
            char* upPtr = strstr(servRes, "Happy");
            while(upPtr != NULL && iter < end){
                strncpy(upPtr, "Silly", 5);
                upPtr = strstr(servRes, "Happy");
            }

            char* lowerPtr = strstr(servRes, "happy");
            while (lowerPtr != NULL && iter < end){
                strncpy(lowerPtr, "silly", 5);
                lowerPtr = strstr(servRes, "happy");
            }
            
            iter = end + length;
        }
    }
}