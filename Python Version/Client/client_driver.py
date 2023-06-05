import sys
import os


if __name__ == '__main__':
      conf_file = open(sys.argv[1], "r")
      with open(sys.argv[1]) as f:
            lines = [line for line in f]

      server_IP = lines[1][10:-1]
      server_PORT = lines[2][12:]

      while True:
            print("Select the number of the server you want to connect to"
                  "\n1.) SMTP Server"
                  "\n2.) HTTP Server\n")

            choice = input(">> ")
            if int(choice) == 1:
                  print("\nNow booting SMTP Client Application")
                  smtp = 'python3 smtp_client.py ' + server_IP + " " + server_PORT
                  os.system(smtp)
                  exit(1)
            elif int(choice) == 2:
                  print("\nNow booting HTTP Client Application")
                  http = 'python3 http_client.py ' + server_IP + " " + server_PORT
                  print(http)
                  os.system(http)
                  exit(1)
            else:
                  print("\nImproper Selection, Please Try Again...")

