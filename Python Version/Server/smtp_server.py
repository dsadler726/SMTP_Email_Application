import string
import sys
import os
import socket
import datetime
import instances
import random
import base64

# Globals
SALT = "Snowy726"


def server_log(reply_code, from_ip, to_ip, command):
    # timestamp from-ip to-ip protocol-command reply-code description
    description = " "
    current_time = str(datetime.datetime.now())
    entry = "\n" + current_time[:-7:] + " | From: " + from_ip + " | To: " + to_ip + " | " + command + " | " + reply_code + " | \n"
    file = open(r'db/.server_log', 'a')
    file.write(entry)
    file.close()
    print(entry)


def help_command(command) -> string:
    message = """214 Help Menu:  
          > HELO <Hostname> \n
               - Sends a greeting to the Server \n
               - Need to greet server in order to begin proper interaction \n
          > MAIL FROM: <User> \n
               - Sends to the server who is sending the email \n
               - HELP MAIL for more information \n
          > RCPT TO: <User> \n
               - Sends to the server who the email is being sent to\n
               - HELP RCPT for more information \n
          > DATA \n
               - Sets the server to DATA Mode and allows the user to enter the email body\n
               - HELP DATA for more information \n
          > MISC. Commands: \n
               - HELP - Returns a list of useful commands to the user \n
               - QUIT - ends client interactions with the server \n \n"""

    if (command == "MAIL"):
        message += """------------------------------------------------------------ \n
          > More Info: MAIL \n
               - Proper Syntax: MAIL FROM: <User> \n
               - Syntax Example: MAIL FROM: username@447m22 \n
               - <User> has to follow the application domain of '@447m22', deviations will be rejected \n
               - '<>' can be added if user wishes, though can proceed without \n
               - Starts the mail creation process, advances the sequence \n
               - Following command in the sequence is RCPT \n"""

    if (command == "RCPT"):
        message += """------------------------------------------------------------ \n
          > More Info: RCPT \n
               - Proper Syntax: RCPT TO: <User> \n
               - Syntax Example: RCPT TO: username@447m22 \n
               - <User> has to follow the application domain of '@447m22', deviations will be rejected \n
               - '<>' can be added if user wishes, though can proceed without \n
               - Proceeds the email interaction, always follows MAIL FROM: \n
               - Following command in the sequence is DATA \n"""

    if (command == "DATA"):
        message += """------------------------------------------------------------ \n"
          > More Info: DATA \n
               - Proper Syntax: DATA \n
               - Syntax Example: DATA \n
               - NO other parameters should be entered, else will reply with an error code \n
               - Sets the interaction to DATA Mode \n
               - Allows the user to enter the body of the email \n
               - ends when the user enters a singular '.' on its own line\n
               - Processes the data and creates an email with the user inputs\n"""
    return message


def read_user_pass(user_name):
    with open(r'db/.user_pass', 'r') as file:
        count = -1
        # read all lines in a list
        lines = file.readlines()
        for line in lines:
            count += 1
            if line.find(user_name) != -1:
                return lines[count+1]
        return "None"


def decode_b64(string):
    base64_bytes = string.encode("ascii")
    decoded_base64 = base64.b64decode(base64_bytes)
    decoded_string = decoded_base64.decode("ascii")
    return decoded_string


def encode_b64(string):
    byte_string = string.encode("ascii")
    base64_bytes = base64.b64encode(byte_string)
    base64_string = base64_bytes.decode("ascii")
    return base64_string


def generate_password():
    S = 6
    P = ''.join(random.choices(string.ascii_uppercase + string.ascii_lowercase + string.digits, k=S))
    return P


def add_user_entry(user, password):
    file = open(r'db/.user_pass', 'a')
    file.write("\n" + user + "\n" + password + "\n")
    file.close()


