import base64


class Email:
    MAIL_FROM = " "
    RCPT_TO = " "
    DATA = " "

    header = " "
    subject = " "
    body = " "

    filePath = " "


class ClientInstance:
    def __init__(self, clientSocket, clientAddress):
        self.clientSocket = clientSocket
        self.clientAddress = clientAddress

    buffer = "None"

    # SMTP Attributes
    command = " "
    greeting = 0
    commandSequence = 0
    auth_check = 0
    DATA_Mode = 0
    quitCheck = 0
    clientEmail = Email()
    resetEmail = Email()


    # HTTP Attributes
    serverResponse = " "
    get_count = 0
    badRequest = 0
    get_request = " "
    get_host = " "
    get_path = " "


class server_instance:
    def __init__(self, server_socket, server_address):
        self.server_socket = server_socket
        self.server_address = server_address

