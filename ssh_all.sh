#!/bin/bash

# Function to print usage instructions
print_usage() {
    echo "Usage: $0 <NETID> <command>"
    exit 1
}

# Check for the correct number of arguments
if [ $# -ne 2 ]; then
    print_usage
fi


# Define the constant values
ILLINI_ACCOUNT="$1"
GROUP_NUMBER="41"
# Assign the command-line argument to a variable
COMMAND="$2"

for CLUSTER_NUMBER in $(seq -w 1 10)
do
    echo "Executing command for ${CLUSTER_NUMBER} server"
    ssh ${ILLINI_ACCOUNT}@fa23-cs425-${GROUP_NUMBER}${CLUSTER_NUMBER}.cs.illinois.edu "cd mp1; nohup ${COMMAND} < /dev/null &" &
done