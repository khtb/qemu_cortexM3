#!/bin/bash
HOST_PORT=12345

echo "Waiting for traffic on localhost:$HOST_PORT..."
# Using netcat to connect and print output. 
# Loop to retry connection if it fails or disconnects.
while true; do
    nc -v localhost $HOST_PORT
    echo "Disconnected. Retrying in 1 second..."
    sleep 1
done
