/*
    Author: Dakota Sadler
    Date: June 15th, 2022
    SMTP_Client.cpp
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <string>
#include <iostream>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

using namespace std;

int main(int argc, char *argv[]){
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int exitCheck = 0;
    char buffer[4096];

    if (argc < 3) {
       cout << "Improper arguements" << endl;
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        cout << "Error No Host" << endl;
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
        error("ERROR connecting");
    }

    //Welcome Message
    bzero(buffer,4096);
    n = read(sockfd,buffer,4096);
    cout << "\n" + string(buffer) << endl;
    bzero(buffer,4096);

    while(1){

        //User Input
        cout << ">>";
        bzero(buffer,4096);
        fgets(buffer,4096,stdin);
        n = write(sockfd,buffer,strlen(buffer));
        if (n < 0) error("ERROR writing to socket");

        //Server Response
        bzero(buffer,4096);
        n = read(sockfd,buffer,4096);
        cout << "\n" + string(buffer) << endl;

        //Email Data Entry
        if (strncmp(buffer, "354 ", 4) == 0) {
           string DataLine;
           while(DataLine.compare(".") != 0){
                cout << ">>> ";
                bzero(buffer,4096);
                fgets(buffer,4096,stdin);
                DataLine = string(buffer);
                DataLine.erase(DataLine.size() - 1);
                n = write(sockfd,buffer,strlen(buffer));
                bzero(buffer,4096);
                if(DataLine.compare(".") == 0){
                    n = read(sockfd,buffer,4096);
                    cout << "\n" + string(buffer) << endl;
                    bzero(buffer,4096);
                }
           }
        }

        if (strncmp(buffer, "221 ", 4) == 0) {
            cout << "Close Session Request Accepted - Goodbye" << endl;
            close(sockfd);
            return 0;
        }
        if (n < 0) error("ERROR reading from socket");
    }
    
    
}