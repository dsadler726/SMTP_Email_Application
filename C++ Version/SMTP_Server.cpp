#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <filesystem>
#include <bits/stdc++.h>
#include <ostream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <pthread.h>
#include <glib.h>
#include <gmodule.h>
#include <glib-object.h>

using namespace std;

namespace fs = filesystem;

//Email Container
struct email{
    string MAILFROM; // Reverse-Path
    string RCPTTO; // Forward-Path
    string DATA; // Mail-Data
    string header;
    string subject = "None";
    string body;
    string filePath;
};

//Peer Server Container
struct peerServer{
    int n;
    int sock;
    string IP;
    int portno;
    string domain = " ";
    char buffer[4096];
    string message;
    string name;
};

//Client Info Container
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
    int rastapopulous = 0;
    string userName;
    peerServer foreignPeer;
    int AUTH_Mode = 0;
    string address;
    int instanceType = 0; 
    //0 -- Client (Default)
    //1 -- Server
};

//globals
int sock, newsock, portNumber;
int peersock1, peersock2, peersock3; //server peers
socklen_t client_len;
struct sockaddr_in server_addr, client_addr;
string hostname;
string salter = "SNOWY22";
string serverIP;
static string lineData[20];
int configLineCount;
string sourceDomain;
int totalPeers;
peerServer peer01, peer02, peer03;
int psock01, psock02, psock03;
struct sockaddr_in pserver01_addr, pserver02_addr, pserver03_addr;
struct hostent *pserver01;
struct hostent *pserver02;
struct hostent *pserver03;

//Prototypes
void SMTP_Driver(int); 
void command_Driver(clientInstance& client);
bool domainCheck(string address, string domain);
void DATA_Processing(clientInstance& client);
string traceRecord(clientInstance& client);
string currentTimeStamp();
void fileCreate(clientInstance& client);
string helpCommand(string argm);
void AUTH_Processing(clientInstance& client);
string genAlphaNumString();
void storePassword(string password);
string encodeString(string storePassword);
string decodeString(string encoded);
bool usernameLookup(string username);
bool passwordLookup(string password, clientInstance& client);
string* fileLoader(string file, int& num);
void logEntry(string from, string to, string command, string replyCode);
void readConfigFile();
string retrieveHostName(string message);

void error(const char *message){
    perror(message);
    exit(1);
}

//extern "C" void* loadPeers (void*);
void* loadPeers (void*);

int main(int argc, char *argv[]){
    char hostbuffer[256];
    struct hostent *host_entry;
    int pid;
    email reset;
    char *IPbuffer;

    g_thread_new(NULL, loadPeers, NULL);

    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    hostname = string(hostbuffer);
    host_entry = gethostbyname(hostbuffer);
    
    IPbuffer = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));
    serverIP = string(IPbuffer);

    readConfigFile();

    configLineCount = configLineCount - 5; //deletes the source of the config
    totalPeers = configLineCount/3;
    sourceDomain = lineData[1];

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    // Creating a directory
    mkdir("db", 0777);

    //creates User_Password Log -- Resets everytime server restarts
    ofstream userPass("db/.user_pass");
    userPass.close();

    //creates Server_Log -- Resets everytime server restarts
    ofstream serverlog("db/.server_log");
    serverlog.close();

    //TCP Socket Set Up
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) 
        error("ERROR opening socket");
    bzero((char *) &server_addr, sizeof(server_addr));
    portNumber = atoi(argv[1]);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(portNumber);

    if (bind(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) 
        error("ERROR on binding");
    listen(sock,5);
    client_len = sizeof(client_addr);

    //Server Side Start up Greeting
    cout << "\nSMTP Server is now Online and awaiting connections...." << endl;
    
    pid = fork();
    if(pid == 0){
    }else{
        while (1) {
            newsock = accept(sock,(struct sockaddr *) &client_addr, &client_len);
            pid = fork();
            if (pid < 0)
                error("ERROR fork");
            if (pid == 0)  {
                close(sock);
                SMTP_Driver(newsock); //Processing each Process
                exit(0);
            }
            else close(newsock);
        }
    }  
    close(sock);
    return 0; 
}

