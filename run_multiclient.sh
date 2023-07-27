#!/bin/bash

# Replace SERVER_ADDRESS and SERVER_PORT with your server's address and port
SERVER_PORT="88888"

NUM_CLIENTS=500

# Function to launch a client in the background
launch_client() {
    ./build/clone-client "$SERVER_PORT" &
}

# Loop to create and run 1000 clients
for ((i=1; i<=NUM_CLIENTS; i++))
do
    launch_client
    # sleep 0.1  # Adjust this sleep time as needed to avoid overwhelming the system
done

# Wait for all background clients to finish
wait
