#ifndef SDFSPREQUEST_H
#define SDFSPREQUEST_H
#include <string>
#include <cstring>
#include <vector>
#include <arpa/inet.h>
#include <queue>
#include <set>
#include <mutex>
#include <iostream>
#include <unordered_map>
#include "Message.h"

struct SDFSRequest : public Message
{
    string messageString;
    MESSAGE_TYPE message_type;
    string hostname;
    string port;
    string localfilename;
    string sdfsfilename;
    int version;

    bool operator==(const SDFSRequest &other) const
    {
        return (message_type == other.message_type) && (hostname == other.hostname) && (sdfsfilename == other.sdfsfilename);
    }
    bool operator!=(const SDFSRequest &other) const
    {
        return !(*this == other);
    }

    // SDFSRequest(MESSAGE_TYPE message_type, string hostname, string port);
    SDFSRequest();
    SDFSRequest(MESSAGE_TYPE message_type);
    SDFSRequest(MESSAGE_TYPE message_type, string sdfsfilename, int version);
    SDFSRequest(MESSAGE_TYPE message_type, string hostname, string port = "4480", string localfilename = "", string sdfsfilename = "", int version = 0);
    virtual ~SDFSRequest(){};
    const string serialize();

    static SDFSRequest deserialize(string &request_string);
    // static void deserialize(string &request_string);
};

struct SDFSReturnMsg
{
    vector<string> replicaMembers;
    bool hasFile;
    int timestamp;

    void fixReplicaServerNames();
};

vector<char> serializeSDFSResponse(const SDFSReturnMsg &response);
SDFSReturnMsg deserializeSDFSResponse(const char *buffer);
vector<char> serializeSDFSRequest(const SDFSRequest &request);
SDFSRequest deserializeSDFSRequest(const char *buffer);

class FileRequest
{
public:
    static unordered_map<string, queue<SDFSRequest>> putQueue;
    static unordered_map<string, queue<SDFSRequest>> getQueue;
    static unordered_map<string, queue<pair<int, SDFSRequest>>> allQueue;
    static unordered_map<string, int> getAccu;
    static unordered_map<string, int> putAccu;
    static unordered_map<string, int> getCount;
    static unordered_map<string, int> putCount;
    static mutex QuqueMutex;
    static void processQueue(int version, SDFSRequest request);
    static void FinishQueue(SDFSRequest request);
    static void reorderPutToFront(string sdfsfilename);
    static void reorderGetToFront(string sdfsfilename);
    static void popQueue(SDFSRequest request);
    static void removeRequestFromAllQueue(SDFSRequest request);
    // static bool elementInQueue(string sdfsfilename, MESSAGE_TYPE message_type);
};
#endif