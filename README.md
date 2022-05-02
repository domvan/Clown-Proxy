# Clown-Proxy

This program is a basic HTTP proxy that handles HTTP GET requests. The proxy is designed to parse through these requests and replace all JPG images with an image of a clown. The proxy also replaces all occurrences of the word “Happy” with “Silly”.
To use the proxy...
1) Manually configure an HTTP web proxy through your system settings.
Set the IP to 127.0.0.1 (local host) and the port to 67892)

2) To compile the program, use the command... $ g++ -Wall ClownProxy.cpp
in the command line.

3)To run the proxy, use the command... $ ./a.out
in the command line.

The proxy will now be running on your local host on port 6789. Ctrl-c can be used at any time on the command line to terminate the program.

Note: To change the port number, open the ClownProxy.cpp file and change the value of PROXYPORT on line 20 to the desired port number. Ensure this value matches the port number on the proxy configured in the system settings.
