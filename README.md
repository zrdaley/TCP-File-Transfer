# TCP-File-Transfer

This repository contains a TCP client and server.

## Client

The TCP client takes an IP number, port number and a zip filename as input. It sends the zip file to the server program and receives the unzipped file that is returned. 

## Server

The TCP server takes an IP number and port number as input. It receives the zip file from the client, unzips it, and returns it's contents.

## Run

1) `cd server`
2) `gcc -o tfs tfs.c`
3) `./tfs <IP> <Port>`

4) `cd ../client`
5) `gcc -o tfc tfc.c`
6) `./tfc <IP> <Port> <Zip Filename>`
