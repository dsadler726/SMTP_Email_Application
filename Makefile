#Dakota Sadler MakeFile
#Project 01

all: 
	c++ -std=c++1z Server_Driver.cpp -o App/server
	c++ -std=c++1z Client_Driver.cpp -o App/client

	c++ -std=c++1z SMTP_Client.cpp -o App/SMTP_Client
	c++ -std=c++1z SMTP_Server.cpp -o App/SMTP

	c++ -std=c++1z HTTP_Client.cpp -o App/HTTP_Client
	c++ -std=c++1z HTTP_Server.cpp -o App/HTTP

