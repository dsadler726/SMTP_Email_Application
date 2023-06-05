# Import socket module
import socket
import sys
import base64

if __name__ == '__main__':
    # Create a socket object
    socket = socket.socket()

    # Define the port on which you want to connect
    server_IP = sys.argv[1]
    server_PORT = sys.argv[2]

    # connect to the server on local computer
    socket.connect((server_IP, int(server_PORT)))


    while True:
        server_message = socket.recv(2048).decode()
        print(server_message)

        # Data-Mode
        if server_message.startswith('354') == True:
            data_line = " "
            while data_line != ".":
                data_line = input(">>>  ")
                socket.send(data_line.encode())
                if data_line == ".":
                    server_message = socket.recv(2048).decode()
                    print(server_message)

        # New User-Password
        elif server_message.startswith('330') == True:
            decoded_pass = server_message[4:]
            base64_bytes = decoded_pass.encode("ascii")
            decoded_base64 = base64.b64decode(base64_bytes)
            decoded_pass = decoded_base64.decode("ascii")
            print("New User Registered with passcode: " + decoded_pass)
            print("Initiate AUTH command with the new login")

        # Encodes the Password
        if server_message.startswith('334 Password'):
            password = input(">>  ")
            pass_byte = password.encode("ascii")
            base64_pass_byte = base64.b64encode(pass_byte)
            base64_pass = base64_string = base64_pass_byte.decode("ascii")
            socket.send(base64_pass.encode())

        # Prime Message Outbound
        else:
            message = input(">>  ")  # Prime Outbound
            socket.send(message.encode())

    # close the connection
    socket.close()