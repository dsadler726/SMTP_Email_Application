/*
    Author: Dakota Sadler
    Date: June 15th, 2022
    HTTP_Client.cpp
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <filesystem>
#include <bits/stdc++.h>
#include <ostream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <algorithm>
#include <fstream>
#include <string>
#include <netdb.h>
#include <cstring>
#include <cctype>

using namespace std;
namespace fs = filesystem;

void error(const char *msg){
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[]){
    string username;
    int numEmails;
    string hostname;
    string GET_Message;
    string confirmation;
    int exitCheck = 0;
    
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char sendBuffer[8192];
    char recieveBuffer[8192];

    if (argc < 3) error("Error - Invalid Arguements");

    //Socket Creation
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    while(1){
        cout << "\nPlease enter Username: ";
        cin >> username;
        cout << "\n";

        //send username to check for emails
        bzero(sendBuffer,8192);
        strcpy(sendBuffer,username.c_str());
        n = write(sockfd,sendBuffer,strlen(sendBuffer));
        if (n < 0) error("ERROR writing to socket");

        //total file count
        bzero(recieveBuffer,8192);
        n = read(sockfd,recieveBuffer,255);
        if (n < 0) error("ERROR reading from socket");
        string response = string(recieveBuffer);

        if(response.compare("-1") == 0){
            cout << "No recorded user, please enter a valid user" << endl;
        }
        else{

            //Download Selection
            cout << "File Count: " << recieveBuffer << "\n" << endl;
            cout << "Enter number of emails to download: ";
            cin >> numEmails;
            cout << "\nPlease enter server hostname: ";
            cin >> hostname;

            //HTTP GET Request 
            string pathName = "/db/" + username + "/";
            GET_Message = "GET " + pathName + " HTTP/1.1 \n";
            GET_Message = GET_Message + "Host: <" + hostname + "> \n";
            GET_Message = GET_Message + "Count: " + to_string(numEmails);

            cout << "\nGenerated GET Request: \n" << endl;
            cout << "\n" << GET_Message << "\n" << endl;

            int GETProceed = 0;

            cout << "Confirmation to proceed?  Y/N \n"
                    << "    Y - Yes\n "
                    << "   N - No\n "
                    << "Choice: ";
            cin >> confirmation;
    
            if(confirmation.compare("Y") == 0){
                mkdir(username.c_str(), 0777);
                cout << "\nFolder created under Username: " + username << endl;;
                GETProceed = 1;
            }else if(confirmation.compare("N") == 0){
                cout << "\nExiting Request" << endl;  
                    close(sockfd);
                    exit(1);
            
            }else{
                //error wrong input
                cout << "/nWrong input, please try again \n" << endl;
            }

            if(GETProceed == 1){

                //Sending the GET Request
                bzero(sendBuffer,8192);
                strcpy(sendBuffer,GET_Message.c_str());
                n = write(sockfd,sendBuffer,strlen(sendBuffer));
                if (n < 0) error("ERROR writing to socket");
                bzero(recieveBuffer,8192);
                
                //GET Response
                n = read(sockfd,recieveBuffer,4095);
                if (n < 0) error("ERROR reading from socket");
                string responseFile = string(recieveBuffer);
                string statusCode;

                istringstream ss(responseFile);
                string word;
                int i = 0;
                while(ss >> word){
                    if(i < 3){
                        statusCode = statusCode + " " + word;
                    }
                    i++;
                }
                statusCode.erase(0,1);
                cout << "\n" << statusCode << endl;

                //Start of saving response to .txt
                string filepath = username + "/";
                int fileCount = 0;
                for (const auto & entry : fs::directory_iterator(filepath)){
                    fileCount++;
                }

                fileCount = fileCount + 1;
                string fileName = "response" + to_string(fileCount) + ".txt";

                filepath = filepath + fileName;

                ofstream file;
                file.open(filepath);
                file << responseFile;
                file.close();

                string askExit;
                cout << "\nwould you like to exit?\n" 
                        << "   Y - Yes\n"
                        << "   or press any key to exit\n"
                        << "   Choice: ";
        
                cin >> askExit;

                if(askExit.compare("Y") == 0 || askExit.compare("y") == 0){
                    n = write(sockfd,"exit",4);
                    cout << "\nNow Exiting" << endl;   
                    close(sockfd);
                    exit(1);
                }
            }
            
        }
    }
    
    return 0;
}