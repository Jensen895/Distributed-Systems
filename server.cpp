#include "src/RequestMessage.h"
#include "src/ResponseMessage.h"
#include "src/unit_test.h"
#include "src/MembershipRequest.h"
#include "src/MJRequest.h"
#include "src/Message.h"
#include "src/Member.h"
#include "src/Membershiplist.h"
#include "src/MemberIdRequest.h"
#include "src/MemberIdResponse.h"
#include "src/GossipMessage.h"
#include "src/SDFSRequest.h"
#include "src/SDFS_filelist.h"
#include "src/SDFS_file_transfer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <regex>
#include <vector>
#include <chrono>
#include <mutex>
#include <dirent.h> // POSIX directory handling
#include <thread>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <random>

#define DEFAULT_PORT 9999
#define PORT 3491 // the port users will be connecting to
#define PORT_MEMBERSHIP 3481
#define LISTEN_PORT 4940
#define SEND_PORT 5040
#define SDFS_PORT 4481
#define MJ_PORT 5481
#define RECOVER_PORT 5480

#define T_DISSEM 300
#define b 3
#define Replicas 4
#define PutQuorum 3
#define GetQuorum 3

#define TEST_FP false
#define MAXDATASIZE 10000
#define BACKLOG 20

#define MAPLE_INPUT_LINES 30

using namespace std;
volatile bool serverRunning = true;

void sigchld_handler(int s)
{
    (void)s;

    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

struct GrepResult
{
    std::vector<std::string> matchingLines;
    int matchingLinesCount;

    GrepResult() : matchingLinesCount(0) {}
};

struct ServerGossip
{
    std::string hostname;
    std::string address;
    std::string test_message;
    bool error;
    std::string error_message;
};

std::string ConvertHostname2Ip(const std::string &hostname, std::string port)
{
    struct addrinfo hints;
    struct addrinfo *addresult;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM; // Use UDP

    int status = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &addresult);
    if (status != 0)
    {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
        exit(1);
    }

    struct sockaddr_in *ipv4_addr = (struct sockaddr_in *)(addresult->ai_addr);
    char serveraddress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(ipv4_addr->sin_addr), serveraddress, INET_ADDRSTRLEN);

    std::string serverAddress(serveraddress);
    freeaddrinfo(addresult);

    return serverAddress;
}

GrepResult executeGrepCommand(const std::string &pattern, const std::string &filename, const bool &useRegex)
{
    GrepResult result;
    FILE *ptr;
    ptr = fopen(filename.c_str(), "r");
    // std::cout << "pattern: " << pattern << ", useRegex: " << useRegex << ", filename: " << filename << std::endl;
    if (NULL == ptr)
    {
        std::cout << "File cannot be opened: " << filename << std::endl;
    }
    if (useRegex)
    {
        // for lines count
        std::string grepCommand_count = "grep -c -E '" + pattern + "' " + filename;
        FILE *pipe = popen(grepCommand_count.c_str(), "r");
        if (pipe)
        {
            std::string result_count;
            char buffer[128];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            {
                // Capture the matching line
                result_count += buffer;
            }
            result.matchingLinesCount = std::stoi(result_count);
            pclose(pipe);
        }
        else
        {
            std::cerr << "Error executing grep command." << std::endl;
        }

        // for lines
        std::string grepCommand = "grep -E '" + pattern + "' " + filename;
        FILE *pipe2 = popen(grepCommand.c_str(), "r");
        if (pipe2)
        {
            std::string result_count;
            char buffer[128];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            {
                // Capture the matching line
                result.matchingLines.push_back(buffer);
            }
            pclose(pipe2);
        }
        else
        {
            std::cerr << "Error executing grep command." << std::endl;
        }
    }
    else // using std::string search
    {
        char *buffer = NULL;
        size_t buffer_size = 0;
        int total_line_count = 0;
        int match_line_count = 0;
        std::string match_lines;
        while (getline(&buffer, &buffer_size, ptr) >= 0)
        {
            total_line_count += 1;
            std::string str(buffer);
            size_t found = str.find(pattern);
            if (found != std::string::npos)
            {
                result.matchingLines.push_back(str);
                match_line_count += 1;
            }
        }
        result.matchingLinesCount = match_line_count;
        free(buffer);
        buffer = NULL;
        fclose(ptr);
    }

    // std::cout << "result: " << result.matchingLinesCount << std::endl;
    return result;
}

int generate_TCPserversocket(const char *port)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    struct sigaction sa;
    int yes = 1;
    int rv;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0)
    {
        std::cerr << "getaddrinfo: " << gai_strerror(rv) << std::endl;
        exit(1);
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }
        if (::bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        std::cerr << "server: failed to bind" << std::endl;
        exit(1);
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    return sockfd;
}

int generate_TCP_SDFS_serversocket(const char *port)
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(port)); // Set the port number (use htons for network byte order)
    serverAddress.sin_addr.s_addr = INADDR_ANY; // Listen on all available network interfaces

    if (::bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("bind");
        close(serverSocket);
        exit(1);
    }

    if (listen(serverSocket, 5) == -1)
    {
        perror("listen");
        close(serverSocket);
        exit(1);
    }

    return serverSocket;
}

int generate_FAILURE_RECOVERY_serversocket(const char *port)
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(port)); // Set the port number (use htons for network byte order)
    serverAddress.sin_addr.s_addr = INADDR_ANY; // Listen on all available network interfaces

    if (::bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("bind");
        close(serverSocket);
        exit(1);
    }

    if (listen(serverSocket, 5) == -1)
    {
        perror("listen");
        close(serverSocket);
        exit(1);
    }

    return serverSocket;
}

struct ServerResult
{
    string hostname;
    string address;
    ResponseMessage response;
    bool error;
    string error_message;
};

int generate_TCP_client_socket(string serverNames, int port)
{
    struct addrinfo hints;
    struct addrinfo *addresult;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(serverNames.c_str(), NULL, &hints, &addresult);
    if (status != 0)
    {
        cerr << "getaddrinfo error: " << gai_strerror(status) << endl;
        exit(1);
    }
    struct sockaddr_in *ipv4_addr = (struct sockaddr_in *)(addresult->ai_addr);
    char serveraddress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(ipv4_addr->sin_addr), serveraddress, INET_ADDRSTRLEN);

    string serverAddress(serveraddress);
    freeaddrinfo(addresult);
    ServerResult serverResult;
    serverResult.address = serverAddress;
    serverResult.hostname = serverNames;
    std::cout << "create socket in client : " << serverNames << ", turn to the ip:" << serverAddress << endl;
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1)
    {
        cerr << "Error creating client socket for " << serverAddress << ":" << port << endl;
        // serverResult.error_message = "Error creating client socket for " + serverAddress + ":" + to_string(port);
        // serverResult.error = true;
        // exit(1);
    }

    sockaddr_in serverAddressInfo;
    serverAddressInfo.sin_family = AF_INET;
    serverAddressInfo.sin_port = htons(port);
    if (inet_pton(AF_INET, serverAddress.c_str(), &(serverAddressInfo.sin_addr)) <= 0)
    {
        cerr << "Invalid server address: " << serverAddress << endl;
        // serverResult.error_message = "Invalid server address: " + serverAddress;
        // serverResult.error = true;
        close(clientSocket);
        // exit(1);
    }

    if (connect(clientSocket, reinterpret_cast<struct sockaddr *>(&serverAddressInfo), sizeof(serverAddressInfo)) == -1)
    {
        cerr << "Error connecting to " << serverAddress << ":" << port << endl;
        // serverResult.error_message = "Error connecting to " + serverAddress + ":" + to_string(port);
        // serverResult.error = true;
        // results.push_back(serverResult);
        close(clientSocket);
        // exit(1);
    }
    return clientSocket;
}

