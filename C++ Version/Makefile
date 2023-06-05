#MakeFile

all: 
	g++ -std=c++17 `pkg-config --cflags glib-2.0` SMTP_Client.cpp -o SMTP_Client `pkg-config --libs glib-2.0`
	g++ -std=c++17 `pkg-config --cflags glib-2.0` SMTP_Server.cpp -o SMTP `pkg-config --libs glib-2.0`
	c++ -std=c++1z Server_Driver.cpp -o server
	c++ -std=c++1z Client_Driver.cpp -o client
	c++ -std=c++1z HTTP_Client.cpp -o HTTP_Client
	c++ -std=c++1z HTTP_Server.cpp -o HTTP

	

