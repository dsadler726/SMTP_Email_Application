import socket
import os
import sys
from datetime import datetime


if __name__ == '__main__':
    socket = socket.socket()
    server_IP = sys.argv[1]
    server_PORT = sys.argv[2]
    server_reply = "No Response from server"

    socket.connect((server_IP, int(server_PORT)))

    while True:
        while True:
            print("Enter username")
            user_name = input(">> ")
            socket.send(user_name.encode())

            server_reply = socket.recv(2048).decode()
            if server_reply != "No Emails Try Another":
                print("\nNumber of emails for this email: " + server_reply)
                break
            print(server_reply + "\n")

        # Download section
        user_count = input("print the number of emails to download: ")
        host_name = input("\nPlease enter server hostname: ")

        # Create GET Request
        path_name = "db/" + user_name + "/"
        GET_message = "GET " + path_name + " HTTP/1.1\n" \
                      "Host: <" + host_name + ">\n" \
                      "Count: " + user_count

        print("\nGenerated GET request: \n" + GET_message)

        confirm = input("\nConfirmation to proceed? Y/N: ")
        if confirm == "Y" or confirm == "Yes":
            print("\nSending GET Request\n")
            socket.send(GET_message.encode())

            # Receive the server response
            server_reply = str(socket.recv(2048).decode())
            print(server_reply + "\n")

            if server_reply == "HTTP/1.1 200 OK":
                print("Gets HEREEEEE")
                socket.send("200 OK".encode())
                db = user_name + "_mailbox"
                d = os.path.dirname(__file__)  # directory of script
                p = r'/' + db.format(d)  # path to be created

                dir_exist = os.path.exists(p)
                if dir_exist == False:
                    os.mkdir(db)
                stamp = datetime.today().strftime('%m-%d_%H-%M-%S')
                file_name = user_name + "_" + stamp + ".email"
                path = db + "/" + file_name
                f = open(path, "w")
                email = socket.recv(2048).decode()
                f.write(email)
                f.close()

                print("Emails Downloaded Successfully, now exiting")
            break

    exit(1)





