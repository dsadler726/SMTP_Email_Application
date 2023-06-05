import socket
import string
import os
import sys
import instances


def create_header(client, num) -> string:
    header = "\nServer: <" + HOST + ">" \
             "\nBatch Count: " + num + "/" + client.get_count + \
             "\nContent-Type: text/plain \n"
    return header


def file_count(p, List):
    counter = 0
    for path in os.listdir(p):
        if os.path.isfile(os.path.join(p, path)):
            List.append(path)
            counter += 1
    return counter


def file_load(List, client):
    load_files = {}
    for i in List:
        path = client.get_path + "/" + i
        file_open = open(path, "r")
        load_files[i] = create_header(client, i) + file_open.read()
    return load_files


def get_email_num(user):
    tot_emails = 0
    user_path = "db/" + user + "/"
    try:
        for path in os.listdir(user_path):
            if os.path.isfile(os.path.join(user_path, path)):
                tot_emails += 1
    except:
        tot_emails = -1

    return tot_emails


def http_driver(client):
    while True: # Main Loop, exits when done with interaction

        # Pre-HTTP Configuration / Receives username
        # Won't exit until present username is given
        while True:
            client.buffer = client.clientSocket.recv(2048).decode()
            user_name = client.buffer

            # Checks if the username is present
            num_emails = get_email_num(user_name)
            if num_emails > 0:
                client.clientSocket.send(str(num_emails).encode())
                break
            else:
                client.clientSocket.send("No Emails Try Another".encode())

        while True: # HTTP Interaction
            bad_request = 0
            # GET Reception
            client.buffer = client.clientSocket.recv(2048).decode()
            get_message = client.buffer

            # Parse GET Response
            client.get_request = get_message.split()
            client.get_path = client.get_request[1]  # /db/username/
            client.get_host = client.get_request[4]  # <server-host-name>
            client.get_count = client.get_request[6]  # 1

            # GET Acknowledgement
            if client.get_request[0] == "GET":
                pass
            else:
                client.serverResponse = protocol + " 400 Bad Request"
                bad_request = 1
                break

            # Host Acknowledgement
            client.get_host.split()
            host_name = "<" + HOST + ">"
            if client.get_host == host_name:
                pass
            else:
                client.serverResponse = protocol + " 400 Bad Request"
                break

            # File Count Acknowledgement
            client.get_count.split()
            path = client.get_request[1]
            email_list = []
            total_file = file_count(path, email_list)
            if int(client.get_count) > total_file:
                client.serverResponse = protocol + " 400 Bad Request"
                break

            # GET Request Valid
            while True:
                client.serverResponse = protocol + " 200 OK"
                email_batch = file_load(email_list, client)
                client.clientSocket.send(client.serverResponse.encode())  # REPLY CODE
                client.buffer = client.clientSocket.recv(2048).decode()
                if client.buffer == "200 OK":
                    # Sending to client
                    batch_file = "\nEmail Batch List: \n"
                    for i in email_batch:
                        batch_file += "\n------------------------------\n" + email_batch[i] + "\n"
                    client.clientSocket.send(batch_file.encode())
                    for i in email_list:
                        os.remove("db/" + user_name + "/" + i)
                break

            if bad_request == 1:
                client.clientSocket.send(client.serverResponse.encode()) # REPLY CODE
            break
        client.clientSocket.close()
        exit(1)


if __name__ == '__main__':
    protocol = "HTTP/1.1"
    HOST = socket.gethostname()

    hostSocket = socket.socket()

    port = int(sys.argv[1])
    HOST_IP = socket.gethostbyname(HOST)
    hostSocket.bind((HOST_IP, port))

    # put the socket into listening mode
    hostSocket.listen(5)

    while True:
        # Establish connection with client.
        cs, addr = hostSocket.accept()
        pid = os.fork()

        if(pid == 0):
            client = instances.ClientInstance(cs, addr)
            http_driver(client)