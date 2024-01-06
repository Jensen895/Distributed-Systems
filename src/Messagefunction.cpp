#include <iostream>
#include <sstream>
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <future>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <random>
#include <arpa/inet.h>
#include <chrono>
#include <netdb.h>

#include "RequestMessage.h"
#include "ResponseMessage.h"
#include "unit_test.h"
#include "MembershipRequest.h"
#include "Messagefunction.h"
#include "Message.h"
#include "MemberIdRequest.h"
#include "MemberIdResponse.h"
#include "SDFSRequest.h"
#include "Membershiplist.h"
#include "SDFS_file_transfer.h"
#include "SDFS_filelist.h"
#define PORT 3491        // the port client will be connecting to
#define MAXDATASIZE 4096 // max number of bytes we can get at once
#define PORT_MEMBERSHIP 3481
#define SDFS_PORT 4481
#define MJ_PORT 5481
using namespace std;

struct ServerResult
{
    string hostname;
    string address;
    ResponseMessage response;
    bool error;
    string error_message;
};

int generate_client_socket(string serverNames, int port)
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
    cout << "create socket in client : " << serverNames << ", turn to the ip:" << serverAddress << endl;
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1)
    {
        cerr << "Error creating client socket for " << serverAddress << ":" << port << endl;
        // serverResult.error_message = "Error creating client socket for " + serverAddress + ":" + to_string(port);
        // serverResult.error = true;
        exit(1);
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
        exit(1);
    }

    if (connect(clientSocket, reinterpret_cast<struct sockaddr *>(&serverAddressInfo), sizeof(serverAddressInfo)) == -1)
    {
        cerr << "Error connecting to " << serverAddress << ":" << port << endl;
        // serverResult.error_message = "Error connecting to " + serverAddress + ":" + to_string(port);
        // serverResult.error = true;
        // results.push_back(serverResult);
        close(clientSocket);
        exit(1);
    }
    return clientSocket;
}

void sendGrepCommand(const string &hostname, int port, const RequestMessage &req_message, vector<ServerResult> &results)
{
    struct addrinfo hints;
    struct addrinfo *addresult;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(hostname.c_str(), NULL, &hints, &addresult);
    if (status != 0)
    {
        cerr << "getaddrinfo error: " << gai_strerror(status) << endl;
        return;
    }
    struct sockaddr_in *ipv4_addr = (struct sockaddr_in *)(addresult->ai_addr);
    char serveraddress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(ipv4_addr->sin_addr), serveraddress, INET_ADDRSTRLEN);

    string serverAddress(serveraddress);
    freeaddrinfo(addresult);
    ServerResult serverResult;
    serverResult.address = serverAddress;
    serverResult.hostname = hostname;
    // cout << "create socket in client : " << hostname << ", turn to the ip:" << serverAddress << endl;
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1)
    {
        // cerr << "Error creating client socket for " << serverAddress << ":" << port << endl;
        serverResult.error_message = "Error creating client socket for " + serverAddress + ":" + to_string(port);
        serverResult.error = true;
        results.push_back(serverResult);
        return;
    }

    sockaddr_in serverAddressInfo;
    serverAddressInfo.sin_family = AF_INET;
    serverAddressInfo.sin_port = htons(port);
    if (inet_pton(AF_INET, serverAddress.c_str(), &(serverAddressInfo.sin_addr)) <= 0)
    {
        // cerr << "Invalid server address: " << serverAddress << endl;
        serverResult.error_message = "Invalid server address: " + serverAddress;
        serverResult.error = true;
        results.push_back(serverResult);
        close(clientSocket);
        return;
    }

    if (connect(clientSocket, reinterpret_cast<struct sockaddr *>(&serverAddressInfo), sizeof(serverAddressInfo)) == -1)
    {
        // cerr << "Error connecting to " << serverAddress << ":" << port << endl;
        serverResult.error_message = "Error connecting to " + serverAddress + ":" + to_string(port);
        serverResult.error = true;
        results.push_back(serverResult);
        close(clientSocket);
        return;
    }

    RequestMessage req_message_copy = req_message;
    const string serializedRequest = req_message_copy.serialize();

    ssize_t sentBytes = send(clientSocket, serializedRequest.c_str(), serializedRequest.size(), 0);
    if (sentBytes == -1)
    {
        // cerr << "Error sending command to " << serverAddress << ":" << port << endl;
        serverResult.error_message = "Error sending command to " + serverAddress + ":" + to_string(port);
        serverResult.error = true;
        results.push_back(serverResult);
        close(clientSocket);
        return;
    }

    char buffer_receive[1024];
    ssize_t bytesRead = recv(clientSocket, buffer_receive, sizeof(buffer_receive), 0);
    if (bytesRead <= 0)
    {
        // cerr << "Error receiving result from " << serverAddress << ":" << port << endl;
        serverResult.error_message = "Error receiving result from " + serverAddress + ":" + to_string(port);
        serverResult.error = true;
    }
    else
    {
        buffer_receive[bytesRead] = '\0';
        ResponseMessage receivedResponse = ResponseMessage::deserialize(buffer_receive);
        serverResult.response = receivedResponse;
        serverResult.error = false;
    }

    results.push_back(serverResult);
    close(clientSocket);
}