def auth_command(client):
    auth_reply = "334 Username"
    server_log(auth_reply, HOST_IP, client.clientAddress[0], client.command)
    client.clientSocket.send(auth_reply.encode())
    user_name = client.clientSocket.recv(2048).decode()
    password = read_user_pass(user_name)
    if password != "None":
        auth_reply = "334 Password"
        server_log(auth_reply, HOST_IP, client.clientAddress[0], user_name)
        client.clientSocket.send(auth_reply.encode())
        encoded_pass = client.clientSocket.recv(2048).decode()
        user_pass = decode_b64(encoded_pass)
        salted_user_pass = SALT + user_pass
        user_pass = encode_b64(salted_user_pass)
        if user_pass.strip() == password.strip():
            client.auth_check = 1
            return "235 Authentication Succeeded"
        else:
            return "535 Authentication Credentials Invalid"
    else:
        new_pass = generate_password()
        salted_pass = SALT + new_pass
        encoded_pass = encode_b64(salted_pass)
        add_user_entry(user_name, encoded_pass)
        auth_reply = "330 " + encode_b64(new_pass)
        return auth_reply


def domain_check(domain) -> bool:
    ending = "@server.com"
    domainLength = len(domain)
    endingLength = len(ending)
    if domain[domainLength - endingLength:] == ending:
        return True
    else:
        return False


def command_driver(request, client) -> string:
    server_reply = "None"
    command = request.split(" ")
    command_args = len(command)

    # Pre Greeting Commands - No Greeting Needed #
    # HElO Command
    if command_args == 2 and command[0] == "HELO" and command[1] == HOST:
        server_reply = "250 OK - HELO"  # Reply Code
        client.greeting = 1
    # HELP Command
    elif command[0] == "HELP":
        if command_args == 2:
            server_reply = help_command(command[1])
        else:
            server_reply = help_command(command[0])
    # Bad HELO
    elif client.greeting != 1:
        server_reply = "503 Bad Sequence of Commands - No HELO"  # Reply Code
    # Quit Command
    elif command[0] == "QUIT":
        server_reply = "221 OK"  # Reply Code

    # Post Greeting Commands - Greeting Needed #
    # Auth Command
    elif client.greeting == 1 and client.auth_check == 0:
        if command[0] == "AUTH":
            server_reply = auth_command(client)
        elif command[0] != "AUTH":
            server_reply = "503 Bad Sequence of Commands - AUTH Needed"
    # Post Auth Commands
    elif client.greeting == 1 and client.auth_check == 1:
        # MAIL FROM: Sets the email sender
        if(command[0] == "MAIL" and command [1] == "FROM:" and client.commandSequence == 0):
            client.clientEmail = client.resetEmail
            temp = command[2]
            suffixCheck = domain_check(temp)
            if suffixCheck == True:
                client.clientEmail.MAIL_FROM = temp
                client.commandSequence = 1
                server_reply = "250 OK - MAIL"
            else:
                server_reply = "553 Request Not Taken - Name Not Allowed"  # REPLY CODE
        elif command[0] == "MAIL" and command [1] == "FROM:" and client.commandSequence != 0:
            server_reply = "503 Bad Sequence of Commands - MAIL in Progress"

        # RCPT TO: Command to set the recipient
        elif command[0] == "RCPT" and command[1] == "TO:" and client.commandSequence == 1:
            temp = command[2]
            suffixCheck = domain_check(temp)
            if suffixCheck == True:
                client.clientEmail.RCPT_TO = temp
                client.commandSequence = 2
                # Directory Creation
                temp.split('@')[0]
                d = os.path.dirname(__file__)  # directory of script
                p = r'db/'+temp.split('@')[0].format(d)  # path to be created
                try:
                    os.makedirs(p)
                except OSError:
                    pass
                server_reply = "250 OK - RCPT TO"  # REPLY CODE
            else:
                server_reply = "553 Request Not Taken - Name Not Allowed"  # REPLY CODE
        elif command[0] == "RCPT" and command[1] == "TO:" and client.commandSequence < 1:
            server_reply = "503 Bad Sequence of Commands - MAIL Before RCPT"   # REPLY CODE

        # Data command - Sets the server to DATA Mode
        elif command[0] == "DATA" and client.commandSequence == 2 and command_args == 1:
            client.DATA_Mode = 1
            server_reply = "354 Enter Mail Input, First line is subject header, End with '.'"  # REPLY CODE
        elif command[0] == "DATA" and client.commandSequence < 2 and command_args == 1:
            server_reply = "503 Bad Sequence of Commands"  # REPLY CODE
        elif command[0] == "DATA" and command_args > 1:
            server_reply = "501 Syntax Error in Parameters - Only DATA Parameter Allowed"  # REPLY CODE
        else:
            server_reply = "500 Syntax Error - Command Unrecognized"  # Reply Code

    # Unrecognized Command
    else:
        server_reply = "500 Syntax Error - Command Unrecognized"  # Reply Code

    return server_reply