void SMTP_Driver (int sock){
    int n;
    char buffer[4096];
    clientInstance client;
    client.sockNum = sock;
    int isSubject = 0;
    email reset;
    char* address = inet_ntoa(client_addr.sin_addr);
    client.address = string(address);
    
    string welcomeMessage = "220 " + hostname + " - Service Ready"; //RESPONSE CODE
    logEntry(serverIP, client.address, "Initial ", welcomeMessage);
    strcpy(client.buffer, welcomeMessage.c_str());

    //Connection Message
    n = write(client.sockNum,client.buffer,strlen(client.buffer));

    while(client.quitCheck == 0){
        try{
            bzero(buffer,4096);
            n = read(client.sockNum,buffer,4096); //PRIME INBOUND
            client.message = string(buffer);
            if (n < 0) error("ERROR reading from socket");

            //Non DATA-Mode
            if(client.DATA_Mode == 0){  
                command_Driver(client);
                bzero(client.buffer,4096);
                strcpy(client.buffer, client.serverReply.c_str());
                logEntry(serverIP,client.address,client.message,client.serverReply);
                n = write(client.sockNum,client.buffer,strlen(client.buffer)); //PRIME OUTBOUND
                if (n < 0) error("ERROR writing to socket");            
            }

            //DATA-Mode
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
                        client.mail.DATA = client.mail.DATA + "."; //appends to end per SMTP RFC
                        DATA_Processing(client); //Processes Lines entered
                        client.serverReply = "250 OK Data Confirmed"; //REPLY CODE
                        bzero(client.buffer,4096);
                        strcpy(client.buffer, client.serverReply.c_str());
                        n = write(client.sockNum,client.buffer,strlen(client.buffer)); //DATA-MODE OUTBOUND
                        client.commandSequence = 0; //resets command order
                        isSubject = 0; //resets subject field
                        client.mail = reset;
                    }else{
                        client.mail.DATA = client.mail.DATA + client.message;
                    }
                }  
            }
        }
        catch(const std::exception& e){
            exit(1);
        }
    }
}

