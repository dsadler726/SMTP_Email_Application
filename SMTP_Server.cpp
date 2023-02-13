/*
    Author: Dakota Sadler
    Date: June 15th, 2022
    SMTP_Server.cpp
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
#include <ctime>

using namespace std;

namespace fs = filesystem;

struct email{
    string MAILFROM; // Reverse-Path
    string RCPTTO; // Forward-Path
    string DATA; // Mail-Data
    
    string header;
    string subject = "None";
    string body;

    string filePath;
};

struct clientInstance{
    int sockNum = 0;
    char buffer[4096];
    string message = "";
    int greeting = 0;
    int commandSequence = 0;
    email mail;
    int DATA_Mode = 0;
    string serverReply = "None";
    int quitCheck = 0;
};


//globals
string hostname;

//Prototypes
void SMTP_Driver(int); 
void command_Driver(clientInstance& client);
bool domainCheck (string String);
void DATA_Processing(clientInstance& client);
string traceRecord(clientInstance& client);
string currentTimeStamp();
void fileCreate(clientInstance& client);
string helpCommand(string argm);

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]){
    int sockfd, newsockfd, portno, pid;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    char hostbuffer[256];
    struct hostent *host_entry;
    email reset;

    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    hostname = string(hostbuffer);

    if (argc < 2) error("ERROR, no port provided\n");

    //Server Side Start up Greeting
    cout << "\nSMTP Server is now Online and awaiting connections....\n" << endl;

    // Creating a directory
    mkdir("db", 0777);
    cout << "\n[SMTP] Database Directory Created" << endl;

    //TCP Socket Set Up
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) error("ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    //Setup to allow multiple users
    while (1) {
        newsockfd = accept(sockfd, 
            (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) error("ERROR on accept");
        pid = fork();
        if (pid < 0)
            error("ERROR on fork");
        if (pid == 0)  {
            close(sockfd);
            SMTP_Driver(newsockfd); //Processing each Process
            exit(0);
        }
        else close(newsockfd);
    } 
    close(sockfd);
    return 0; 
}

void SMTP_Driver (int sock){
    int n;
    char buffer[4096];
    clientInstance client;
    client.sockNum = sock;
    int isSubject = 0;
    email reset;

    //Welcome Message
    string welcomeMessage = "220 " + hostname + " - Service Ready"; //RESPONSE CODE
    cout << "[SMTP] Server Reply to Client: " + welcomeMessage << endl;
    strcpy(client.buffer, welcomeMessage.c_str());

    //Connection Message
    n = write(client.sockNum,client.buffer,strlen(client.buffer));

    while(client.quitCheck == 0){
        bzero(buffer,4096);
        n = read(client.sockNum,buffer,4096); //PRIME INBOUND
        client.message = string(buffer);
        if (n < 0) error("ERROR reading from socket");
        
        //Non DATA-Mode
        if(client.DATA_Mode == 0){ 
            command_Driver(client);
            bzero(client.buffer,4096);
            strcpy(client.buffer, client.serverReply.c_str());
            cout << "[SMTP] Server Reply to Client: " + client.serverReply + "\n" << endl;
            n = write(client.sockNum,client.buffer,strlen(client.buffer)); //PRIME OUTBOUND
            if (n < 0) error("ERROR writing to socket");
        }

        //DATA-Mode (Data Mode = 1)
        else{
            if(isSubject == 0){
                client.mail.subject = client.message;
                isSubject = 1;
            }else{
                string temp = client.message;
                temp.erase(temp.size() - 1);
                if(temp.compare(".") == 0){
                    client.DATA_Mode = 0; //Ends DATA-Mode
                    client.mail.DATA.erase(client.mail.DATA.size() - 1);
                    client.mail.DATA = client.mail.DATA + ".";//appends to end per SMTP RFC
                    DATA_Processing(client); //Processes Lines entered
                    client.serverReply = "250 OK Data Confirmed"; //REPLY CODE

                    bzero(client.buffer,4096);
                    strcpy(client.buffer, client.serverReply.c_str());
                    cout << "[SMTP] Server Reply to Client: " + client.serverReply + "\n" << endl;
                    n = write(client.sockNum,client.buffer,strlen(client.buffer)); //DATA-MODE OUTBOUND

                    client.commandSequence = 0; //resets command order
                    isSubject = 0; //resets subject field
                    client.mail = reset;

                }else{
                    //Appends new line to previous
                    client.mail.DATA = client.mail.DATA + client.message;
                }
            }
            
        }
    }
}

void command_Driver(clientInstance& client){
    string tempReply;
    istringstream ss(client.message);
    string word;
    string commandArguements[10];
    int commandCount = -1;

    //Parsing the Client
    int i = 0;
    while(ss >> word){
        commandArguements[i] = word;
        i++;
    }
    commandCount = i;

    ///// Pre Greeting Commands - No greeting needed /////

    //HELO Command 
    if(commandArguements[0].compare("HELO") == 0 && commandArguements[1].compare(hostname) == 0){
        client.serverReply = "250 OK - HELO"; //REPLY CODE
        client.greeting = 1;
    }

    //HELP Command
    else if(commandArguements[0].compare("HELP") == 0){
        client.serverReply = helpCommand(commandArguements[1]);
    }

    //Bad Helo Command
    else if(commandArguements[0].compare("HELO") != 0 && client.greeting != 1){
        client.serverReply = "503 Bad Sequence of Commands - No HELO"; //REPLY CODE
    }

    //QUIT Command
    else if(commandArguements[0].compare("QUIT") == 0){
        client.serverReply = "221 OK"; //REPLY CODE
        client.quitCheck = 1;
    }

    ///// Pre Greeting Commands - Greeting needed /////

    else if(client.greeting == 1){

        //MAIL FROM: Command to set the email sender
        if(commandArguements[0].compare("MAIL") == 0 && commandArguements[1].compare("FROM:") == 0 && client.commandSequence == 0){
            email resetMail;
            client.mail = resetMail;
            string temp = commandArguements[2];
            temp.erase(remove(temp.begin(), temp.end(), '<'), temp.end());
            temp.erase(remove(temp.begin(), temp.end(), '>'), temp.end());

            bool suffix = domainCheck(temp);
            if(suffix == true){
                client.mail.MAILFROM = temp;
                client.commandSequence = 1;
                cout << "[SMTP] Client Inititated Mail Creation" << endl;
                client.serverReply = "250 OK - MAIL"; //REPLY CODE
            }else{
                client.serverReply = "553 Request Not Taken - Name Not Allowed"; //REPLY CODE
            }
        }else if(commandArguements[0].compare("MAIL") == 0 && commandArguements[1].compare("FROM:") == 0 && client.commandSequence != 0){
            client.serverReply = "503 Bad Sequence of Commands - MAIL in Progress"; //REPLY CODE
        }

        //RCPT TO: Command to set the recipient
        else if(commandArguements[0].compare("RCPT") == 0 && commandArguements[1].compare("TO:") == 0 && client.commandSequence == 1){
            string temp = commandArguements[2];
            temp.erase(remove(temp.begin(), temp.end(), '<'), temp.end());
            temp.erase(remove(temp.begin(), temp.end(), '>'), temp.end());

            bool suffix = domainCheck(temp);
            if(suffix == true){
                cout << "[SMTP] Recipient Set to: " << temp << endl;
                client.mail.RCPTTO = temp;
                client.commandSequence = 2;
                //creates a directory for the recipient
                string recipientDirectory = temp;
                recipientDirectory = recipientDirectory.substr(0, recipientDirectory.size()-11);                
                client.mail.filePath = "db/" + recipientDirectory;
                mkdir(client.mail.filePath.c_str(), 0777);
                client.serverReply = "250 OK - RCPT TO"; //REPLY CODE
            }else{
                client.serverReply = "553 Request Not Taken - Name Not Allowed"; //REPLY CODE
            }
        }else if(commandArguements[0].compare("RCPT") == 0 && commandArguements[1].compare("TO:") == 0 && client.commandSequence < 1){
            client.serverReply = "503 Bad Sequence of Commands - MAIL Before RCPT"; //REPLY CODE
        }

        //Data command - Sets the server to DATA Mode
        else if(commandArguements[0].compare("DATA") == 0 && commandCount == 1 && client.commandSequence == 2){
            client.DATA_Mode = 1;
            cout << "[SMTP] DATA-Mode started by client" << endl;
            client.serverReply = "354 Enter Mail Input, First line is subject header, End with '.'"; //REPLY CODE           
        }else if(commandArguements[0].compare("DATA") == 0 && client.commandSequence < 2 && commandCount == 1){
            client.serverReply = "503 Bad Sequence of Commands"; //REPLY CODE
        }else if(commandArguements[0].compare("DATA") == 0  && commandCount > 1){
            client.serverReply = "501 Syntax Error in Parameters - Only DATA Parameter Allowed"; //REPLY CODE
        }

        //Unrecognized Command
        else{
            cout << "[SMTP] Client Command Unrecognized" << endl;
            client.serverReply = "500 Syntax Error - Command Unrecognized"; //REPLY CODE
        }
    }
}

bool domainCheck(string String) {
    string ending = "@server.com";

    if(0 == String.compare(String.length() - ending.length(), ending.length(), ending)){
        return true;
    }else{
        return false;
    }
}

string helpCommand(string argm){
    string message = "214 Help Menu: \n"
            "      > HELO <Hostname> \n"
            "           - Sends a greeting to the Server \n"
            "           - Need to greet server in order to begin proper interaction \n"

            "      > MAIL FROM: <User> \n"
            "           - Sends to the server who is sending the email \n"
            "           - HELP MAIL for more information \n"

            "      > RCPT TO: <User> \n"
            "           - Sends to the server who the email is being sent to\n"
            "           - HELP RCPT for more information \n"

            "      > DATA \n"
            "           - Sets the server to DATA Mode and allows the user to enter the email body\n"
            "           - HELP DATA for more information \n"

            "      > MISC. Commands: \n"
            "           - HELP - Returns a list of useful commands to the user \n"
            "           - QUIT - ends client interactions with the server \n \n";

    if(argm.compare("MAIL") == 0){
        message = message + "------------------------------------------------------------ \n"
            "      > More Info: MAIL \n"
            "           - Proper Syntax: MAIL FROM: <User> \n"
            "           - Syntax Example: MAIL FROM: username@447m22 \n"
            "           - <User> has to follow the application domain of '@447m22', deviations will be rejected \n"
            "           - '<>' can be added if user wishes, though can proceed without \n"
            "           - Starts the mail creation process, advances the sequence \n"
            "           - Following command in the sequence is RCPT \n";
    }
    if(argm.compare("RCPT") == 0){
        message = message + "------------------------------------------------------------ \n"
            "      > More Info: RCPT \n"
            "           - Proper Syntax: RCPT TO: <User> \n"
            "           - Syntax Example: RCPT TO: username@447m22 \n"
            "           - <User> has to follow the application domain of '@447m22', deviations will be rejected \n"
            "           - '<>' can be added if user wishes, though can proceed without \n"
            "           - Proceeds the email interaction, always follows MAIL FROM: \n"
            "           - Following command in the sequence is DATA \n";
    }
    if(argm.compare("DATA") == 0){
        message = message + "------------------------------------------------------------ \n"
            "      > More Info: DATA \n"
            "           - Proper Syntax: DATA \n"
            "           - Syntax Example: DATA \n"
            "           - NO other parameters should be entered, else will reply with an error code \n"
            "           - Sets the interaction to DATA Mode \n"
            "           - Allows the user to enter the body of the email \n"
            "           - ends when the user enters a singular '.' on its own line\n"
            "           - Processes the data and creates an email with the user inputs\n";
    }
    return message;
}

void DATA_Processing(clientInstance& client){
    string header = traceRecord(client);
    client.mail.body = header + client.mail.DATA;
    fileCreate(client);
}

string traceRecord(clientInstance& client){
    string traceRecord;
    string Date = currentTimeStamp();

    traceRecord = "Date: " + Date + "\n" + "FROM: <" + client.mail.MAILFROM + "> \n"
        "TO: " + "<" + client.mail.RCPTTO + "> \n" + "Subject: " + client.mail.subject + "\n";
    
    return traceRecord;
}

string currentTimeStamp(){
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

void fileCreate(clientInstance& client){
    int fileCount = 0;
    for (const auto & entry : fs::directory_iterator(client.mail.filePath)){
        fileCount++;
    }

    fileCount = fileCount + 1;

    string fileName;

    if(fileCount < 10){
        //files 1-9
        fileName = "00" + to_string(fileCount) + ".email";
    }else if(fileCount > 9 && fileCount < 100){
        //files 10 - 99
        fileName = "0" + to_string(fileCount) + ".email";
    }else{
        fileName = to_string(fileCount) + ".email";
    }

    client.mail.filePath =  client.mail.filePath + "/" + fileName;
    ofstream out(client.mail.filePath);
    out << client.mail.body;
    out.close();
}