void QueryFunction(Message *request) // MP1
{
    RequestMessage *reqmessage = dynamic_cast<RequestMessage *>(request);
    RequestMessage req_message = *reqmessage;
    vector<string> serverNames = {
        "localhost",
        // "fa23-cs425-4101.cs.illinois.edu",
        // "fa23-cs425-4102.cs.illinois.edu",
        // "fa23-cs425-4103.cs.illinois.edu",
        // "fa23-cs425-4104.cs.illinois.edu",
        // "fa23-cs425-4105.cs.illinois.edu",
        // "fa23-cs425-4106.cs.illinois.edu",
        // "fa23-cs425-4107.cs.illinois.edu",
        // "fa23-cs425-4108.cs.illinois.edu",
        // "fa23-cs425-4109.cs.illinois.edu",
        // "fa23-cs425-4110.cs.illinois.edu"
    };
    vector<thread> threads;
    vector<ServerResult> results;
    int port = 3490;
    for (const string &serverName : serverNames)
    {
        threads.push_back(thread(sendGrepCommand, serverName, PORT, req_message, ref(results)));
    }

    // Join all the threads
    for (thread &t : threads)
    {
        t.join();
    }
    if (req_message.unit_test)
    {
        bool allpass = true;
        for (const ServerResult &result : results)
        {
            UnitTest temp;
            int unittest_result = result.response.unittest_result;
            bool tempbool = temp.unit_test(unittest_result);
            if (!tempbool)
            {
                cout << result.hostname << " failed!" << endl;
            }
            allpass = allpass && tempbool;
        }
        if (allpass)
        {
            cout << "All servers are good!!" << endl;
        }
    }
    else
    {
        // Print the results
        int totalCount = 0;

        for (const ServerResult &result : results)
        {

            if (result.error)
            {
                cout << result.error_message << endl;
                continue;
            }
            else
            {
                totalCount += result.response.matching_line_count;
                cout << "Result from " << result.hostname << ":\n log file from:"
                     << result.response.in_file << "\n matching count:"
                     << result.response.matching_line_count << endl;
            }
            cout << "================================" << endl;
        }
        cout << "Total Line Count: " << totalCount << endl;
    }
}

void MembershipFunction_Join(Message *request) // MP2
{
    const string request_string = request->serialize();
    string serverName = "fa23-cs425-4101.cs.illinois.edu"; // the server that client will send to
    int clientSocket = generate_client_socket(serverName, PORT_MEMBERSHIP);

    ssize_t sentBytes = send(clientSocket, request_string.c_str(), request_string.size(), 0);
    if (sentBytes == -1)
    {
        cerr << "Error sending command to server" << endl;
        close(clientSocket);
        return;
    }
    char buffer_receive[MAXDATASIZE];
    sentBytes = recv(clientSocket, buffer_receive, sizeof(buffer_receive), 0);
    if (sentBytes <= 0)
    {
        cerr << "Error receiving result" << endl;
    }
    else
    {
        buffer_receive[sentBytes] = '\0';
    }
    MemberIdResponse receivedResponse = MemberIdResponse::deserialize(buffer_receive);
    cout << "Message: " << receivedResponse.message_type << " ID:" << receivedResponse.memberId << endl;

    MemberIdRequest Idrequest = MemberIdRequest(receivedResponse.message_type, receivedResponse.memberId, receivedResponse.membershiplist);
    clientSocket = generate_client_socket("localhost", PORT_MEMBERSHIP);
    cout << "Receive server response successfully!" << endl;
    sentBytes = send(clientSocket, Idrequest.serialize().c_str(), Idrequest.serialize().size(), 0);
    if (sentBytes == -1)
    {
        cerr << "Error sending command to server" << endl;
        close(clientSocket);
        return;
    }
    close(clientSocket);
}

void MembershipFunction_Leave(Message *request) // MP2
{
    const string request_string = request->serialize();
    string serverName = "localhost"; // the server that client will send to

    int clientSocket = generate_client_socket(serverName, PORT_MEMBERSHIP);

    ssize_t sentBytes = send(clientSocket, request_string.c_str(), request_string.size(), 0);
    if (sentBytes == -1)
    {
        cerr << "Error sending command to server" << endl;
        close(clientSocket);
        return;
    }
}

