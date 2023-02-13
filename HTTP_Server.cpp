/*
    Author: Dakota Sadler
    Date: June 15th, 2022
    HTTP_Server.cpp
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
#include <fstream>

using namespace std;
namespace fs = filesystem;

struct clientInstance{
    string serverResponse;
    string GETcount;
    char buffer[256];
    int badRequest = 0;

    string GETrequest;
    string GEThost;
};


string protocol = "HTTP/1.1";
string hostname;

void HTTP_Driver (int sock);
string createHeader(clientInstance& client);
string TimeStamp();
void ParseGetResponse(clientInstance& client, string message);
int GetEmailNum(string user);

void error(const char *msg){
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, pid;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    char hostbuffer[256];
    struct hostent *host_entry;

    //Returns Hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    hostname = string(hostbuffer);

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    //TCP Socket Setup
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
            sizeof(serv_addr)) < 0) 
            error("ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    cout << "HTTP Server is now online, waiting for connections..." << endl;
    
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) 
            error("ERROR on accept");
        pid = fork();
        if (pid < 0)
            error("ERROR on fork");
        if (pid == 0){
            HTTP_Driver(newsockfd);
            close(newsockfd);
        }
     } 
     close(sockfd);
     return 0; 
}

void HTTP_Driver(int sock){
    int n;
    int exitCheck = 0;
    clientInstance client;

    while(exitCheck == 0){

        //PRE-HTTP Configurations STATELESS
        int preconCheck = 1;
        while(preconCheck == 1){
            //reading the username from client
            bzero(client.buffer,256);
            n = read(sock,client.buffer,256);
            if (n < 0) error("ERROR reading from socket");
            string userName =  string(client.buffer);
            
            //checks if username is present
            int numEmails = GetEmailNum(userName);
            if(numEmails > 0) preconCheck = 0;

            //Sends the number of emails
            strcpy(client.buffer,to_string(numEmails).c_str());
            n = write(sock,client.buffer,strlen(client.buffer));
        }

        //START of GET reception
        bzero(client.buffer,256);
        n = read(sock,client.buffer,256);
        string GETmessage = string(client.buffer);
        
        //Parse GET Request into respective allocations
        ParseGetResponse(client, GETmessage);
        
        //GET Line Parsing
        istringstream ssGET(client.GETrequest);
        string GETLine[10];
        string word;
        int i = 0;
        while(ssGET >> word){
            GETLine[i] = word;
            i++;
        }

        //GET Line attributes
        client.GETrequest = GETLine[0];
        string clientPath = GETLine[1];
        string clientProtocol = GETLine[2];

        //GET Check
        if(client.GETrequest.rfind("GET", 0) == 0) { 
            cout << "\n[HTTP] GET Request Acknowledged" << endl;
        }else{
            client.serverResponse = protocol + " 400 Bad Request";
            client.badRequest = 1;
        }

        //Hostname Line Parsing
        istringstream ssHost(client.GEThost);
        string hostLine[10];
        i = 0;
        while(ssHost >> word){
            hostLine[i] = word;
            i++;
        }
        client.GEThost = hostLine[1];
        client.GEThost.erase(remove(client.GEThost.begin(), client.GEThost.end(), '<'), client.GEThost.end());
        client.GEThost.erase(remove(client.GEThost.begin(), client.GEThost.end(), '>'), client.GEThost.end());

        //Count Line Parsing
        istringstream ssCount(client.GETcount);
        string countLine[10];
        i = 0;
        while(ssCount >> word){
            countLine[i] = word;
            i++;
        }
        client.GETcount = countLine[1];

        //Host Check
        if(client.GEThost.compare(hostname) == 0){
            cout << " " << endl;
            cout << "[HTTP] Host Acknowledged" << endl;
        }else{
            cout << " " << endl;
            cout << "Host Not Accepted" << endl;
            client.serverResponse = protocol + " 400 Bad Request";
            client.badRequest = 1;
        }

        //Getting total files
        int totFileCount = 0;
        clientPath.erase(0,1); //Clears the first / in the GETPath
        string FileCountString;

        for (const auto & entry : fs::directory_iterator(clientPath)){ totFileCount++; }
        cout << "[HTTP] Email count: " << totFileCount << endl;
            
        if(stoi(client.GETcount) > totFileCount){
            client.serverResponse = protocol + " 404 Not Found";
            client.badRequest = 1;
        }

        //Proceeds with download, as long as Instance hasnt found an error
        if(client.badRequest == 0){
            cout << "GET Request accepted" << endl;
            client.serverResponse = protocol + " 200 OK";
            vector<string> SelectedEmails;
            int j = 0;
            
            //Loads fileNames
            for (const auto & entry : fs::directory_iterator(clientPath)){
                if(j < stoi(client.GETcount)){
                    SelectedEmails.push_back(entry.path());
                    j++;
                }
            }

            //Prints serverside list of selected files
            cout << "\n[HTTP] Client Selected files:" << endl;
            for (int i = 0; i < stoi(client.GETcount); i++){
                cout << SelectedEmails[i] << endl;
            }

            string header = createHeader(client);
            client.serverResponse = header;

            string emailText;
            string temp;
            int k = 1;

            for (int i = 0; i < stoi(client.GETcount); i++){
                temp = temp + "\n\n" + "Message: " + to_string(k) + "\n";
                ifstream ReadFile(SelectedEmails[i]);
                while (getline (ReadFile, emailText)){
                    temp = temp + "\n" + emailText;
                }
                k++;
                ReadFile.close();
            }
            temp.erase(0,1);
            client.serverResponse = client.serverResponse + temp;

            //Delete downloaded files
            for (int i = 0; i < stoi(client.GETcount); i++){
                std::filesystem::remove(SelectedEmails[i]);
            }

            cout << "Server Reply:" << endl;
            cout << client.serverResponse << endl;

            strcpy(client.buffer,client.serverResponse.c_str());
            n = write(sock,client.buffer,strlen(client.buffer));

        }else{
            strcpy(client.buffer,client.serverResponse.c_str());
            n = write(sock,client.buffer,strlen(client.buffer));
        }

        bzero(client.buffer,256);
        n = read(sock,client.buffer,256);
        string ex = string(client.buffer);
        if(ex.compare("exit") == 0){
            exitCheck = 1;
        }else{
            n = write(sock,"continue",8);
        }

    }
}

string createHeader(clientInstance& client){
    string header;
    string time = TimeStamp();
    header = client.serverResponse + "\n" 
        "Server: <" + hostname + "> \n"
        "Last-Modified: " + time + "\n"
        "Count: " + client.GETcount + "\n";
        "Content-Type: text/plain \n";
        
    return header;
}

string TimeStamp(){
    string timeStamp;
    time_t timer;
    struct tm * ti;
    time(&timer);
    ti = localtime(&timer);
    timeStamp = asctime(ti);

    timeStamp.erase(timeStamp.size() - 1);
    istringstream ss(timeStamp);
    string word;
    string TA[10];
    int i = 0;

    while(ss >> word){
        TA[i] = word;
        i++;
    }
    
    timeStamp = TA[0] + ", " + TA[1] + " " + TA[2] + " " + TA[4] + " " + TA[3] + " -0600"; //timezone;
    return timeStamp;
}

int GetEmailNum(string user){
    int totEmails = 0;
    string CHECKuserName;
    string userPath = "db/" + user + "/";
    try{
        cout << "[HTTP] Client Username: " << user << endl;
        for (const auto & entry : fs::directory_iterator(userPath)){
            totEmails++;
        }
        cout << "[HTTP] " + user + " email count: " << totEmails << endl;
    }
    catch(const std::exception& e){
        cout << "[HTTP] No username recorded under name " << CHECKuserName << endl;
        totEmails = -1;
    }
    return totEmails;
}

void ParseGetResponse(clientInstance& client, string message){
    int counter = 0;
    string temp;
    stringstream GET(message);
    while(getline(GET,temp,'\n')){
        counter++;
        if(counter == 1){
            client.GETrequest = temp; //Sets Protocol/GET Line
        }else if(counter == 2){
            client.GEThost = temp; //Sets the Hostname Line
        }else if(counter == 3){
            client.GETcount = temp; //Sets the FileCount Line
        }else{
            cout << "Error Occured while Parsing" << endl;
        }
    }
}