int generate_UDPserverlistensocket(const char *port)
{
    int recvSocket; // listen socket
    int yes = 1;
    struct addrinfo hints, *servinfo, *p;
    if ((recvSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("listener: socket");
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM; // Use UDP
    hints.ai_flags = AI_PASSIVE;

    int rv = getaddrinfo(NULL, port, &hints, &servinfo);
    if (rv != 0)
    {
        std::cerr << "getaddrinfo (listener): " << gai_strerror(rv) << std::endl;
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((recvSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server socket");
            continue;
        }
        if (setsockopt(recvSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }
        if (::bind(recvSocket, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(recvSocket);
            perror("server: bind");
            continue;
        }
        break;

        break;
    }
    if (p == NULL)
    {
        std::cerr << "server: failed to bind" << std::endl;
        exit(1);
    }
    freeaddrinfo(servinfo);
    return recvSocket;
}

void grep_function()
{
    //=======================find the .log file=====================//
    std::regex pattern("vm[1-9]|vm10\\.log");
    std::string current_directory = "./";
    std::string logFile;
    DIR *dir = opendir(current_directory.c_str());
    if (!dir)
    {
        std::cerr << "Failed to open directory." << std::endl;
        exit(1);
    }
    dirent *entry;
    while ((entry = readdir(dir)))
    {
        if (std::regex_search(entry->d_name, pattern))
        {
            logFile = entry->d_name;
            std::cout << "Match found: " << entry->d_name << std::endl;
        }
    }
    closedir(dir);
    //=================================================================//

    int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
    char s[INET_ADDRSTRLEN];
    socklen_t sin_size;
    struct sockaddr_storage their_addr; // connector's address information
    sockfd = generate_TCPserversocket(to_string(PORT).c_str());
    std::cout << "server: waiting for connections..." << std::endl;

    while (1)
    {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        std::cout << "server: got connection from " << s << std::endl;

        if (!fork())
        {                  // this is the child process
            close(sockfd); // child doesn't need the listener
            char buf[MAXDATASIZE];
            int bytesRead;

            bytesRead = recv(new_fd, buf, MAXDATASIZE - 1, 0);
            if (bytesRead > 0)
            {
                buf[bytesRead] = '\0';
            }

            RequestMessage receivedQuery = RequestMessage::deserialize(buf);
            ResponseMessage responseQuery;
            if (receivedQuery.unit_test)
            {
                UnitTest unittest;
                responseQuery.unittest_result = unittest.generateLogFileAndGrep();
                responseQuery.in_file = "unit_test.log";
            }
            else
            {
                GrepResult grepResult = executeGrepCommand(receivedQuery.pattern, logFile, receivedQuery.useRegex);
                responseQuery.in_file = logFile;
                responseQuery.matching_line_count = grepResult.matchingLinesCount;
            }

            if (send(new_fd, responseQuery.serialize().c_str(), responseQuery.serialize().size(), 0) == -1)
                perror("send");

            close(new_fd);
            exit(0);
        }
        close(new_fd); // parent doesn't need this
    }
}

void handleMembershipList(int new_fd, int port)
{
    char buf[MAXDATASIZE];
    int bytesRead;
    bytesRead = recv(new_fd, buf, MAXDATASIZE - 1, 0);
    if (bytesRead > 0)
    {
        buf[bytesRead] = '\0';
    }
    std::string bufStr(buf);
    MESSAGE_TYPE message_type = (MESSAGE_TYPE)stoi(bufStr.substr(0, bufStr.find(CUSTOM_DELIMITER)));

    if (message_type == JOIN)
    {

        MembershipRequest receivedMembershipCommand = MembershipRequest::deserialize(buf);
        std::string new_member_id = Member::generate_member(receivedMembershipCommand.hostname, stoi(receivedMembershipCommand.port));

        // check if it is localhost(for introducer)
        if (!Membershiplist::CheckSelfExist(new_member_id))
        {
            char hostname[256];
            gethostname(hostname, sizeof(hostname));
            std::string currentHostname(hostname);
            Member new_member = Member::parse_member_id(new_member_id);
            if (new_member.hostname == currentHostname)
            {
                Membershiplist::UpdateSelf(new_member_id);
            }
        }

        Membershiplist::AddMember(new_member_id);
        std::string dataToSend;
        for (auto &member : Membershiplist::allMembersList)
        {
            // std::cout << "Addressing member: " << member.first << " data." << endl;
            // Membershiplist::ServerMode mode;

            GossipMessage gossip;
            if (member.first != "")
            {
                // if (member.second.mode == Membershiplist::ServerMode::Gossip)
                if (Membershiplist::servermode == Membershiplist::ServerMode::Gossip)
                {
                    gossip = GossipMessage(std::to_string(Membershiplist::ServerMode::Gossip), member.first, std::to_string(member.second.heartbeat), std::to_string(member.second.clock));
                }
                // else if (member.second.mode == Membershiplist::ServerMode::GossipS)
                else if (Membershiplist::servermode == Membershiplist::ServerMode::GossipS)
                {
                    gossip = GossipMessage(std::to_string(Membershiplist::ServerMode::GossipS), member.first, std::to_string(member.second.heartbeat), std::to_string(member.second.clock), std::to_string(member.second.sus), std::to_string(member.second.incarnation));
                }
                dataToSend += gossip.membership_serialize();
                dataToSend += "\n";
            }
        }
        MemberIdResponse respose(MEMBERID, new_member_id, dataToSend);
        // if (send(new_fd, respose.serialize().c_str(), respose.serialize().size(), 0) == -1)
        if (send(new_fd, respose.serialize().c_str(), respose.serialize().size(), 0) == -1)
        {
            perror("send");
            close(new_fd);
        }
    }
    else if (message_type == LEAVE)
    {
        MembershipRequest receivedMembershipCommand = MembershipRequest::deserialize(buf);
        serverRunning = false;
        std::cout << "Got the command from client: LEAVE, hostname:"
                  << receivedMembershipCommand.hostname << ", map size : " << Membershiplist::Map.size() << ", allmap size : " << Membershiplist::allMembersList.size() << std::endl;
    }
    else if (message_type == MBRLIST)
    {
        Membershiplist::print_membershiplist();
    }
    else if (message_type == MEMBERID)
    {
        MemberIdRequest receivedMembershipCommand = MemberIdRequest::deserialize(buf);
        std::string update_id = receivedMembershipCommand.memberId;
        // std::cout << "MEMBERID: " << update_id << std::endl;
        // Update Selfmember
        if (!Membershiplist::CheckSelfExist(update_id))
        {
            Membershiplist::UpdateSelf(update_id);
            Membershiplist::AddMember(update_id);
        }
        else
        {
            std::cout << "Self already in list!" << endl;
        }
        // Update membershiplist
        unordered_map<string, Membershiplist::MemberKeys> newmem;
        std::istringstream iss(receivedMembershipCommand.membershiplist);
        std::string memberlist;
        while (std::getline(iss, memberlist))
        {
            // GossipMessage gossip = gossip.deserialize(buffer);
            GossipMessage gossip = gossip.deserialize(memberlist);
            Membershiplist::MemberKeys memberKey;
            memberKey.mode = (Membershiplist::ServerMode)stoi(gossip.mode);
            if (Membershiplist::servermode == Membershiplist::ServerMode::Gossip && memberKey.mode == Membershiplist::ServerMode::GossipS)
            {
                std::cout << "Switching to Gossip+S mode!" << endl;
                Membershiplist::EnableGossipS();
            }
            if (memberKey.mode == Membershiplist::ServerMode::GossipS)
            {
                memberKey.incarnation = std::stoi(gossip.incarnation);
                memberKey.sus = std::stoi(gossip.sus);
            }
            else
            {
                memberKey.incarnation = 0;
                memberKey.sus = 0;
            }
            memberKey.clock = std::stoi(gossip.clock);
            memberKey.heartbeat = std::stoi(gossip.heartbeat);
            newmem[gossip.id] = memberKey;
        }
        Membershiplist::UpdateMembershiplist(newmem); // for the instructor
        // std::cout << "check finish" << endl;
    }
    else if (message_type == SUSPICION)
    {
        std::cout << "Switching to GOSSIP+S mode" << std::endl;
        // TODO:
        Membershiplist::EnableGossipS();
    }

    close(new_fd);
}

void MembershipListCommand(int port) // used for address join/leave/print list
{
    int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
    char s[INET_ADDRSTRLEN];
    socklen_t sin_size;

    struct sockaddr_storage their_addr; // connector's address information
    sockfd = generate_TCPserversocket(to_string(PORT_MEMBERSHIP).c_str());
    std::cout << "server for membership_commad: waiting for connections..." << std::endl;

    while (1)
    {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        // std::cout << "server: got connection from " << s << std::endl;

        std::thread clientThread(handleMembershipList, new_fd, port);
        clientThread.detach();
    }
}

void ReceiveGossip(std::string currentHostname, int port)
{

    int listenSock = generate_UDPserverlistensocket(to_string(port).c_str());
    // std::cout << "Hostname: " << currentHostname << ":" << port << "is ready for receiving gossip." << std::endl;
    int test_fp_count = 0;
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    char buffer[MAXDATASIZE];

    while (1)
    {
        if (Membershiplist::Map.size() > 0 && Membershiplist::SelfMember.find(currentHostname) && serverRunning)
        {

            std::string receivedData;
            ssize_t bytesRead = recvfrom(listenSock, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &addrLen);
            if (bytesRead == -1)
            {
                perror("recvfrom");
                close(listenSock);
                exit(1);
            }

            buffer[bytesRead] = '\0'; // Null-terminate the received data

            unordered_map<string, Membershiplist::MemberKeys> newmem;
            string dataReceived(buffer);
            size_t delimiterPos = dataReceived.find(MEMBER_FILE_DELIMITER);

            string firstPart = dataReceived.substr(0, delimiterPos);
            string secondPart = dataReceived.substr(delimiterPos + MEMBER_FILE_DELIMITER.length());

            std::istringstream iss(firstPart);
            std::string memberlist;
            while (std::getline(iss, memberlist))
            {
                // GossipMessage gossip = gossip.deserialize(buffer);
                GossipMessage gossip = gossip.deserialize(memberlist);
                Membershiplist::MemberKeys memberKey;
                memberKey.mode = (Membershiplist::ServerMode)stoi(gossip.mode);
                if (Membershiplist::servermode == Membershiplist::ServerMode::Gossip && memberKey.mode == Membershiplist::ServerMode::GossipS)
                {
                    std::cout << "Switching to Gossip+S mode!" << endl;
                    Membershiplist::EnableGossipS();
                }
                if (memberKey.mode == Membershiplist::ServerMode::GossipS)
                {
                    memberKey.incarnation = std::stoi(gossip.incarnation);
                    memberKey.sus = std::stoi(gossip.sus);
                    // Membershiplist::EnableGossipS();
                }
                else
                {
                    memberKey.incarnation = 0;
                    memberKey.sus = 0;
                }
                memberKey.clock = std::stoi(gossip.clock);
                memberKey.heartbeat = std::stoi(gossip.heartbeat);
                newmem[gossip.id] = memberKey;
            }

            string self = Member::parse_member_id(Membershiplist::SelfMember).hostname;

            if (TEST_FP) // introduce artificial drop
            {
                test_fp_count++;
                if (test_fp_count % 20 == 0)
                {
                    test_fp_count = 0;
                }
                else
                {
                    Membershiplist::compareGossip(newmem);
                }
            }
            else
            {
                Membershiplist::compareGossip(newmem);
            }

            if (Membershiplist::Leader != self)
            {
                FileList::CompareGlobalFile(secondPart);
            }
        }
        else if (!serverRunning)
        {
            Membershiplist::DeleteList();
        }
    }
    close(listenSock);
}

void SendGossip(std::string currentHostname)
{
    // std::cout << "SendGossip currentHostname: " << currentHostname << std::endl;
    // std::cout << "send start, the size of list: " << Membershiplist::allMembersList.size() << Membershiplist::Map.size() << endl;

    int sendSocket = socket(AF_INET, SOCK_DGRAM, 0); // Use UDP socket
    if (sendSocket == -1)
    {
        std::cerr << "Error creating client socket" << std::endl;
        return;
    }

    while (1)
    {
        if (Membershiplist::Map.size() > 0 && Membershiplist::SelfMember.find(currentHostname) && serverRunning)
        {
            std::string dataToSend;

            vector<Member> random_members = Membershiplist::ChooseBRandomMember(b);
            // Membershiplist::ListMutex.lock();
            if (random_members.size() == b)
            {
                GossipMessage gossip;
                // prepare this time send data
                Membershiplist::ListMutex.lock();
                for (auto &member : Membershiplist::allMembersList)
                {
                    // std::cout << "Addressing member: " << member.first << " data." << endl;
                    // Membershiplist::ServerMode mode;
                    if (member.first != "")
                    {
                        // if (member.second.mode == Membershiplist::ServerMode::Gossip)
                        if (Membershiplist::servermode == Membershiplist::ServerMode::Gossip)
                        {
                            gossip = GossipMessage(std::to_string(Membershiplist::ServerMode::Gossip), member.first, std::to_string(member.second.heartbeat), std::to_string(member.second.clock));
                        }
                        // else if (member.second.mode == Membershiplist::ServerMode::GossipS)
                        else if (Membershiplist::servermode == Membershiplist::ServerMode::GossipS)
                        {
                            gossip = GossipMessage(std::to_string(Membershiplist::ServerMode::GossipS), member.first, std::to_string(member.second.heartbeat), std::to_string(member.second.clock), std::to_string(member.second.sus), std::to_string(member.second.incarnation));
                        }
                        dataToSend += gossip.membership_serialize();
                        dataToSend += "\n";
                    }
                }
                Membershiplist::ListMutex.unlock();

                string filelist_string = FileList::GlobalFile_serialize();

                dataToSend = dataToSend + MEMBER_FILE_DELIMITER + filelist_string;
                // std::cout << "dataToSend:" << dataToSend << endl;
                for (auto &random_member : random_members)
                {
                    std::string serverAddress = ConvertHostname2Ip(random_member.hostname, random_member.port);
                    sockaddr_in serverAddressInfo;
                    serverAddressInfo.sin_family = AF_INET;
                    serverAddressInfo.sin_port = htons(stoi(random_member.port));

                    if (inet_pton(AF_INET, serverAddress.c_str(), &(serverAddressInfo.sin_addr)) <= 0)
                    {
                        std::cerr << "Invalid server address: " + serverAddress;
                        close(sendSocket);
                        return;
                    }
                    // dataToSend += "test from " + random_member.hostname;

                    // std::cout << "Gossip sent: \n"
                    //           << dataToSend << std::endl;

                    if (sendto(sendSocket, dataToSend.c_str(), dataToSend.size(), 0, (struct sockaddr *)&serverAddressInfo, sizeof(serverAddressInfo)) == -1)
                    {
                        perror("sendto");
                        close(sendSocket);
                    }
                    // std::cout << "=====================" << endl;
                    // old_member = random_member;
                }
            }

            // Membershiplist::ListMutex.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(T_DISSEM));
        }
    }
    close(sendSocket);
}

void Inc(std::string currentHostname)
{
    // std::cout << "Inc start, the size of list: " << Membershiplist::allMembersList.size() << Membershiplist::Map.size() << endl;
    while (serverRunning)
    {
        if (Membershiplist::SelfMember != "")
        {
            if (Membershiplist::servermode == Membershiplist::ServerMode::Gossip)
                Membershiplist::detectFailure();
            else if (Membershiplist::servermode == Membershiplist::ServerMode::GossipS)
                Membershiplist::detectFailureS();
            // std::cout << "Start self increase!" << endl;
            Membershiplist::clockInc();
            Membershiplist::heartbeatInc();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void handleSDFS(int new_fd, int port)
{
    // SDFSRequest request;
    char buffer[SOCKET_BUFFER_SIZE];
    int bytesRead;

    bytesRead = recv(new_fd, buffer, sizeof(SDFSRequest), 0);
    if (bytesRead < 0)
    {
        perror("SDFS recv");
        exit(1);
    }
    // request = deserializeSDFSRequest(buffer);

    string requestbufStr(buffer);
    cout << "got request: " << requestbufStr << endl;
    MESSAGE_TYPE message_type = (MESSAGE_TYPE)stoi(requestbufStr.substr(0, requestbufStr.find(CUSTOM_DELIMITER)));
    Member member = Member::parse_member_id(Membershiplist::SelfMember);
    SDFSRequest request = SDFSRequest::deserialize(requestbufStr);
    cout << "got request: " << requestbufStr << endl;
    if (message_type == SDFS_PUT)
    {
        if (request.hostname == member.hostname)
        {
            std::cout << "got SDFS put from my client" << endl;

            int LeaderSocket = generate_TCP_client_socket(Membershiplist::Leader, SDFS_PORT);
            // vector<char> serializeData = serializeSDFSRequest(request);
            request.message_type = SDFS_PUT_LEADER;
            ssize_t sentBytes = send(LeaderSocket, request.serialize().c_str(), request.serialize().size(), 0); // ask leader for replicas list
            // ssize_t sentBytes = send(LeaderSocket, serializeData.data(), serializeData.size(), 0); // ask leader for replicas list
            if (sentBytes == -1)
            {
                cerr << "Error sending command to server" << endl;
                close(LeaderSocket);
                exit(1);
            }

            vector<char> buffer(MAXDATASIZE);
            int bytesRead = recv(LeaderSocket, buffer.data(), buffer.size(), 0);
            SDFSReturnMsg response;

            // if (bytesRead <= 0)
            // {
            //     perror("SDFS response");
            //     exit(1);
            // }
            int leaderversion;
            bool update = false;
            if (bytesRead > 0)
            {
                response = deserializeSDFSResponse(buffer.data());
                buffer.resize(bytesRead);
                response.fixReplicaServerNames();
                leaderversion = response.timestamp;
                update = response.hasFile;
                // std::cout << "Give replicas to:" << endl;
                // for (int i = 0; i < response.replicaMembers.size(); i++)
                // {
                //     std::cout << response.replicaMembers[i] << endl;
                // }
                // std::cout << "Version: " << response.timestamp << endl;
            }
            else
            {
                perror("Something wrong, try again");
                // exit(1);
            }

            send(new_fd, START_RECV_FILE.c_str(), sizeof(START_RECV_FILE), 0); // send back the ready receive message
            string temp_sdfsfilename = SDFSFileTransfer::generate_temp_sdfsfilename(request.sdfsfilename);
            SDFSFileTransfer::recv_file(new_fd, temp_sdfsfilename); // receive file and save in self server first
            // close(LeaderSocket);
            // cout << "put sleep for 5 seconds!" << endl;
            // for (int i = 0; i < 10; i++)
            // {
            //     cout << i << " second." << endl;
            //     this_thread::sleep_for(std::chrono::seconds(1));
            // }
            SDFSRequest sdfsRequest_server;

            vector<char> serializedData_server = serializeSDFSRequest(sdfsRequest_server);
            bool contain_itself = false;
            int success = 0;
            for (int i = 0; i < response.replicaMembers.size(); i++) // TODO: change to wait for Qurom ack when update
            {
                if (update && success == 3) // Quorum
                {
                    break;
                }
                if (response.replicaMembers[i] == member.hostname)
                {
                    contain_itself = true;
                    success++;
                    continue;
                }
                char buffer_server[SOCKET_BUFFER_SIZE] = {0};

                int OtherServerSocket = generate_TCP_client_socket(response.replicaMembers[i], SDFS_PORT);
                sdfsRequest_server.message_type = SDFS_PUT_LOCAL;
                sdfsRequest_server.sdfsfilename = request.sdfsfilename;
                sdfsRequest_server.version = leaderversion;
                // std::cout << "sdfsRequest_server.serialize().c_str():" << sdfsRequest_server.serialize() << endl;
                ssize_t sentBytes = send(OtherServerSocket, sdfsRequest_server.serialize().c_str(), sdfsRequest_server.serialize().size(), 0);
                if (sentBytes == -1)
                {
                    cerr << "Error connect to serverL" << response.replicaMembers[i] << endl;
                    exit(1);
                }
                recv(OtherServerSocket, buffer_server, SOCKET_BUFFER_SIZE, 0);
                string resp;
                stringstream strstream(buffer_server);
                strstream >> resp;
                if (START_RECV_FILE == resp)
                {
                    // std::cout << "Other Server is ready to receive the file!!!" << endl;
                    SDFSFileTransfer::send_file(OtherServerSocket, temp_sdfsfilename);
                    recv(OtherServerSocket, buffer_server, SOCKET_BUFFER_SIZE, 0);
                    stringstream strstream(buffer_server);
                    strstream >> resp;
                    if (FINISH_TRANSFER == resp)
                    {
                        success++;
                    }
                }
                close(OtherServerSocket);
            }

            if ((success == response.replicaMembers.size() && !update) || (success == 3 && update) || update) // TODO: remove the last condition, it is just for debugging
            {
                // if self is not in the replica list, delete the temp file
                if (contain_itself)
                {
                    // rename the temp sdfsfile
                    if (std::rename(temp_sdfsfilename.c_str(), request.sdfsfilename.c_str()) != 0)
                    {
                        cerr << "Error renaming file." << endl;
                        return exit(1); // Error handling, return an error code
                    }

                    // insert into local filemap
                    FileList::LocalMutex.lock();
                    FileList::FileMap[request.sdfsfilename] = response.timestamp;
                    FileList::LocalMutex.unlock();
                }
                else
                {
                    // remove the temp sdfsdile
                    if (std::remove(temp_sdfsfilename.c_str()) != 0)
                    {
                        std::cerr << "Error removing the file." << std::endl;
                        return exit(1); // Error handling, return an error code
                    }
                }
                send(new_fd, FINISH_TRANSFER.c_str(), sizeof(FINISH_TRANSFER), 0);
                // FileRequest::putCount--;
                // FileRequest::putQueue.pop();
            }
            else
            {
                send(new_fd, SOMETHING_WRONG.c_str(), sizeof(SOMETHING_WRONG), 0);
            }
            send(LeaderSocket, IAMDONE.c_str(), IAMDONE.size(), 0);
            close(new_fd);
            close(LeaderSocket);
        }
    }
    else if (message_type == SDFS_GET)
    {
        if (request.hostname == member.hostname) // ask leader for the
        {
            int LeaderSocket = generate_TCP_client_socket(Membershiplist::Leader, SDFS_PORT);
            request.message_type = SDFS_GET_LEADER;
            ssize_t sentBytes = send(LeaderSocket, request.serialize().c_str(), request.serialize().size(), 0); // ask leader replicas list contain file

            std::cout << "got SDFS get from my client" << endl;

            vector<char> buffer(MAXDATASIZE);
            int bytesRead;
            bytesRead = recv(LeaderSocket, buffer.data(), buffer.size(), 0);
            SDFSReturnMsg response;

            if (bytesRead > 0)
            {
                response = deserializeSDFSResponse(buffer.data());
                buffer.resize(bytesRead);
                response.fixReplicaServerNames();
                std::cout << "The sdfs file:" << request.sdfsfilename << " store at:" << endl;
                for (int i = 0; i < response.replicaMembers.size(); i++)
                {
                    std::cout << response.replicaMembers[i] << endl;
                }
            }
            else
            {
                perror("SDFS Something wrong, try again");
                // exit(1);
            }

            // cout << "get sleep for 5 seconds!" << endl;
            // for (int i = 0; i < 10; i++)
            // {
            //     cout << i << " second." << endl;
            //     this_thread::sleep_for(std::chrono::seconds(1));
            // }

            if (response.hasFile)
            {
                // ask each replica the file version
                bool contain_itself = false;
                int success = 0;
                SDFSRequest sdfsRequest_server;
                sdfsRequest_server.message_type = SDFS_ASK_LOCAL_VERSION;
                sdfsRequest_server.sdfsfilename = request.sdfsfilename;

                unordered_map<string, int> replica_version;
                for (int i = 0; i < response.replicaMembers.size(); i++) // TODO: change to wait for Qurom ack when update
                {
                    if (success == 3) // Read Quroum
                    {
                        break;
                    }

                    if (response.replicaMembers[i] == member.hostname)
                    {
                        replica_version[response.replicaMembers[i]] = FileList::FileMap[request.sdfsfilename];
                        success++;
                        continue;
                    }
                    try
                    {
                        char buffer_server[SOCKET_BUFFER_SIZE] = {0};
                        int OtherServerSocket = generate_TCP_client_socket(response.replicaMembers[i], SDFS_PORT);
                        ssize_t sentBytes = send(OtherServerSocket, sdfsRequest_server.serialize().c_str(), sdfsRequest_server.serialize().size(), 0);
                        if (sentBytes == -1)
                        {
                            cerr << "Error connect to serverL" << response.replicaMembers[i] << endl;
                            // exit(1);
                        }
                        else
                        {
                            recv(OtherServerSocket, buffer_server, SOCKET_BUFFER_SIZE, 0);
                            string resp;
                            stringstream strstream(buffer_server);
                            strstream >> resp;
                            std::cout << "version " << resp << " store at " << response.replicaMembers[i] << endl;
                            replica_version[response.replicaMembers[i]] = stoi(resp);
                            close(OtherServerSocket);
                            success++;
                        }
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << e.what() << '\n';
                    }
                }

                // check which replica has the highest version
                int max_version = -1;
                std::string max_replica;
                for (const auto &pair : replica_version)
                {
                    const std::string &key = pair.first;
                    int value = pair.second;
                    if (value > max_version)
                    {
                        max_version = value;
                        max_replica = key;
                    }
                }
                std::cout << "max replica is:" << max_replica << "max_version:" << max_version << endl;
                // check if the higest replica is itself
                if (max_replica == member.hostname)
                {
                    // then just copy a file from sdfs to local
                    std::ifstream sourceFile(request.sdfsfilename, std::ios::binary);
                    std::ofstream destinationFile(request.localfilename, std::ios::binary);

                    if (!sourceFile)
                    {
                        std::cerr << "Error opening source file." << std::endl;
                        send(new_fd, SOMETHING_WRONG.c_str(), sizeof(SOMETHING_WRONG), 0);
                    }

                    if (!destinationFile)
                    {
                        std::cerr << "Error opening destination file." << std::endl;
                        send(new_fd, SOMETHING_WRONG.c_str(), sizeof(SOMETHING_WRONG), 0);
                    }

                    destinationFile << sourceFile.rdbuf();

                    if (!sourceFile)
                    {
                        std::cerr << "Error reading from source file." << std::endl;
                        send(new_fd, SOMETHING_WRONG.c_str(), sizeof(SOMETHING_WRONG), 0);
                    }

                    if (!destinationFile)
                    {
                        std::cerr << "Error writing to destination file." << std::endl;
                        send(new_fd, SOMETHING_WRONG.c_str(), sizeof(SOMETHING_WRONG), 0);
                    }
                    send(new_fd, FINISH_TRANSFER.c_str(), sizeof(FINISH_TRANSFER), 0);
                }
                else
                {
                    SDFSRequest sdfsRequest_get;
                    sdfsRequest_get.message_type = SDFS_GET_LOCAL;
                    sdfsRequest_get.sdfsfilename = request.sdfsfilename;

                    // ask for that server and save in local
                    int OtherServerSocket = generate_TCP_client_socket(max_replica, SDFS_PORT);
                    ssize_t sentBytes = send(OtherServerSocket, sdfsRequest_get.serialize().c_str(), sdfsRequest_get.serialize().size(), 0);
                    char buffer_server_get[SOCKET_BUFFER_SIZE] = {0};
                    recv(OtherServerSocket, buffer_server_get, SOCKET_BUFFER_SIZE, 0);
                    string resp;
                    stringstream strstream(buffer_server_get);
                    strstream >> resp;
                    if (START_RECV_FILE == resp)
                    {
                        send(OtherServerSocket, START_RECV_FILE.c_str(), sizeof(START_RECV_FILE), 0);
                        SDFSFileTransfer::recv_file(OtherServerSocket, request.localfilename); // receive file and save in self server first
                        send(OtherServerSocket, FINISH_TRANSFER.c_str(), sizeof(FINISH_TRANSFER), 0);
                        send(new_fd, FINISH_TRANSFER.c_str(), sizeof(FINISH_TRANSFER), 0);
                    }
                    else
                    {
                        send(new_fd, SOMETHING_WRONG.c_str(), sizeof(SOMETHING_WRONG), 0);
                    }
                }
            }
            else
            {
                send(new_fd, NO_FILE_EXIST.c_str(), sizeof(NO_FILE_EXIST), 0);
            }
            send(LeaderSocket, IAMDONE.c_str(), IAMDONE.size(), 0);
            close(new_fd);
            close(LeaderSocket);
        }
    }
    else if (message_type == SDFS_DELETE)
    {
        if (request.hostname == member.hostname) //&& strcmp(request.port.c_str(), "0") == 0
        {
            std::cout << "got SDFS delete from my client" << endl;

            int LeaderSocket = generate_TCP_client_socket(Membershiplist::Leader, SDFS_PORT);
            request.message_type = SDFS_DELETE_LEADER;
            ssize_t sentBytes = send(LeaderSocket, request.serialize().c_str(), request.serialize().size(), 0);
            if (sentBytes == -1)
            {
                cerr << "Error sending Delete to Leader" << endl;
                close(LeaderSocket);
                exit(1);
            }

            vector<char> buffer(MAXDATASIZE);
            int bytesRead = recv(LeaderSocket, buffer.data(), buffer.size(), 0);
            SDFSReturnMsg response;
            if (bytesRead < 0)
            {
                perror("SDFS delete response");
                exit(1);
            }
            response = deserializeSDFSResponse(buffer.data());
            buffer.resize(bytesRead);

            int clientSocket = generate_TCP_client_socket("localhost", SDFS_PORT);
            char buf[MAXDATASIZE];
            if (response.hasFile)
            {
                strcpy(buf, "Delete Sucessfully");
            }
            else
            {
                strcpy(buf, "File not found");
            }
            sentBytes = send(new_fd, buf, MAXDATASIZE - 1, 0);
            if (sentBytes < 0)
            {
                perror("Delete file reply to client");
                exit(1);
            }
            send(LeaderSocket, IAMDONE.c_str(), IAMDONE.size(), 0);
            close(new_fd);
        }
    }
    else if (message_type == SDFS_PUT_LEADER)
    {
        cout << "In!!!!!!" << endl;
        FileList::GlobalMutex.lock();
        FileList::VersionNumber++;
        FileList::GlobalMutex.unlock();

        FileRequest::processQueue(FileList::VersionNumber, request);

        std::cout << " start put localfile: " << request.localfilename << " to sdfsfile: " << request.sdfsfilename << std::endl;

        SDFSReturnMsg returnMsg;
        if (FileList::CheckFileExistGlobal(request.sdfsfilename)) // TODO: update the version of the sdfs and tell all the replicas
        {
            std::cout << request.sdfsfilename << " is already in Global file list, need to connect replicas to update content!" << endl;
            FileList::UpdateFileGlobal(FileList::VersionNumber, request.sdfsfilename);
            FileList::GlobalMutex.lock();
            vector<string> replicas;
            for (auto &entry : FileList::FileMetadataMap[request.sdfsfilename]->version_replicaids)
            {
                replicas = entry.second;
            }
            FileList::GlobalMutex.unlock();

            returnMsg.replicaMembers = replicas;
            returnMsg.hasFile = true;
        }
        else
        {
            vector<string> members(Membershiplist::Map.begin(), Membershiplist::Map.end());
            random_device rd;
            mt19937 gen(rd());
            shuffle(members.begin(), members.end(), gen);

            int replicaNum = min(Replicas, (int)Membershiplist::Map.size());
            vector<string> replicas(members.begin(), members.begin() + replicaNum);
            FileList::InsertFileGlobal(FileList::VersionNumber, request.sdfsfilename, replicas);
            returnMsg.replicaMembers = replicas;
            returnMsg.hasFile = false;
        }
        returnMsg.timestamp = FileList::VersionNumber;
        vector<char> serializeData = serializeSDFSResponse(returnMsg);
        ssize_t sentBytes = send(new_fd, serializeData.data(), serializeData.size(), 0);

        // get ok from ask server
        char buffer_server_get[SOCKET_BUFFER_SIZE] = {0};
        recv(new_fd, buffer_server_get, SOCKET_BUFFER_SIZE, 0);
        string resp;
        stringstream strstream(buffer_server_get);
        strstream >> resp;
        if (IAMDONE == resp)
        {
            cout << "Finish request from " << request.hostname << endl;
            FileRequest::FinishQueue(request);
        }
    }
    else if (message_type == SDFS_GET_LEADER)
    {
        FileList::GlobalMutex.lock();
        FileList::VersionNumber++;
        FileList::GlobalMutex.unlock();

        FileRequest::processQueue(FileList::VersionNumber, request);

        std::cout << "start ask for: " << request.sdfsfilename << std::endl;
        SDFSReturnMsg returnMsg;
        if (FileList::CheckFileExistGlobal(request.sdfsfilename))
        { // return the replica list
            std::cout << request.sdfsfilename << "exist! " << std::endl;
            FileList::GlobalMutex.lock();
            vector<string> replicas;
            for (auto &entry : FileList::FileMetadataMap[request.sdfsfilename]->version_replicaids)
            {
                replicas = entry.second;
            }
            FileList::GlobalMutex.unlock();
            returnMsg.replicaMembers = replicas;
            returnMsg.hasFile = true;
            vector<char> serializeData = serializeSDFSResponse(returnMsg);
            ssize_t sentBytes = send(new_fd, serializeData.data(), serializeData.size(), 0);
        }
        else
        {
            std::cout << request.sdfsfilename << "doesn't exist! " << std::endl;
            returnMsg.hasFile = false;
            vector<char> response = serializeSDFSResponse(returnMsg);
            int sentBytes = send(new_fd, response.data(), response.size(), 0);
            if (sentBytes < 0)
            {
                perror("return MSG to query server");
                exit(1);
            }
        }

        // get ok from ask server
        char buffer_server_get[SOCKET_BUFFER_SIZE] = {0};
        recv(new_fd, buffer_server_get, SOCKET_BUFFER_SIZE, 0);
        string resp;
        stringstream strstream(buffer_server_get);
        strstream >> resp;
        if (IAMDONE == resp)
        {
            cout << "Finish request from " << request.hostname << endl;
            FileRequest::FinishQueue(request);
        }
    }
    else if (message_type == SDFS_DELETE_LEADER)
    {
        FileList::GlobalMutex.lock();
        FileList::VersionNumber++;
        FileList::GlobalMutex.unlock();

        std::cout << "Delete File: " << request.sdfsfilename << endl;

        SDFSReturnMsg returnMsg;
        if (FileList::CheckFileExistGlobal(request.sdfsfilename))
        {
            returnMsg.hasFile = true;
            int deleteCount = 0;
            vector<string> replicaList = FileList::GetFileReplicas(request.sdfsfilename);
            for (int i = 0; i < replicaList.size(); i++)
            {
                Member replica = Member::parse_member_id(replicaList[i]);
                int otherServerClient = generate_TCP_client_socket(replica.hostname, SDFS_PORT);
                request.message_type = SDFS_DELETE_LOCAL;
                int sentBytes = send(otherServerClient, request.serialize().c_str(), request.serialize().size(), 0);
                if (sentBytes < 0)
                {
                    perror("send delete local to other server");
                    exit(1);
                }
                char buf[MAXDATASIZE];
                int bytesRead = recv(otherServerClient, buf, MAXDATASIZE - 1, 0);
                // std::cout << "server responds: " << buf << endl;
                if (strcmp(buf, "Deleted") == 0)
                {
                    deleteCount++;
                }
            }
            // std::cout << "deleteCount:" << deleteCount << ", replicaList.size():" << replicaList.size() << endl;
            if (deleteCount == replicaList.size())
            {
                std::cout << "delete " << request.sdfsfilename << " from meta map" << endl;
                vector<char> serializedData = serializeSDFSResponse(returnMsg);
                FileList::DeleteFileGlobal(request.sdfsfilename);
                int sentBytes = send(new_fd, serializedData.data(), serializedData.size(), 0);
            }
        }
        else
        {
            returnMsg.hasFile = false;
            vector<char> response = serializeSDFSResponse(returnMsg);
            int sentBytes = send(new_fd, response.data(), response.size(), 0);
            if (sentBytes < 0)
            {
                perror("return MSG to query server");
                exit(1);
            }
        }

        // get ok from ask server
        char buffer_server_get[SOCKET_BUFFER_SIZE] = {0};
        recv(new_fd, buffer_server_get, SOCKET_BUFFER_SIZE, 0);
        string resp;
        stringstream strstream(buffer_server_get);
        strstream >> resp;
        if (IAMDONE == resp)
        {
            FileRequest::FinishQueue(request);
        }
    }
    else if (message_type == SDFS_LS)
    {
        FileList::PrintFileLocation(request.sdfsfilename);
    }
    else if (message_type == SDFS_STORE)
    {
        FileList::PrintFileStoreLocal();
    }
    else if (message_type == SDFS_PRINT_GLOBAL)
    {
        FileList::PrintFileList();
    }
    else if (message_type == SDFS_PUT_LOCAL)
    {
        send(new_fd, START_RECV_FILE.c_str(), sizeof(START_RECV_FILE), 0);
        // TODO: UPDATE EXISTED FILE FUNCTION
        SDFSFileTransfer::recv_file(new_fd, request.sdfsfilename);
        send(new_fd, FINISH_TRANSFER.c_str(), sizeof(FINISH_TRANSFER), 0);
        std::cout << "Get the sdfs file " << request.sdfsfilename << " version:" << request.version << " done!" << endl;

        // Insert to local file list
        FileList::LocalMutex.lock();
        FileList::FileMap[request.sdfsfilename] = request.version;
        FileList::LocalMutex.unlock();
    }
    else if (message_type == SDFS_GET_LOCAL)
    {
        char buffer_server[SOCKET_BUFFER_SIZE] = {0};
        send(new_fd, START_RECV_FILE.c_str(), sizeof(START_RECV_FILE), 0);
        recv(new_fd, buffer_server, SOCKET_BUFFER_SIZE, 0);
        string resp;
        stringstream strstream(buffer_server);
        strstream >> resp;
        if (START_RECV_FILE == resp)
        {
            std::cout << "Other Server is ready to receive the file!!!" << endl;
            SDFSFileTransfer::send_file(new_fd, request.sdfsfilename);
            recv(new_fd, buffer_server, SOCKET_BUFFER_SIZE, 0);
            stringstream strstream(buffer_server);
            strstream >> resp;
            if (FINISH_TRANSFER == resp)
            {
                std::cout << "Send the sdfs file " << request.sdfsfilename << " done!" << endl;
            }
        }
    }
    else if (message_type == SDFS_ASK_LOCAL_VERSION)
    {
        string version = to_string(FileList::FileMap[request.sdfsfilename]);
        ssize_t sentBytes = send(new_fd, version.c_str(), sizeof(version), 0);
        if (sentBytes == -1)
        {
            cerr << "Error connect to server" << endl;
            exit(1);
        }
    }
    else if (message_type == SDFS_DELETE_LOCAL)
    {
        if (std::remove(request.sdfsfilename.c_str()) != 0)
        {
            std::cerr << "Error removing the file." << std::endl;
            return; // Error handling, return an error code
        }

        FileList::DeleteFileLocal(request.sdfsfilename);
        if (!FileList::CheckFileExistLocal(request.sdfsfilename))
        {
            char buf[MAXDATASIZE];
            strcpy(buf, "Deleted");
            send(new_fd, buf, MAXDATASIZE - 1, 0);
        }
        std::cout << "Delete file: " << request.sdfsfilename << " success." << endl;
    }
    else if (message_type == SDFS_FR_PUT)
    {
        cout << "I have replica: " << request.sdfsfilename << ", I need to send this to " << request.hostname << " with version : " << request.version << endl;
        char buffer_server[SOCKET_BUFFER_SIZE] = {0};
        SDFSRequest sdfsRequest_server;
        int OtherServerSocket = generate_TCP_client_socket(request.hostname, SDFS_PORT);
        sdfsRequest_server.message_type = SDFS_FR_PUT_LOCAL;
        sdfsRequest_server.sdfsfilename = request.sdfsfilename;
        sdfsRequest_server.version = request.version;
        ssize_t sentBytes = send(OtherServerSocket, sdfsRequest_server.serialize().c_str(), sdfsRequest_server.serialize().size(), 0);
        if (sentBytes == -1)
        {
            cerr << "Error connect to server " << request.hostname << endl;
            exit(1);
        }
        recv(OtherServerSocket, buffer_server, SOCKET_BUFFER_SIZE, 0);
        string resp;
        stringstream strstream(buffer_server);
        strstream >> resp;
        if (START_RECV_FILE == resp)
        {
            // std::cout << "Other Server is ready to receive the file!!!" << endl;
            SDFSFileTransfer::send_file(OtherServerSocket, request.sdfsfilename);
            recv(OtherServerSocket, buffer_server, SOCKET_BUFFER_SIZE, 0);
            stringstream strstream(buffer_server);
            strstream >> resp;
            if (FINISH_TRANSFER == resp)
            {
                cout << "transfer success." << endl;
            }
        }
        close(OtherServerSocket);
        send(new_fd, IAMDONE.c_str(), IAMDONE.size(), 0);
    }
    else if (message_type == SDFS_FR_PUT_LOCAL)
    {
        send(new_fd, START_RECV_FILE.c_str(), sizeof(START_RECV_FILE), 0);
        // TODO: UPDATE EXISTED FILE FUNCTION
        SDFSFileTransfer::recv_file(new_fd, request.sdfsfilename);
        send(new_fd, FINISH_TRANSFER.c_str(), sizeof(FINISH_TRANSFER), 0);
        std::cout << "Get the sdfs file " << request.sdfsfilename << " version:" << request.version << " done!" << endl;

        // Insert to local file list
        FileList::LocalMutex.lock();
        FileList::FileMap[request.sdfsfilename] = request.version;
        FileList::LocalMutex.unlock();
    }
    else if (message_type == SDFS_FR_INSERT_RECOVER)
    {
        FileList::InsertRecover(request.hostname);
    }
    close(new_fd);
}

void SDFSCommand(int port) // used for sdfs
{
    int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
    char s[INET_ADDRSTRLEN];
    socklen_t sin_size;

    struct sockaddr_storage their_addr; // connector's address information
    sockfd = generate_TCP_SDFS_serversocket(to_string(SDFS_PORT).c_str());
    std::cout << "server for SDFS_commad: waiting for connections..." << std::endl;

    while (1)
    {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("accept");
            // continue;
            exit(1);
        }
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        std::cout << "SDFS server: got connection from " << s << std::endl;
        std::thread clientThread2(handleSDFS, new_fd, port);
        clientThread2.detach();
    }
}

void FailureRecovery(string currentHostname, int port) // used for sdfs
{
    while (1)
    {
        if (Membershiplist::membersToRemove.size() > 1 && Membershiplist::SelfMember.find(currentHostname) && serverRunning && Membershiplist::Leader == currentHostname)
        {
            // cout << "start checking failure!" << endl;
            auto start = chrono::high_resolution_clock::now();
            unordered_map<string, set<string>> failservers_replicas;
            set<string> check = Membershiplist::membersToRemove;
            failservers_replicas = FileList::CheckRecovery(check);

            for (auto &failserver : failservers_replicas)
            {
                string server = failserver.first;
                if (!failserver.second.empty())
                {
                    this_thread::sleep_for(std::chrono::seconds(1));
                    FileList::GlobalMutex.lock();
                    FileList::Recoverstart = true;
                    FileList::GlobalMutex.unlock();
                    cout << "failserver: " << server << endl;
                    // for each file, ask all remains servers who also have it and take the highest version
                    vector<string> whohasreplia;
                    for (auto &replica : failserver.second)
                    {
                        whohasreplia = FileList::GetFileReplicas(replica);
                        int max_version = -1;
                        string max_server;
                        SDFSRequest sdfsRequest_FR;
                        sdfsRequest_FR.message_type = SDFS_ASK_LOCAL_VERSION;
                        sdfsRequest_FR.sdfsfilename = replica;
                        // ask each alive server the version of the sdfs file(get the server with highest version)
                        for (auto &servercontainreplia : whohasreplia)
                        {
                            Member aliveserver = Member::parse_member_id(servercontainreplia);
                            if (Membershiplist::membersToRemove.find(servercontainreplia) == Membershiplist::membersToRemove.end()) // this server is alive, ask it!
                            {
                                char buffer_server[SOCKET_BUFFER_SIZE] = {0};
                                int OtherServerSocket = generate_TCP_client_socket(aliveserver.hostname, SDFS_PORT);
                                ssize_t sentBytes = send(OtherServerSocket, sdfsRequest_FR.serialize().c_str(), sdfsRequest_FR.serialize().size(), 0);
                                if (sentBytes == -1)
                                {
                                    cerr << "Error connect to server" << servercontainreplia << endl;
                                }
                                else
                                {
                                    recv(OtherServerSocket, buffer_server, SOCKET_BUFFER_SIZE, 0);
                                    string resp;
                                    stringstream strstream(buffer_server);
                                    strstream >> resp;
                                    std::cout << "version " << resp << " store at " << servercontainreplia << endl;
                                    if (max_version < stoi(resp))
                                    {
                                        max_version = stoi(resp);
                                        max_server = servercontainreplia;
                                    }
                                    close(OtherServerSocket);
                                }
                            }
                        }

                        // ask the max_version to put replica to target server that doesn't contain it
                        string target_server;
                        // find the server that don't have that replica
                        vector<string> members_shuffle(Membershiplist::Map.begin(), Membershiplist::Map.end());
                        random_device rd;
                        mt19937 gen(rd());
                        shuffle(members_shuffle.begin(), members_shuffle.end(), gen);
                        for (auto &aliveserver : members_shuffle)
                        {
                            if (find(whohasreplia.begin(), whohasreplia.end(), aliveserver) == whohasreplia.end())
                            {
                                target_server = aliveserver;
                                break;
                            }
                        }
                        sdfsRequest_FR.message_type = SDFS_FR_PUT;
                        sdfsRequest_FR.sdfsfilename = replica;
                        sdfsRequest_FR.hostname = Member::parse_member_id(target_server).hostname; // who will receive replica
                        sdfsRequest_FR.version = max_version;
                        cout << "Transfer " << replica << " to server:" << sdfsRequest_FR.hostname << endl;
                        int max_server_socket = generate_TCP_client_socket(Member::parse_member_id(max_server).hostname, SDFS_PORT);
                        ssize_t sentBytes = send(max_server_socket, sdfsRequest_FR.serialize().c_str(), sdfsRequest_FR.serialize().size(), 0);
                        if (sentBytes == -1)
                        {
                            cerr << "Error connect to server" << sdfsRequest_FR.hostname << endl;
                        }

                        // after transfer the file success, add the target server into global filelist

                        char buffer_server_get[SOCKET_BUFFER_SIZE] = {0};
                        recv(max_server_socket, buffer_server_get, SOCKET_BUFFER_SIZE, 0);
                        string resp;
                        stringstream strstream(buffer_server_get);
                        strstream >> resp;
                        if (IAMDONE == resp)
                        {
                            cout << "success transfer fail server: " << server << " replica: " << replica << endl;
                        }

                        FileList::GlobalMutex.lock();
                        for (auto &r : FileList::FileMetadataMap[replica]->version_replicaids)
                        {
                            r.second.push_back(target_server);
                        }
                        FileList::GlobalMutex.unlock();
                    }
                    // delete the server from the global file list
                    FileList::DeleteServerGlobal(server);

                    // tell all alive member to put fail server into recover
                    SDFSRequest Insert_recover;
                    for (auto &aliveserver : Membershiplist::Map)
                    {
                        if (aliveserver == Membershiplist::SelfMember)
                        {
                            continue;
                        }
                        Member aliveserverhost = Member::parse_member_id(aliveserver);
                        Insert_recover.message_type = SDFS_FR_INSERT_RECOVER;
                        Insert_recover.hostname = server;
                        int other_server_socket = generate_TCP_client_socket(aliveserverhost.hostname, SDFS_PORT);
                        ssize_t sentBytes = send(other_server_socket, Insert_recover.serialize().c_str(), Insert_recover.serialize().size(), 0);
                        if (sentBytes == -1)
                        {
                            cerr << "Error connect to server" << endl;
                        }
                    }
                }
                FileList::InsertRecover(server);
            }
            auto stop = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
            float time = float(duration.count()) / 1000000.0;
            if (FileList::Recoverstart)
            {
                cout << "Recovery time: " << time << " seconds" << endl;
            }
            // cout << "Checking Done!" << endl;
            FileList::GlobalMutex.lock();
            FileList::Recoverstart = false;
            FileList::GlobalMutex.unlock();
        }
        this_thread::sleep_for(std::chrono::seconds(1));
    }
    // while (1)
    // {
    //     sin_size = sizeof their_addr;
    //     new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    //     if (new_fd == -1)
    //     {
    //         perror("accept");
    //         continue;
    //     }
    //     inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
    //     std::cout << "SDFS server: got connection from " << s << std::endl;
    //     std::thread clientThread3(handleFR, new_fd, port);
    //     clientThread3.detach();
    // }
}

int hashPartition(const string &line, int numPartitions)
{
    int hash = 0;
    for (char c : line)
    {
        hash = (hash * 31 + c) % numPartitions;
    }
    return hash;
}

void handleMJ(int new_fd, int port)
{
    char buffer[SOCKET_BUFFER_SIZE];
    int bytesRead;

    bytesRead = recv(new_fd, buffer, sizeof(MJRequest), 0);
    if (bytesRead < 0)
    {
        perror("MJ recv");
        exit(1);
    }
    // request = deserializeSDFSRequest(buffer);

    string requestbufStr(buffer);
    cout << "got request: " << requestbufStr << endl;
    MESSAGE_TYPE message_type = (MESSAGE_TYPE)stoi(requestbufStr.substr(0, requestbufStr.find(CUSTOM_DELIMITER)));
    Member member = Member::parse_member_id(Membershiplist::SelfMember);
    MJRequest request = MJRequest::deserialize(requestbufStr);

    if (message_type == MAPLE)
    {
        cout << "Execute maple function!" << endl;
        // Put maple_exe in sdfs
        int LeaderSocket = generate_TCP_client_socket(Membershiplist::Leader, MJ_PORT);
        int mapleSocket = generate_TCP_client_socket(Membershiplist::Leader, SDFS_PORT);
        SDFSRequest maplePutRequest;
        maplePutRequest.message_type = SDFS_PUT_LEADER;
        maplePutRequest.localfilename = request.exe_file;
        maplePutRequest.sdfsfilename = "maple_exe";
        ssize_t sentBytes = send(mapleSocket, maplePutRequest.serialize().c_str(), maplePutRequest.serialize().size(), 0);
        if (sentBytes == -1)
        {
            cerr << "Error sending maple put command to leader" << endl;
            close(mapleSocket);
            exit(1);
        }

        if (request.src_dict.back() != '/')
        {
            request.src_dict += '/';
        }
        for (const auto &file : filesystem::directory_iterator(request.src_dict))
        {
            maplePutRequest.localfilename = file.path().filename().string();
            maplePutRequest.sdfsfilename = request.src_dict + file.path().filename().string();
            maplePutRequest.message_type = SDFS_PUT_LEADER;
            sentBytes = send(mapleSocket, maplePutRequest.serialize().c_str(), maplePutRequest.serialize().size(), 0);
            if (sentBytes == -1)
            {
                cerr << "Error putting input files to sdfs" << endl;
                close(mapleSocket);
                exit(1);
            }
        }

        request.message_type = MAPLE_LEADER;
        sentBytes = send(LeaderSocket, request.serialize().c_str(), request.serialize().size(), 0);
        if (sentBytes == -1)
        {
            cerr << "Error sending maple request to leader" << endl;
            close(LeaderSocket);
            exit(1);
        }

        char buffer[MAXDATASIZE];
        int bytesRead = recv(LeaderSocket, buffer, MAXDATASIZE, 0);
        if (bytesRead <= 0)
        {
            cerr << "Error receiving results from maple leader" << endl;
            exit(1);
        }
        sentBytes = send(new_fd, buffer, bytesRead, 0);
        if (sentBytes < 0)
        {
            cerr << "Error sending results to client" << endl;
            exit(1);
        }
    }
    else if (message_type == JUICE)
    {
        cout << "Execute juice function!" << endl;
        int leaderSocket = generate_TCP_client_socket(Membershiplist::Leader, MJ_PORT);
        int sdfsSocket = generate_TCP_client_socket(Membershiplist::Leader, SDFS_PORT);
        SDFSRequest sdfsRequest;
        sdfsRequest.message_type = SDFS_PUT;
        sdfsRequest.localfilename = "juice_exe.py";
        sdfsRequest.sdfsfilename = "juice_exe";
        ssize_t sentBytes = send(sdfsSocket, sdfsRequest.serialize().c_str(), sdfsRequest.serialize().size(), 0);
        if (sentBytes < 0)
        {
            cerr << "Error putting juice_exe to sdfs" << endl;
            exit(1);
        }
        request.message_type = JUICE_LEADER;
        sentBytes = send(leaderSocket, request.serialize().c_str(), request.serialize().size(), 0);
        if (sentBytes < 0)
        {
            cerr << "Error sending juice request to leader" << endl;
            exit(1);
        }
    }
    else if (message_type == MAPLE_LEADER)
    {
        cout << "Leader processes maple function!" << endl;
        // tell all member to do the job with exe file, also need to split the input files to everyone.
        int taskPerMachine, machineNeed;
        if (stoi(request.num_workers) < (int)Membershiplist::Map.size())
        {
            taskPerMachine = 1;
            machineNeed = stoi(request.num_workers);
        }
        else
        {
            taskPerMachine = stoi(request.num_workers) / (int)Membershiplist::Map.size();
            machineNeed = (int)Membershiplist::Map.size();
        }
        int linesPerMachine = taskPerMachine * MAPLE_INPUT_LINES;

        unordered_map<int, vector<pair<string, fpos_t>>> partitionStartPositions;

        // Directory path containing the files
        filesystem::path directoryPath(request.src_dict); // Replace this with your directory path

        for (const auto &entry : filesystem::directory_iterator(directoryPath))
        {
            if (filesystem::is_regular_file(entry))
            {
                ifstream file(entry.path());
                if (file.is_open())
                {
                    string line;
                    int lineCount = 0;
                    fpos_t startPos = 0;
                    while (getline(file, line))
                    {
                        if (lineCount % linesPerMachine == 0)
                        {
                            int partition = hashPartition(line, linesPerMachine);
                            if (partitionStartPositions.find(partition) == partitionStartPositions.end())
                            {
                                file.clear();
                                file.seekg(startPos);
                                partitionStartPositions[partition].push_back(make_pair(entry.path().filename().string(), file.tellg()));
                            }
                        }

                        ++lineCount;
                        startPos = file.tellg();
                    }
                    file.close();
                }
                else
                {
                    cerr << "Unable to open file: " << entry.path() << endl;
                }
            }
        }

        int i = 0;
        int taskCount = 0;
        for (auto machine : Membershiplist::Map)
        {
            if (i >= machineNeed)
            {
                break;
            }
            int otherServerSocket = generate_TCP_client_socket(machine, MJ_PORT);
            for (const auto &entry : partitionStartPositions[i])
            {
                request.message_type = MAPLE_WORKER;
                request.maple_filename = entry.first;
                request.maple_fileposition = entry.second;

                ssize_t sentBytes = send(otherServerSocket, request.serialize().c_str(), request.serialize().size(), 0);
                if (sentBytes == -1)
                {
                    cerr << "Error connect to maple workers" << machine << endl;
                    exit(1);
                }

                char buf[MAXDATASIZE];
                int bytesRead = recv(otherServerSocket, buf, MAXDATASIZE, 0);
                if (bytesRead <= 0)
                {
                    cerr << "No response from maple worker";
                    exit(1);
                }
                if (strcmp(buf, "Maple Success") == 0)
                {
                    taskCount++;
                    if (taskCount == partitionStartPositions[i].size())
                    {
                        sentBytes = send(new_fd, buf, MAXDATASIZE, 0);
                        if (sentBytes == -1)
                        {
                            cerr << "Error reporting success to maple query server" << machine << endl;
                            exit(1);
                        }
                    }
                }
            }
        }
    }
    else if (message_type == JUICE_LEADER)
    {
        cout << "Leader processes juice function!" << endl;

        // Count how many lines each machine should process
        int taskPerMachine, machineNeed;
        if (stoi(request.num_workers) < (int)Membershiplist::Map.size())
        {
            taskPerMachine = 1;
            machineNeed = stoi(request.num_workers);
        }
        else
        {
            taskPerMachine = stoi(request.num_workers) / (int)Membershiplist::Map.size();
            machineNeed = (int)Membershiplist::Map.size();
        }
        int linesPerMachine = taskPerMachine * MAPLE_INPUT_LINES;

        // storing partitions into an unordered map and send them to each workers
        int sdfsSocket = generate_TCP_client_socket(Membershiplist::Leader, SDFS_PORT);
        SDFSRequest sdfsRequest;
        unordered_map<int, vector<pair<string, fpos_t>>> partitions;
        for (const auto &entry : FileList::FileMetadataMap)
        {
            string prefix = request.sdfs_intermediate_filename;
            string filename = entry.first;
            if (filename.compare(0, prefix.length(), prefix) != 0)
            {
                continue;
            }
            sdfsRequest.message_type = SDFS_GET;
            sdfsRequest.sdfsfilename = filename;
            sdfsRequest.localfilename = filename;

            ifstream file(filename);
            if (file.is_open())
            {
                string line;
                int lineCount = 0;
                fpos_t startPos = 0;
                while (getline(file, line))
                {
                    if (lineCount % linesPerMachine == 0)
                    {
                        int partition = hashPartition(line, linesPerMachine);
                        if (partitions.find(partition) == partitions.end())
                        {
                            file.clear();
                            file.seekg(startPos);
                            partitions[partition].push_back(make_pair(filename, file.tellg()));
                        }
                    }

                    ++lineCount;
                    startPos = file.tellg();
                }
                file.close();
            }
            else
            {
                cerr << "Unable to open file: " << filename << endl;
            }
        }

        int i = 0;
        int taskCount = 0;
        for (auto machine : Membershiplist::Map)
        {
            if (i >= machineNeed)
            {
                break;
            }
            int otherServerSocket = generate_TCP_client_socket(machine, MJ_PORT);
            for (const auto &entry : partitions[i])
            {
                request.message_type = JUICE_WORKER;
                request.juice_filename = entry.first;
                request.juice_fileposition = entry.second;

                ssize_t sentBytes = send(otherServerSocket, request.serialize().c_str(), request.serialize().size(), 0);
                if (sentBytes == -1)
                {
                    cerr << "Error connect to juice workers" << machine << endl;
                    exit(1);
                }

                char buf[MAXDATASIZE];
                int bytesRead = recv(otherServerSocket, buf, MAXDATASIZE, 0);
                if (bytesRead <= 0)
                {
                    cerr << "No response from juice worker";
                    exit(1);
                }
                if (strcmp(buf, "Juice Success") == 0)
                {
                    taskCount++;
                    if (taskCount == partitions[i].size())
                    {
                        sentBytes = send(new_fd, buf, MAXDATASIZE, 0);
                        if (sentBytes == -1)
                        {
                            cerr << "Error reporting success to maple query server" << machine << endl;
                            exit(1);
                        }
                    }
                }
            }
            if (stoi(request.delete_input) == 1)
            {
                for (const auto &entry : FileList::FileMetadataMap)
                {
                    string filename = entry.first;
                    string prefix = request.sdfs_intermediate_filename;
                    if (filename.compare(0, prefix.length(), prefix) == 0)
                    {
                        sdfsRequest.message_type = SDFS_DELETE;
                        sdfsRequest.sdfsfilename = filename;
                        ssize_t sentBytes = send(sdfsSocket, sdfsRequest.serialize().c_str(), sdfsRequest.serialize().size(), 0);
                        if (sentBytes < 0)
                        {
                            cerr << "Error Deleting intermediate file" << endl;
                            close(sdfsSocket);
                            exit(1);
                        }
                    }
                }
            }
        }
        close(sdfsSocket);
    }
    else if (message_type == MAPLE_WORKER)
    {
        // Get the maple exe first
        SDFSRequest mapleSDFSRequest;
        mapleSDFSRequest.message_type = SDFS_GET;
        mapleSDFSRequest.sdfsfilename = "maple_exe";
        mapleSDFSRequest.localfilename = "maple_exe.py";
        int mapleSDFSSocket = generate_TCP_client_socket(Membershiplist::Leader, SDFS_PORT);
        ssize_t sentBytes = send(mapleSDFSSocket, mapleSDFSRequest.serialize().c_str(), mapleSDFSRequest.serialize().size(), 0);
        if (sentBytes == -1)
        {
            cerr << "Error getting maple exe" << endl;
            exit(1);
        }

        // Do the maple phase
        if (request.src_dict.back() != '/')
        {
            request.src_dict += '/';
        }

        stringstream ss;
        ss << static_cast<long long>(request.maple_fileposition);
        string maple_exe_call = "python3 maple_exe.py " + request.src_dict + request.maple_filename + " " + ss.str() + " " + request.sdfs_intermediate_filename;
        array<char, 128> buffer;
        set<string> outputSet;
        shared_ptr<FILE> pipe(popen(maple_exe_call.c_str(), "r"), pclose);

        if (!pipe)
        {
            cerr << "Error running maple exe" << endl;
            exit(1);
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            outputSet.insert(buffer.data());
        }

        for (auto outputfile : outputSet)
        {
            mapleSDFSRequest.localfilename = outputfile;
            mapleSDFSRequest.sdfsfilename = outputfile;
            mapleSDFSRequest.message_type = SDFS_PUT;

            sentBytes = send(mapleSDFSSocket, mapleSDFSRequest.serialize().c_str(), mapleSDFSRequest.serialize().size(), 0);
            if (sentBytes == -1)
            {
                cerr << "Error putting maple intermediate file" << endl;
                exit(1);
            }
        }
        string resultString = "Maple Success";
        sentBytes = send(new_fd, resultString.c_str(), resultString.length(), 0);
        if (sentBytes == -1)
        {
            cerr << "Error reporting maple success" << endl;
            exit(1);
        }
    }
    else if (message_type == JUICE_WORKER)
    {
        // Get the maple exe first
        SDFSRequest juiceSDFSRequest;
        juiceSDFSRequest.message_type = SDFS_GET;
        juiceSDFSRequest.sdfsfilename = "juice_exe";
        juiceSDFSRequest.localfilename = "juice_exe.py";
        int juiceSDFSSocket = generate_TCP_client_socket(Membershiplist::Leader, SDFS_PORT);
        ssize_t sentBytes = send(juiceSDFSSocket, juiceSDFSRequest.serialize().c_str(), juiceSDFSRequest.serialize().size(), 0);
        if (sentBytes == -1)
        {
            cerr << "Error getting juice exe" << endl;
            exit(1);
        }

        // Do the juice phase
        stringstream ss;
        ss << static_cast<long long>(request.juice_fileposition);
        string juice_exe_call = "python3 juice_exe.py " + request.juice_filename + " " + ss.str() + " " + to_string(MAPLE_INPUT_LINES) + " " + request.sdfs_intermediate_filename + " " + request.dest_file;
        int result = system(juice_exe_call.c_str());
        if (result != 0)
        {
            cerr << "Error running juice exe" << endl;
            exit(1);
        }
    }
    close(new_fd);
}

void MJCommand(int port)
{
    int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
    char s[INET_ADDRSTRLEN];
    socklen_t sin_size;

    struct sockaddr_storage their_addr; // connector's address information
    sockfd = generate_TCP_SDFS_serversocket(to_string(MJ_PORT).c_str());
    std::cout << "server for MJ_commad: waiting for connections..." << std::endl;

    while (1)
    {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("accept");
            exit(1);
        }
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        std::cout << "MJ server: got connection from " << s << std::endl;
        std::thread clientThread3(handleMJ, new_fd, port);
        clientThread3.detach();
    }
}

int main(int argc, char const *argv[])
{
    int port = (argc >= 2) ? atoi(argv[1]) : DEFAULT_PORT;

    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    std::string currentHostname(hostname);
    Membershiplist::CreateList(port);
    FileList::CreateContext();
    thread tcp_grep(grep_function);
    thread tcp_MembershipList(MembershipListCommand, port);
    thread udp_receivembrlist(ReceiveGossip, currentHostname, port);
    thread udp_sendmbrlist(SendGossip, currentHostname);
    thread clockHeartInc(Inc, currentHostname);
    thread tcp_SDFS(SDFSCommand, port);
    thread FailureRecovery_(FailureRecovery, currentHostname, port);
    thread MapleJuice(MJCommand, port);
    tcp_grep.join();
    tcp_MembershipList.join();
    udp_sendmbrlist.join();
    udp_receivembrlist.join();
    clockHeartInc.join();
    tcp_SDFS.join();
    FailureRecovery_.join();
    MapleJuice.join();
    return 0;
}
