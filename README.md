# Sliding-Window-ARQ-with-Congestion-Control
Implementing a Slow-Start congestion control algorithm over the sliding window ARQ protocol over UDP


Makefile is provided to run the server and client scripts.

USAGE/RUN commands are at the top of each scripts.

For Server, after make, simply type on terminal : udpserver <port_for_server> <dropProbability>

For Client, after make, simply type on terminal : udpclient <host_ip_server> <port_for_server>

For Client, it will further ask a filename to transfer