void command_Driver(clientInstance& client){
    string tempReply;
    istringstream ss(client.message);
    string word;
    string commandArguements[10];
    int commandCount = -1;
    int i = 0;

    while(ss >> word){
        commandArguements[i] = word;
        i++;
    }
    commandCount = i;

    //HELO Command -- INDEPENDENT
    if(commandArguements[0].compare("HELO") == 0 && commandArguements[1].compare(hostname) == 0){
        client.serverReply = "250 OK - HELO"; //REPLY CODE
        client.greeting = 1;
    }

    //HELP Command -- INDEPENDENT
    else if(commandArguements[0].compare("HELP") == 0){
        client.serverReply = helpCommand(commandArguements[1]);
    }

    //QUIT Command -- INDEPENDENT
    else if(commandArguements[0].compare("QUIT") == 0){
        client.serverReply = "221 OK"; //REPLY CODE
        client.quitCheck = 1;
    }

    //Post Greeting Commands
    else if(client.greeting == 1){
        
        //AUTH Mode - Username
        if(client.AUTH_Mode == 1){
            bool userPresent;
            if(client.message.compare("rastapopulous ") == 0){ //Server to Server check
                client.serverReply = "rastapopulous";
                client.rastapopulous = 1;
                client.AUTH_Mode == 3;
            }else{
                client.userName = client.message;
                client.userName.erase(client.userName.size() - 1);
                userPresent = usernameLookup(client.message);
                
                if(userPresent == true){
                    client.serverReply = "334 " + encodeString("password:"); //REPLY CODE
                    client.AUTH_Mode = 2; //Sets to password
                }else{
                    sourceDomain.erase(sourceDomain.size() - 1);
                    bool suffix = domainCheck(client.userName, sourceDomain);
                    if(suffix == false){
                        client.serverReply = "535 5.7.8  Authentication credentials invalid";
                    }else{
                        //New user set here
                        string newPass = genAlphaNumString();
                        client.serverReply = "330 " + encodeString(newPass);
                        newPass = "SNOWY22" + newPass; //Sets the salter
                        newPass = encodeString(newPass);

                        //log new entry
                        ofstream userpass("db/.user_pass", ios_base::app);
                        userpass << client.userName << endl;
                        userpass << newPass << endl;
                        userpass.close();
                    }
                }
            }
        }

        //AUTH Mode - Password
        //Username matches - Not first login
        else if(client.AUTH_Mode == 2){
            string password;
            string decoded;
            bool passwordCheck;
            
            decoded = decodeString(client.message); //Returns decoded password
            decoded.erase(decoded.size() - 1);
            password = "SNOWY22" + decoded; //Prepends the salt to the password
            password = encodeString(password); //encodes with the salt included
            passwordCheck = passwordLookup(password, client);

            if(passwordCheck == true){
                client.serverReply = "235 2.7.0  Authentication Succeeded"; //REPLY CODE
                client.AUTH_Mode = 3; //Authentication mode ended
            }else{
                client.serverReply = "535 5.7.8  Authentication credentials invalid";
            }
        }

        //AUTH Command
        else if(commandArguements[0].compare("AUTH") == 0 && commandArguements[1].compare("PLAIN") == 0 && client.commandSequence == 0){
            //Client gives the AUTH Command, first reply is username, then sets the server to AUTH Mode
            //Client replies with the username, checked above, then the server replies with the password
            string temp = "username:";
            client.AUTH_Mode = 1;
            
            client.serverReply = "334 " + encodeString(temp); //REPLY CODE
            client.commandSequence = 1;
        }else if(commandArguements[0].compare("AUTH") == 0 && commandArguements[1].compare("PLAIN") == 0 && client.commandSequence != 0){
            client.serverReply = "503 Bad Sequence of Commands - AUTH already set"; //REPLY CODE
        }else if(client.commandSequence == 0 && client.AUTH_Mode == 0){
            //Auth has not been set and it is not within auth phase 3
            client.serverReply = "530 5.7.0  Authentication required"; //REPLY CODE
        }

        //Successful Authentication
        else if(client.AUTH_Mode == 3){

            //MAIL FROM: Command to set the email sender
            if(commandArguements[0].compare("MAIL") == 0 && commandArguements[1].compare("FROM:") == 0 && client.commandSequence == 1){
                email resetMail;
                client.mail = resetMail;
                string temp = commandArguements[2];
                temp.erase(remove(temp.begin(), temp.end(), '<'), temp.end());
                temp.erase(remove(temp.begin(), temp.end(), '>'), temp.end());

                //If match
                if(client.userName.compare(temp) == 0){
                    client.mail.MAILFROM = temp;
                    client.commandSequence = 2;
                    client.serverReply = "250 OK - MAIL"; //REPLY CODE
                }else{
                    client.serverReply = "553 Request Not Taken - Name Not Allowed"; //REPLY CODE
                }
            }else if(commandArguements[0].compare("MAIL") == 0 && commandArguements[1].compare("FROM:") == 0 && client.commandSequence > 1){
                client.serverReply = "503 Bad Sequence of Commands - MAIL in Progress"; //REPLY CODE
            }else if(commandArguements[0].compare("MAIL") == 0 && commandArguements[1].compare("FROM:") == 0 && client.commandSequence < 1){
                client.serverReply = "503 Bad Sequence of Commands - AUTH NEEDED"; //REPLY CODE
            }

            //RCPT TO: Command to set the recipient
            else if(commandArguements[0].compare("RCPT") == 0 && commandArguements[1].compare("TO:") == 0 && client.commandSequence == 2){
                string temp = commandArguements[2];
                temp.erase(remove(temp.begin(), temp.end(), '<'), temp.end());
                temp.erase(remove(temp.begin(), temp.end(), '>'), temp.end());

                bool suffix = domainCheck(temp, sourceDomain);
                if(suffix == true){
                    client.mail.RCPTTO = temp;
                    client.commandSequence = 3;
                    //creates a directory for the recipient
                    string recipientDirectory = temp;
                    recipientDirectory = recipientDirectory.substr(0, recipientDirectory.size()-11);                
                    client.mail.filePath = "db/" + recipientDirectory;
                    mkdir(client.mail.filePath.c_str(), 0777);
                    client.serverReply = "250 OK - RCPT TO"; //REPLY CODE
                }else{
                    bool suffixCheck01, suffixCheck02, bool suffixCheck03;

                    if(totalPeers >= 1){
                        suffixCheck01 = domainCheck(temp, peer01.domain);
                    }else if(totalPeers >= 2){
                        suffixCheck02 = domainCheck(temp, peer02.domain);
                    }else if(totalPeers >= 3){
                        suffixCheck03 = domainCheck(temp, peer03.domain);
                    }
                    
                    if(suffixCheck01 == true){
                        //MAIL FROM --> Peer Server
                        client.foreignPeer = peer01;
                        peer01.message = "MAIL FROM: " + client.mail.MAILFROM;
                        bzero(peer01.buffer,4096);
                        strcpy(peer01.buffer, peer01.message.c_str());
                        peer01.n = write(psock01,peer01.buffer,strlen(peer01.buffer));
                        bzero(peer01.buffer,4096);
                        peer01.n = read(psock01,peer01.buffer,4096);
                        logEntry(serverIP,peer01.IP,peer01.message,string(peer01.buffer));

                        //RCPT TO --> Peer Server
                        client.foreignPeer = peer01;
                        peer01.message = "RCPT TO: " + temp;
                        bzero(peer01.buffer,4096);
                        strcpy(peer01.buffer, peer01.message.c_str());
                        peer01.n = write(psock01,peer01.buffer,strlen(peer01.buffer));
                        bzero(peer01.buffer,4096);
                        peer01.n = read(psock01,peer01.buffer,4096);
                        logEntry(serverIP,peer01.IP,peer01.message,string(peer01.buffer));
                        client.serverReply = "250 OK - RCPT TO";

                    
                    }else if(suffixCheck02 == true){
                        //MAIL FROM --> Peer Server
                        client.foreignPeer = peer02;
                        peer02.message = "MAIL FROM: " + client.mail.MAILFROM;
                        bzero(peer02.buffer,4096);
                        strcpy(peer02.buffer, peer02.message.c_str());
                        peer02.n = write(psock02,peer02.buffer,strlen(peer02.buffer));
                        bzero(peer02.buffer,4096);
                        peer02.n = read(psock02,peer02.buffer,4096);
                        logEntry(serverIP,peer02.IP,peer02.message,string(peer02.buffer));

                        //RCPT TO --> Peer Server
                        client.foreignPeer = peer02;
                        peer02.message = "RCPT TO: " + client.mail.RCPTTO;
                        bzero(peer02.buffer,4096);
                        strcpy(peer02.buffer, peer02.message.c_str());
                        peer02.n = write(psock02,peer02.buffer,strlen(peer02.buffer));
                        bzero(peer02.buffer,4096);
                        peer02.n = read(psock02,peer02.buffer,4096);
                        logEntry(serverIP,peer02.IP,peer02.message,string(peer02.buffer));
                        client.serverReply = "250 OK - RCPT TO";

                    }else if(suffixCheck03 == true){
                        //MAIL FROM --> Peer Server
                        client.foreignPeer = peer03;
                        peer03.message = "MAIL FROM: " + client.mail.MAILFROM;
                        bzero(peer03.buffer,4096);
                        strcpy(peer03.buffer, peer03.message.c_str());
                        peer03.n = write(psock03,peer03.buffer,strlen(peer03.buffer));
                        bzero(peer03.buffer,4096);
                        peer03.n = read(psock03,peer03.buffer,4096);
                        logEntry(serverIP,peer03.IP,peer03.message,string(peer03.buffer));

                        //RCPT TO --> Peer Server
                        client.foreignPeer = peer03;
                        peer03.message = "MAIL FROM: " + client.mail.MAILFROM;
                        bzero(peer03.buffer,4096);
                        strcpy(peer03.buffer, peer03.message.c_str());
                        peer03.n = write(psock03,peer03.buffer,strlen(peer03.buffer));
                        bzero(peer03.buffer,4096);
                        peer03.n = read(psock03,peer03.buffer,4096);
                        logEntry(serverIP,peer03.IP,peer03.message,string(peer03.buffer));
                        client.serverReply = "250 OK - RCPT TO";
                    
                    }else{
                        client.serverReply = "553 Request Not Taken - Name Not Allowed"; //REPLY CODE
                    }

                }
            }else if(commandArguements[0].compare("RCPT") == 0 && commandArguements[1].compare("TO:") == 0 && client.commandSequence < 2){
                client.serverReply = "503 Bad Sequence of Commands - MAIL Before RCPT"; //REPLY CODE
            }else if(commandArguements[0].compare("MAIL") == 0 && commandArguements[1].compare("FROM:") == 0 && client.commandSequence < 1){
                client.serverReply = "503 Bad Sequence of Commands - AUTH NEEDED"; //REPLY CODE
            }

            //Data command - Sets the server to DATA Mode
            else if(commandArguements[0].compare("DATA") == 0 && commandCount == 1 && client.commandSequence == 3){
                client.DATA_Mode = 1;
                client.serverReply = "354 Enter Mail Input, End with '.'"; //REPLY CODE           
            }else if(commandArguements[0].compare("DATA") == 0 && client.commandSequence < 2 && commandCount == 1){
                client.serverReply = "503 Bad Sequence of Commands"; //REPLY CODE
            }else if(commandArguements[0].compare("DATA") == 0  && commandCount > 1){
                client.serverReply = "501 Syntax Error in Parameters - Only DATA Parameter Allowed"; //REPLY CODE
            }

            //Unrecognized Command
            else{
                client.serverReply = "500 Syntax Error - Command Unrecognized"; //REPLY CODE
            }
        }
    }

    //Bad Helo Command -- INDEPENDENT
    else{
        client.serverReply = "503 Bad Sequence of Commands - No HELO"; //REPLY CODE
    }

}

