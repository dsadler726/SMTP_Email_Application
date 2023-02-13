/*
    Author: Dakota Sadler
    Date: June 15th, 2022
    Client_Driver.cpp
*/

#include <stdio.h>  
#include <string.h>
#include <unistd.h>
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h> 
#include <errno.h>
#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <stdlib.h>
#include <iostream>
#include <filesystem>
#include <dirent.h>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <ctime>

using namespace std;

//Function Prototypes
string* confloader(string file);
void error(const char *msg);

int main(int argc, char *argv[]){
    string SERVER_IP, SERVER_PORT;
    int pid;

    if (argc < 2){ error("No file Given"); }
    string fileName = string(argv[1]);
    string* lineData;
    lineData = confloader(fileName); //Reads in config file

    //Sets approriate port # based on config file setup
    SERVER_IP = lineData[1].substr(10);
    SERVER_PORT = lineData[2].substr(12);
    SERVER_IP.erase(SERVER_IP.size() - 1);

    string type;
    int selectionMenu = 0;
    cout << "\nWelcome to the Simple Email Client Application" << endl;
    while(1){
        cout << "\nPlease Select the type of client you would like to run" << endl;
        cout << "\n1.) SMTP Sender Client \n2.) HTTP Reciever Client" << endl;
        cout << "Choice: ";
        cin >> type;
        
        if(type.compare("1") == 0){
            string SMTP_Start = "./SMTP_Client " + SERVER_IP + " " + SERVER_PORT;
            system(SMTP_Start.c_str());
            exit(1);
        }else if(type.compare("2") == 0){
            string HTTP_Start = "./HTTP_Client " + SERVER_IP + " " + SERVER_PORT;
            system(HTTP_Start.c_str());
            exit(2);
        }else{
            cout << "Invalid, please try again" << endl;
        }
    }
}

string* confloader(string file){
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
    static string lineData[4];
    int i = 0;

    for (auto it : lines) {
        lineData[i] = it;
        i++; 
    }
    input_file.close();
    return lineData;
}

void error(const char *msg){
    perror(msg);
    exit(1);
}