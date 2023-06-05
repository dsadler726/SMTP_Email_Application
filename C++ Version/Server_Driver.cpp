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

string* confloader(string file);
void error(const char *msg);

int main(int argc, char *argv[]){
    string SMTP_portno, HTTP_portno;
    int pid;

    if (argc < 2){
        fprintf(stderr,"ERROR, no file provided\n");
        exit(1);
    }

    string fileName = string(argv[1]);
    string* lineData;
    lineData = confloader(fileName); //Reads in config file

    //Sets approriate port # based on config file setup in specifications
    SMTP_portno = lineData[2].substr(10);
    HTTP_portno = lineData[3].substr(10);

    //Welcome Message
    cout << "\nWelcome to the Simple Email Server Application \nBooting up servers..." << endl;

    string SMTP_Start = "./SMTP " + SMTP_portno;
    string HTTP_Start = "./HTTP " + HTTP_portno;

    pid = fork();
    if (pid < 0)
        error("ERROR on fork");
    if (pid == 0)  {
        system(SMTP_Start.c_str());
    }else{
        system(HTTP_Start.c_str());
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
    static string lineData[100];
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