def trace_record(client) -> string:
    current_time = str(datetime.datetime.now())
    trace_record = "Date: " + current_time  + "\nFROM: <" + client.clientEmail.MAIL_FROM
    trace_record += ">\nTO: <" + client.clientEmail.RCPT_TO + ">\nSubject: " + client.clientEmail.subject + "\n"
    return trace_record


def file_count(recipient) -> int:
    counter = 0
    p = r'db/' + recipient.split('@')[0]
    for path in os.listdir(p):
        if os.path.isfile(os.path.join(p, path)):
            counter += 1
    return counter


def file_create(client):
    count = file_count(client.clientEmail.RCPT_TO)
    count += 1
    file_name = r'db/' + client.clientEmail.RCPT_TO.split('@')[0] + '/' + str(count) + '.email'
    file = open(file_name, "w")
    file.write(client.clientEmail.body)
    file.close()


def data_processing(client):
    header = trace_record(client)
    client.clientEmail.body = header + '\n' + client.clientEmail.DATA
    file_create(client)


def SMTP_Driver(client):
    # Welcome Message
    server_reply = "220 " + HOST + " - Service Ready"
    server_log(server_reply, HOST_IP, client.clientAddress[0], "Welcome Message")
    client.clientSocket.send(server_reply.encode())

    is_subject = 0
    while True:
        client_request = client.clientSocket.recv(2048).decode() # Prime Inbound
        client.command = client_request
        # Non DATA-Mode
        if client.DATA_Mode == 0:
            server_reply = command_driver(client_request, client)
            server_log(server_reply, HOST_IP, client.clientAddress[0], client_request)
            client.clientSocket.send(server_reply.encode())
            if server_reply == "221 OK":
                client.clientSocket.close()
                exit("client closed connection")
        # DATA-Mode
        else:
            if is_subject == 0:
                client.clientEmail.subject = client_request
                is_subject = 1
            else:
                if client_request == ".":
                    client.DATA_Mode = 0    # Ends Data Mode
                    client.clientEmail.DATA += "."
                    data_processing(client)
                    server_reply = "250 OK Data Confirmed"  # Reply Code
                    server_log(server_reply, HOST_IP, client.clientAddress[0], "DATA")
                    client.clientSocket.send(server_reply.encode())
                    client.commandSequence = 0
                    is_subject = 0
                else:
                    client.clientEmail.DATA += '\n' + client_request


def create_db():
    db = "db"
    try:
        os.makedirs(db)
        file_name = r'db/' + '.user_pass'
        file = open(file_name, "w")
        file_name_2 = r'dn/' + '.server_log'
        file = open(file_name_2, "w")
        file.close()
    except OSError:
        pass


if __name__ == '__main__':
    # Socket Creation
    hostSocket = socket.socket()
    HOST = socket.gethostname()

    # Binding from server.conf
    port = int(sys.argv[1])
    HOST_IP = socket.gethostbyname(HOST)
    hostSocket.bind((HOST_IP, port))

    # Database Creation
    create_db()

    # put the socket into listening mode
    hostSocket.listen(5)
    print("SMTP Server Online\n")

    while True:
        # Establish connection with client.
        cs, addr = hostSocket.accept()
        pid = os.fork()

        if(pid == 0):
            client = instances.ClientInstance(cs, addr)
            SMTP_Driver(client)

    # Close the connection with the client
    client.clientSocket.close()
    # Breaking once connection closed
