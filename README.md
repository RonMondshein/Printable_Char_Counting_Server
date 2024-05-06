# Printable Characters Counting (PCC) Server

This repository contains a C-based client-server application for counting the number of printable characters in a stream of bytes sent by the client. The server tracks overall statistics on printable characters received from all clients and prints them upon termination.

## Overview

The PCC Server assignment comprises two programs:

- **Server (`pcc_server`):** Accepts TCP connections from clients. Upon connection, clients send a byte stream, and the server counts printable characters (bytes with values between 32 and 126) in it. After the stream ends, the server sends the count back to the client over the same connection. Additionally, it maintains a data structure tracking the occurrence of each printable character across all connections. Upon receiving a SIGINT signal, the server prints these counts and exits.
  
- **Client (`pcc_client`):** Establishes a TCP connection to the server and transmits a user-supplied file's contents. It then receives and prints the count of printable characters from the server before exiting.