void MembershipFunction_PrintList(Message *request) // MP2
{
    const string request_string = request->serialize();
    string serverName = "localhost"; // the server that client will send to

    int clientSocket = generate_client_socket(serverName, PORT_MEMBERSHIP);

    ssize_t sentBytes = send(clientSocket, request_string.c_str(), request_string.size(), 0);
    if (sentBytes == -1)
    {
        cerr << "Error sending command to server" << endl;
        close(clientSocket);
        return;
    }
}

void MembershipFunction_EnableSuspicion(Message *request)
{
    const string request_string = request->serialize();
    string serverName = "localhost";
    int clientSocket = generate_client_socket(serverName, PORT_MEMBERSHIP);
    ssize_t sentBytes = send(clientSocket, request_string.c_str(), request_string.size(), 0);
    if (sentBytes == -1)
    {
        cerr << "Error sending command to server" << endl;
        close(clientSocket);
        return;
    }
}

void SDFSFunction_PUT(Message *request) // MP3
{
    SDFSRequest sdfsRequest;
    const string request_string = request->serialize();
    vector<string> tokens = Message::parse_tokens(request_string);
    MESSAGE_TYPE message = (MESSAGE_TYPE)stoi(tokens[0]);
    string hostname = tokens[1];
    string port = tokens[2];
    string localfilename = tokens[3];
    string sdfsfilename = tokens[4];

    string serverName = "localhost";

    char buffer[SOCKET_BUFFER_SIZE] = {0};

    int clientSocket = generate_client_socket(serverName, SDFS_PORT);
    ssize_t sentBytes = send(clientSocket, request_string.c_str(), request_string.size(), 0);
    // ssize_t sentBytes = send(clientSocket, serializedData.data(), serializedData.size(), 0);
    if (sentBytes == -1)
    {
        cerr << "Error sending command to server" << endl;
        close(clientSocket);
        return;
    }

    recv(clientSocket, buffer, SOCKET_BUFFER_SIZE, 0);
    string resp;
    stringstream strstream(buffer);
    strstream >> resp;
    if (START_RECV_FILE == resp)
    {
        SDFSFileTransfer::send_file(clientSocket, localfilename);
    }

    recv(clientSocket, buffer, SOCKET_BUFFER_SIZE, 0);
    stringstream strstream2(buffer);
    strstream2 >> resp;
    if (FINISH_TRANSFER == resp)
    {
        cout << "Write Finish" << endl;
    }
    else if (SOMETHING_WRONG == resp)
    {
        cout << "something wrong, please try again" << endl;
    }
}

void SDFSFunction_GET(Message *request) // MP3
{
    const string request_string = request->serialize();
    string serverName = "localhost";
    char buffer[SOCKET_BUFFER_SIZE] = {0};
    int clientSocket = generate_client_socket(serverName, SDFS_PORT);
    ssize_t sentBytes = send(clientSocket, request_string.c_str(), request_string.size(), 0);
    if (sentBytes == -1)
    {
        cerr << "Error sending command to server" << endl;
        close(clientSocket);
        return;
    }
    recv(clientSocket, buffer, SOCKET_BUFFER_SIZE, 0);
    string resp;
    stringstream strstream2(buffer);
    strstream2 >> resp;
    if (FINISH_TRANSFER == resp)
    {
        cout << "Read Finish" << endl;
    }
    else if (SOMETHING_WRONG == resp)
    {
        cout << "something wrong, please try again" << endl;
    }
    else if (NO_FILE_EXIST == resp)
    {
        cout << "SDFS file doesn't exist!" << endl;
    }
}

void SDFSFunction_DELETE(Message *request) // MP3
{
    SDFSRequest sdfsRequest;
    const string request_string = request->serialize();
    vector<string> tokens = Message::parse_tokens(request_string);
    sdfsRequest.message_type = (MESSAGE_TYPE)stoi(tokens[0]);
    sdfsRequest.hostname = tokens[1];
    sdfsRequest.port = "0";
    sdfsRequest.sdfsfilename = tokens[3];
    string serverName = sdfsRequest.hostname;

    char buffer[SOCKET_BUFFER_SIZE] = {0};

    int clientSocket = generate_client_socket(serverName, SDFS_PORT);
    ssize_t sentBytes = send(clientSocket, request_string.c_str(), request_string.size(), 0);
    if (sentBytes == -1)
    {
        cerr << "Error sending command to server" << endl;
        close(clientSocket);
        return;
    }

    char buf[MAXDATASIZE];
    int bytesRead = recv(clientSocket, buf, MAXDATASIZE - 1, 0);
    cout << buf << endl;
}

