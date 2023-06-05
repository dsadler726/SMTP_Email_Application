#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <filesystem>
#include <bits/stdc++.h>
#include <ostream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <gmodule.h>
#include <glib-object.h>

using namespace std;

int exitCheck = 0;

void error(const char *msg){
    perror(msg);
    exit(0);
}

string decodeString(string encoded){
    guchar* temp;
    string decodedString;
    gsize length;
    const gchar* text = encoded.c_str();
    temp = g_base64_decode(text,&length);
    decodedString = (char*) temp;
    return decodedString;
}

string encodeString(string storePassword){
    unsigned char* pass = (unsigned char*) storePassword.c_str();
    string encodedString;    
    encodedString = g_base64_encode(pass,strlen((char*)pass));
    return encodedString;
}

int main(int argc, char *argv[])
{
    char buffer[4096];
    string previousMessage; //stores previous server message, to tell if input needs to be encoded
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    string clientMessage;

    if (argc < 3) {
        exit(0);
    }
    portno = atoi(argv[2]);

    while(exitCheck == 0){
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) 
            error("ERROR opening socket");
        server = gethostbyname(argv[1]);
        if (server == NULL) {
            fprintf(stderr,"ERROR, no such host\n");
            exit(0);
        }
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,server->h_length);
        serv_addr.sin_port = htons(portno);

    
        int resetCheck = 0;
        if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
            error("ERROR connecting");
        }

        while(exitCheck == 0 && resetCheck == 0){
            bzero(buffer,4096);
            n = read(sockfd,buffer,4096);
            if(n > 0){
                cout << " " << endl;
                cout << buffer << endl;
                cout << " " << endl;
            }
            string serverMessage = string(buffer);
            bzero(buffer,4096);

            istringstream ss(serverMessage);
            string word;
            string wordBlocks[20];
            int i = 0;
            while(ss >> word){
                wordBlocks[i] = word;
                i++;
            }

            //if Recieves new password
            if (wordBlocks[0].compare("330") == 0) {
                string newPassword = wordBlocks[1]; //is encoded
                newPassword = decodeString(newPassword); //decodes and stores
                cout << "New Password: " << newPassword << endl;
                cout << "Application will reset in ..." << endl;
                int count = 5;

                while(count >= 1){
                    cout << count << endl;
                    usleep(1000000);
                    count--;
                }
                cout << " " << endl;
                resetCheck = 1;

            }
            
            else if(wordBlocks[0].compare("221") == 0){
                exitCheck = 1;
                cout << "Close Session Request Accepted - Goodbye" << endl;
                close(sockfd);
                exit(1);
            
            }

            if(resetCheck == 0){
                printf(">> ");
                fgets(buffer,4096,stdin);
                clientMessage = string(buffer);
                if(wordBlocks[0].compare("334") == 0 && wordBlocks[1].compare("cGFzc3dvcmQ6") == 0){
                    clientMessage = encodeString(clientMessage); //encodes the password
                    cout << "ENCODED: " << clientMessage << endl;
                    strcpy(buffer, clientMessage.c_str());
                }

                n = write(sockfd,buffer,strlen(buffer));
                if (n < 0) 
                    error("ERROR writing to socket");
            }
            
    
        }
        close(sockfd);
    }
    
    close(sockfd);
    return 0;
}



