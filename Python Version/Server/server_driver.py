import sys
import os

if __name__ == '__main__':
    # server server.conf
    print("\nWelcome to the SMTP / HTTP Email Application Server!"
          "\nPlease wait while the servers are booted up...")
    conf_file = open(sys.argv[1], "r")

    with open(sys.argv[1]) as f:
        lines = [line for line in f]

    smtp_server_port = lines[1][10:-1]
    http_server_port = lines[2][10:]

    pid = os.fork()

    if(pid == 0):
        print("\n[Server] Now Starting SMTP Server\n")
        smtp = 'python3 smtp_server.py ' + smtp_server_port
        os.system(smtp)

    print("\n[Server] Now Starting HTTP Server\n")
    http = 'python3 http_server.py ' + http_server_port
    os.system(http)