bool domainCheck(string address, string ending) {
    if(0 == address.compare(address.length() - ending.length(), ending.length(), ending)){
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

            "      > AUTH PLAIN \n"
            "           - Begins user authentication with the server \n"
            "           - prompts the user first for a username \n"
            "           - Generates new password if first login \n"
            "           - prompts user for password to validate credentials \n"

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

string genAlphaNumString(){
    int passSize = 6;
    srand(time(0));
    //characters used in password generation
    static const char characters[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789";
    int charListSize = sizeof(characters) - 1;
    string password;

    //Takes a random character from list and appends it 6 times to form the user password
    for(int i = 0; i < passSize; i++){
        password += characters[rand() % charListSize]; 
    }
    return password;
}

void storePassword(string password){
    string storedPass = salter + password;
}

string encodeString(string storePassword){
    unsigned char* pass = (unsigned char*) storePassword.c_str();
    string encodedString;    
    encodedString = g_base64_encode(pass,strlen((char*)pass));
    return encodedString;
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

bool usernameLookup(string username){
    string* userList;
    username.erase(username.size() - 1);
    string filePath = "db/.user_pass";
    int numLines = 0;
    userList = fileLoader(filePath, numLines);
    for (int i = 0; i < numLines; i + 2){
        if(userList[i].compare(username) == 0){
            return true;
        }
    }
    return false;
}

bool passwordLookup(string password, clientInstance& client){
    string* userList;
    string filePath = "db/.user_pass";
    int numLines = 0;
    userList = fileLoader(filePath, numLines);
    for (int i = 0; i < numLines; i++){   
        if(userList[i].compare(client.userName) == 0){
            i++;
            if(userList[i].compare(password) == 0){
                return true;
            }else{
                return false;
            }
        }
    }
    return false;
}

string* fileLoader(string file, int& num){
    vector<string> lines;
    string line;
    ifstream input_file(file);
    if (!input_file.is_open()) {
        cerr << "Could not open the file - '" << file << "'" << endl;
        exit(1);
    }
    while (getline(input_file, line)){
        lines.push_back(line);
    }
    static string lineData[1000];
    num = 0;
    for (auto it : lines) {
        lineData[num] = it;
        num++; 
    }
    input_file.close();
    return lineData;
}

void logEntry(string from, string to, string command, string replyCode){
    string entry = currentTimeStamp();
    command.erase(command.size() - 1);

    if(command.compare("HELP") == 0){
        replyCode = "214 Help Menu";
    }
    entry = "{" + entry + "} (" + from + ") (" + to + ") [" + command + "] [" + replyCode + "]\n";
    cout << entry << endl;
    ofstream log("db/.server_log", ios_base::app);
    log << entry + "\n";
    log.close();
}

void readConfigFile(){
    vector<string> lines;
    string line;
    ifstream input_file("server.conf");
    if (!input_file.is_open()) {
        cerr << "Could not open the file - " << endl;
        exit(1);
    }
    while (getline(input_file, line)){
        lines.push_back(line);
    }
    
    int i = 0;

    for (auto it : lines) {
        lineData[i] = it;
        i++; 
    }
    configLineCount = i;
    input_file.close();
}

string retrieveHostName(string message){
    string word;
    string Arguements[6];
    istringstream ss(message);
    int i = 0;
    while(ss >> word){
        Arguements[i] = word;
        i++;
    }
    return Arguements[1];
}

void* loadPeers(void*){
    //Peer server setup
        //10 Second delay to allow other servers to boot up
        int count = 10;
        while(count >= 1){
            usleep(1000000);
            count--;
        }

        //Sets the peer info based on number of peers -- SUPPORT FOR UP TO THREE PEERS
        switch(totalPeers){
            case 3:
                peer03.domain = lineData[13];
                peer03.domain.erase(peer03.domain.size() - 1);
                peer03.IP = lineData[14].substr(3);
                peer03.IP.erase(peer03.IP.size() - 1);
                peer03.portno = stoi(lineData[15].substr(5));

                //Socket Creation / Connection
                psock03 = socket(AF_INET, SOCK_STREAM, 0);
                if (psock03 < 0) 
                    error("ERROR opening peer socket");
                pserver03 = gethostbyname(peer03.IP.c_str());
                bzero((char *) &pserver03_addr, sizeof(pserver03_addr));
                pserver03_addr.sin_family = AF_INET;
                bcopy((char *)pserver03->h_addr, (char *)&pserver03_addr.sin_addr.s_addr,pserver03->h_length);
                pserver03_addr.sin_port = htons(peer03.portno);
                if (connect(psock03,(struct sockaddr *) &pserver03_addr,sizeof(pserver03_addr)) < 0){
                    error("ERROR connecting");
                }
                peer03.n = read(psock03,peer03.buffer,4096);
                peer03.message = string(peer03.buffer);
                logEntry(peer03.IP,serverIP, "Initial ",peer03.message);

            case 2:
                peer02.domain = lineData[9];
                peer02.domain.erase(peer02.domain.size() - 1);
                peer02.IP = lineData[10].substr(3);
                peer02.IP.erase(peer02.IP.size() - 1);
                peer02.portno = stoi(lineData[11].substr(5));
                
                //Socket Creation / Connection
                psock02 = socket(AF_INET, SOCK_STREAM, 0);
                if (psock02 < 0) 
                    error("ERROR opening peer socket");
                pserver02 = gethostbyname(peer02.IP.c_str());
                bzero((char *) &pserver02_addr, sizeof(pserver02_addr));
                pserver02_addr.sin_family = AF_INET;
                bcopy((char *)pserver02->h_addr, (char *)&pserver02_addr.sin_addr.s_addr,pserver02->h_length);
                pserver02_addr.sin_port = htons(peer02.portno);
                if (connect(psock02,(struct sockaddr *) &pserver02_addr,sizeof(pserver02_addr)) < 0){
                    error("ERROR connecting");
                }
                peer02.n = read(psock02,peer02.buffer,4096);
                peer02.message = string(peer02.buffer);
                logEntry(peer02.IP,serverIP, "Initial ",peer02.message);
        
            case 1:
                peer01.domain = lineData[5];
                peer01.domain.erase(peer01.domain.size() - 1);
                peer01.IP = lineData[6].substr(3);
                peer01.IP.erase(peer01.IP.size() - 1);
                peer01.portno = stoi(lineData[7].substr(5));

                //Socket Creation / Connection
                psock01 = socket(AF_INET, SOCK_STREAM, 0);
                if (psock01 < 0) 
                    error("ERROR opening peer socket");
                pserver01 = gethostbyname(peer01.IP.c_str());    
                bzero((char *) &pserver01_addr, sizeof(pserver01_addr));
                pserver01_addr.sin_family = AF_INET;
                bcopy((char *)pserver01->h_addr, (char *)&pserver01_addr.sin_addr.s_addr,pserver01->h_length);
                pserver01_addr.sin_port = htons(peer01.portno);
                if (connect(psock01,(struct sockaddr *) &pserver01_addr,sizeof(pserver01_addr)) < 0){
                    error("ERROR connecting");
                }
                peer01.n = read(psock01,peer01.buffer,4096);
                peer01.message = string(peer01.buffer);
                logEntry(peer01.IP,serverIP, "Initial ",peer01.message);    
        }
        if(totalPeers >= 1){
            peer01.name = retrieveHostName(peer01.message);
        }
        if(totalPeers >= 2){
            peer02.name = retrieveHostName(peer02.message);
        }
        if(totalPeers >= 3){
            peer03.name = retrieveHostName(peer03.message);
        }

        if(totalPeers >= 1){
            bzero(peer01.buffer,4096);
            peer01.message = "HELO " + peer01.name + " ";
            strcpy(peer01.buffer, peer01.message.c_str());
            peer01.n = write(psock01,peer01.buffer,strlen(peer01.buffer));
            bzero(peer01.buffer,4096);
            peer01.n = read(psock01,peer01.buffer,4096);
            logEntry(peer01.IP,serverIP,peer01.message,string(peer01.buffer));

            bzero(peer01.buffer,4096);
            peer01.message = "AUTH PLAIN ";
            strcpy(peer01.buffer, peer01.message.c_str());
            peer01.n = write(psock01,peer01.buffer,strlen(peer01.buffer));
            bzero(peer01.buffer,4096);
            peer01.n = read(psock01,peer01.buffer,4096);
            logEntry(peer01.IP,serverIP,peer01.message,string(peer01.buffer));

            bzero(peer01.buffer,4096);
            peer01.message = "rastapopulous ";
            strcpy(peer01.buffer, peer01.message.c_str());
            peer01.n = write(psock01,peer01.buffer,strlen(peer01.buffer));
            bzero(peer01.buffer,4096);
            peer01.n = read(psock01,peer01.buffer,4096);
            logEntry(peer01.IP,serverIP,peer01.message,string(peer01.buffer));
        }

        if(totalPeers >= 2){
            bzero(peer02.buffer,4096);
            peer02.message = "HELO " + peer02.name + " ";
            strcpy(peer02.buffer, peer02.message.c_str());
            peer02.n = write(psock02,peer02.buffer,strlen(peer02.buffer));
            bzero(peer02.buffer,4096);
            peer02.n = read(psock02,peer02.buffer,4096);
            logEntry(peer02.IP,serverIP,peer02.message,string(peer02.buffer));

            bzero(peer02.buffer,4096);
            peer02.message = "AUTH PLAIN ";
            strcpy(peer02.buffer, peer02.message.c_str());
            peer02.n = write(psock02,peer02.buffer,strlen(peer02.buffer));
            bzero(peer02.buffer,4096);
            peer02.n = read(psock02,peer02.buffer,4096);
            logEntry(peer02.IP,serverIP,peer02.message,string(peer02.buffer));

            bzero(peer02.buffer,4096);
            peer02.message = "rastapopulous ";
            strcpy(peer02.buffer, peer02.message.c_str());
            peer02.n = write(psock02,peer02.buffer,strlen(peer02.buffer));
            bzero(peer02.buffer,4096);
            peer02.n = read(psock02,peer02.buffer,4096);
            logEntry(peer02.IP,serverIP,peer02.message,string(peer02.buffer));
        }
        if(totalPeers >= 3){
            bzero(peer03.buffer,4096);
            peer03.message = "HELO " + peer03.name + " ";
            strcpy(peer03.buffer, peer03.message.c_str());
            peer03.n = write(psock03,peer03.buffer,strlen(peer03.buffer));
            bzero(peer03.buffer,4096);
            peer03.n = read(psock03,peer03.buffer,4096);
            logEntry(peer03.IP,serverIP,peer03.message,string(peer03.buffer));

            bzero(peer03.buffer,4096);
            peer03.message = "AUTH PLAIN ";
            strcpy(peer03.buffer, peer03.message.c_str());
            peer03.n = write(psock03,peer03.buffer,strlen(peer03.buffer));
            bzero(peer03.buffer,4096);
            peer03.n = read(psock03,peer03.buffer,4096);
            logEntry(peer03.IP,serverIP,peer03.message,string(peer03.buffer));

            bzero(peer03.buffer,4096);
            peer03.message = "rastapopulous ";
            strcpy(peer03.buffer, peer03.message.c_str());
            peer03.n = write(psock03,peer03.buffer,strlen(peer03.buffer));
            bzero(peer03.buffer,4096);
            peer03.n = read(psock03,peer03.buffer,4096);
            logEntry(peer03.IP,serverIP,peer03.message,string(peer03.buffer));
        }
        return NULL;

}