void SDFSFunction_LS(Message *request) // MP3
{
    const string request_string = request->serialize();
    string serverName = "localhost";
    int clientSocket = generate_client_socket(serverName, SDFS_PORT);
    ssize_t sentBytes = send(clientSocket, request_string.c_str(), request_string.size(), 0);
    if (sentBytes == -1)
    {
        cerr << "Error sending command to server" << endl;
        close(clientSocket);
        return;
    }
}

void SDFSFunction_STORE(Message *request) // MP3
{
    const string request_string = request->serialize();
    string serverName = "localhost";
    int clientSocket = generate_client_socket(serverName, SDFS_PORT);
    ssize_t sentBytes = send(clientSocket, request_string.c_str(), request_string.size(), 0);
    // ssize_t sentBytes = send(clientSocket, serializedData.data(), serializedData.size(), 0);
    if (sentBytes == -1)
    {
        cerr << "Error sending command to server" << endl;
        close(clientSocket);
        return;
    }
}

void SDFSFunction_PRINT_GLOBAL(Message *request)
{
    const string request_string = request->serialize();
    string serverName = "localhost";

    int clientSocket = generate_client_socket(serverName, SDFS_PORT);
    ssize_t sentBytes = send(clientSocket, request_string.c_str(), request_string.size(), 0);
    // ssize_t sentBytes = send(clientSocket, serializedData.data(), serializedData.size(), 0);
    if (sentBytes == -1)
    {
        cerr << "Error sending command to server" << endl;
        close(clientSocket);
        return;
    }
    // char buffer[SOCKET_BUFFER_SIZE] = {0};
    // recv(clientSocket, buffer, SOCKET_BUFFER_SIZE, 0);
    // string resp;
    // stringstream strstream(buffer);
    // strstream >> resp;
    // FileList::PrintFileListinClient(resp);
}

void MAPLEFunction(Message *request) // just send the command to the server side
{
    const string request_string = request->serialize();
    string serverName = "localhost";
    int clientSocket = generate_client_socket(serverName, MJ_PORT);
    ssize_t sentBytes = send(clientSocket, request_string.c_str(), request_string.size(), 0);
    if (sentBytes == -1)
    {
        cerr << "Error sending command to server" << endl;
        close(clientSocket);
        return;
    }
    char buffer[MAXDATASIZE];
    int bytesRead = recv(clientSocket, buffer, MAXDATASIZE, 0);
    if (bytesRead <= 0){
        cerr << "Error receiving results from server" << endl;
        exit(1);
    }
    cout << buffer << endl;
}
void JUICEFunction(Message *request) // just send the command to the server side
{
    const string request_string = request->serialize();
    string serverName = "localhost";
    int clientSocket = generate_client_socket(serverName, MJ_PORT);
    ssize_t sentBytes = send(clientSocket, request_string.c_str(), request_string.size(), 0);
    if (sentBytes == -1)
    {
        cerr << "Error sending command to server" << endl;
        close(clientSocket);
        return;
    }
}

void Messagefunction::implement_request(Message *request)
{
    const string request_string = request->serialize();
    MESSAGE_TYPE type = (MESSAGE_TYPE)stoi(request_string.substr(0, request_string.find(CUSTOM_DELIMITER)));

    if (type == QUERY_LOG)
    {
        QueryFunction(request);
    }

    else if (type == JOIN)
    {
        MembershipFunction_Join(request);
    }

    else if (type == LEAVE)
    {
        MembershipFunction_Leave(request);
    }

    else if (type == MBRLIST)
    {
        MembershipFunction_PrintList(request);
    }
    else if (type == SUSPICION)
    {
        MembershipFunction_EnableSuspicion(request);
    }
    else if (type == SDFS_PUT)
    {
        SDFSFunction_PUT(request);
    }
    else if (type == SDFS_GET)
    {
        SDFSFunction_GET(request);
    }
    else if (type == SDFS_DELETE)
    {
        SDFSFunction_DELETE(request);
    }
    else if (type == SDFS_LS)
    {
        SDFSFunction_LS(request);
    }
    else if (type == SDFS_STORE)
    {
        SDFSFunction_STORE(request);
    }
    else if (type == SDFS_PRINT_GLOBAL)
    {
        SDFSFunction_PRINT_GLOBAL(request);
    }
    else if (type == MAPLE)
    {
        MAPLEFunction(request);
    }
    else if (type == JUICE)
    {
        JUICEFunction(request);
    }
}