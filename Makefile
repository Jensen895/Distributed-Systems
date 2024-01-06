# Compiler
CC = g++

# Compiler flags
CFLAGS = -std=c++11 -Wall -pthread -ldl

# Executable names
CLIENT_EXEC = client
SERVER_EXEC = server

# Shell scripts
CHMOD_SCRIPTS = ssh_all.sh start_servers.sh stop_servers.sh mp2_demo.sh mp3_multiread.sh

# Form complete paths to source files
SRC_DIR = ./src

# Source file names
CLIENT_SRC = client.cpp RequestMessage.cpp ResponseMessage.cpp unit_test.cpp MembershipRequest.cpp Message.cpp Messagefunction.cpp Membershiplist.cpp Member.cpp MemberIdRequest.cpp MemberIdResponse.cpp GossipMessage.cpp SDFSRequest.cpp SDFS_filelist.cpp SDFS_file_transfer.cpp MJRequest.cpp
SERVER_SRC = server.cpp RequestMessage.cpp ResponseMessage.cpp unit_test.cpp MembershipRequest.cpp Message.cpp Messagefunction.cpp Membershiplist.cpp Member.cpp MemberIdRequest.cpp MemberIdResponse.cpp GossipMessage.cpp SDFSRequest.cpp SDFS_filelist.cpp SDFS_file_transfer.cpp MJRequest.cpp

# Object file names (generated during compilation)
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)
SERVER_OBJ = $(SERVER_SRC:.cpp=.o)

all: $(CLIENT_EXEC) $(SERVER_EXEC)

$(CLIENT_EXEC): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER_EXEC): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

# Rule to check and chmod scripts only if they are not already executable
.PHONY: chmod_scripts
chmod_scripts:
	@for script in $(CHMOD_SCRIPTS); do \
		if [ ! -x "$$script" ]; then \
			chmod +x "$$script"; \
		fi; \
	done

clean:
	rm -f $(CLIENT_EXEC) $(SERVER_EXEC) $(CLIENT_OBJ) $(SERVER_OBJ)

# Add 'chmod_scripts' as a dependency for 'all'
all: chmod_scripts

.PHONY: all clean chmod_scripts
