#include "SDFSRequest.h"
#include "SDFS_filelist.h"
#include <stdexcept>
#include <iostream>
#include <vector>
#include <sstream>
#include <cstring>
#include <set>
#include <thread>
using namespace std;

unordered_map<string, queue<SDFSRequest>> FileRequest::putQueue; // sdfsfilename, request
unordered_map<string, queue<SDFSRequest>> FileRequest::getQueue;
unordered_map<string, queue<pair<int, SDFSRequest>>> FileRequest::allQueue;

unordered_map<string, int> FileRequest::getCount;
unordered_map<string, int> FileRequest::putCount;
unordered_map<string, int> FileRequest::getAccu;
unordered_map<string, int> FileRequest::putAccu;

SDFSRequest::SDFSRequest()
{
}

mutex FileRequest::QuqueMutex;

SDFSRequest::SDFSRequest(MESSAGE_TYPE message_type)
    : message_type(message_type)
{
}

SDFSRequest::SDFSRequest(MESSAGE_TYPE message_type, string sdfsfilename, int version)
    : message_type(message_type), sdfsfilename(sdfsfilename), version(version)
{
}

SDFSRequest::SDFSRequest(MESSAGE_TYPE message_type, string hostname, string port, string localfilename, string sdfsfilename, int version)
    : message_type(message_type), hostname(hostname), port(port), localfilename(localfilename), sdfsfilename(sdfsfilename), version(version)
{
}

// TODO: Modify to struct data
const string SDFSRequest::serialize()
{
    // Serialize the struct into a string
    string serialized = to_string(message_type) + CUSTOM_DELIMITER + hostname + CUSTOM_DELIMITER + port + CUSTOM_DELIMITER + localfilename + CUSTOM_DELIMITER + sdfsfilename + CUSTOM_DELIMITER + to_string(version);
    return serialized;
}

SDFSRequest SDFSRequest::deserialize(string &request_string)
{
    vector<string> tokens = Message::parse_tokens(request_string);
    MESSAGE_TYPE message_type = (MESSAGE_TYPE)stoi(tokens[0]);
    string hostname = tokens[1];
    string port = tokens[2];
    string localfilename = tokens[3];
    string sdfsfilename = tokens[4];
    int version = stoi(tokens[5]);
    return SDFSRequest(message_type, hostname, port, localfilename, sdfsfilename, version);
}

void SDFSReturnMsg::fixReplicaServerNames()
{
    vector<string> temp;
    temp = replicaMembers;
    replicaMembers.clear();

    for (int i = 0; i < temp.size(); i++)
    {
        size_t firstAt = temp[i].find('@'); // Find the first '@'
        if (firstAt != std::string::npos)
        {
            size_t secondAt = temp[i].find('@', firstAt + 1); // Find the second '@' starting from the position after the first '@'

            if (secondAt != std::string::npos)
            {
                std::string substring = temp[i].substr(firstAt + 1, secondAt - firstAt - 1);
                replicaMembers.push_back(substring);
            }
            else
            {
                std::cout << "Second '@' not found." << std::endl;
            }
        }
        else
        {
            std::cout << "First '@' not found." << std::endl;
        }
    }
}

vector<char> serializeSDFSRequest(const SDFSRequest &request)
{
    vector<char> data;
    // Serialize message_type as an integer
    int message_type = static_cast<int>(request.message_type);
    data.insert(data.end(), reinterpret_cast<char *>(&message_type), reinterpret_cast<char *>(&message_type) + sizeof(int));

    // Serialize hostname
    data.insert(data.end(), request.hostname.begin(), request.hostname.end());
    data.push_back('\0'); // Null-terminate the hostname

    // Serialize port (assuming it's a 32-bit integer)
    data.insert(data.end(), request.port.begin(), request.port.end());
    data.push_back('\0'); // Null-terminate the localfilename

    // Serialize localfilename
    data.insert(data.end(), request.localfilename.begin(), request.localfilename.end());
    data.push_back('\0'); // Null-terminate the localfilename

    // Serialize sdfsfilename
    data.insert(data.end(), request.sdfsfilename.begin(), request.sdfsfilename.end());
    data.push_back('\0'); // Null-terminate the sdfsfilename

    return data;
}

SDFSReturnMsg deserializeSDFSResponse(const char *buffer)
{
    SDFSReturnMsg msg;

    int timestampNetworkOrder;
    memcpy(&timestampNetworkOrder, buffer, sizeof(int));
    msg.timestamp = ntohl(timestampNetworkOrder);
    buffer += sizeof(int);

    int hasFileNetworkOrder;
    memcpy(&hasFileNetworkOrder, buffer, sizeof(int));
    msg.hasFile = ntohl(hasFileNetworkOrder) != 0;
    buffer += sizeof(int);

    // Deserialize the vector of strings
    int numStrings;
    memcpy(&numStrings, buffer, sizeof(int));
    numStrings = ntohl(numStrings);

    buffer += sizeof(int);

    for (int i = 0; i < numStrings; i++)
    {
        int strLenFromBuffer;
        memcpy(&strLenFromBuffer, buffer, sizeof(int));
        int strLength = ntohl(strLenFromBuffer);

        buffer += sizeof(int);

        string str;
        str.assign(buffer, strLength);

        msg.replicaMembers.push_back(str);
        buffer += strLength;
    }
    return msg;
}

vector<char> serializeSDFSResponse(const SDFSReturnMsg &response)
{
    vector<char> data;

    int timestampNetworkOrder = htonl(response.timestamp);
    data.insert(data.end(), reinterpret_cast<char *>(&timestampNetworkOrder), reinterpret_cast<char *>(&timestampNetworkOrder) + sizeof(int));

    int hasFileNetworkOrder = htonl(static_cast<int>(response.hasFile));
    data.insert(data.end(), reinterpret_cast<char *>(&hasFileNetworkOrder), reinterpret_cast<char *>(&hasFileNetworkOrder) + sizeof(int));

    int vectorLenthNetworkOrder = htonl(static_cast<int>(response.replicaMembers.size()));
    data.insert(data.end(), reinterpret_cast<char *>(&vectorLenthNetworkOrder), reinterpret_cast<char *>(&vectorLenthNetworkOrder) + sizeof(int));

    for (const string &member : response.replicaMembers)
    {
        int strlengthNetworkOrder = htonl(static_cast<int>(member.length()));
        data.insert(data.end(), reinterpret_cast<char *>(&strlengthNetworkOrder), reinterpret_cast<char *>(&strlengthNetworkOrder) + sizeof(int));
        data.insert(data.end(), member.begin(), member.end());
    }

    return data;
}

SDFSRequest deserializeSDFSRequest(const char *buffer)
{
    SDFSRequest request;
    int offset = 0;
    // Deserialize message_type (assuming it's a 32-bit integer)
    int message_type_value;
    memcpy(&message_type_value, buffer + offset, sizeof(int));
    request.message_type = static_cast<MESSAGE_TYPE>(message_type_value);
    offset += sizeof(MESSAGE_TYPE);

    // Deserialize hostname (assuming it's a null-terminated string)
    request.hostname = std::string(buffer + offset);
    offset += request.hostname.length() + 1; // +1 for the null terminator

    // Deserialize port (assuming it's a 32-bit integer)
    request.port = std::string(buffer + offset);
    offset += request.port.length() + 1;

    // Deserialize localfilename (assuming it's a null-terminated string)
    request.localfilename = std::string(buffer + offset);
    offset += request.localfilename.length() + 1; // +1 for the null terminator

    // Deserialize sdfsfilename (assuming it's a null-terminated string)
    request.sdfsfilename = std::string(buffer + offset);
    offset += request.localfilename.length() + 1;

    return request;
}

// void FileRequest::removeRequestFromAllQueue(SDFSRequest request)
// {
//     queue<SDFSRequest> tempQueue;
//     while (!allQueue.empty())
//     {
//         SDFSRequest temp = allQueue.front();
//         allQueue.pop();
//         if (temp != request)
//         {
//             tempQueue.push(temp);
//         }
//     }
//     allQueue.swap(tempQueue);
// }
void print_queue(queue<pair<int, SDFSRequest>> &sdfsQueue)
{
    cout << "================ " << endl;
    while (!sdfsQueue.empty())
    {
        int index = sdfsQueue.front().first;
        SDFSRequest request = sdfsQueue.front().second;
        sdfsQueue.pop();
        string type;
        if (request.message_type == SDFS_GET)
            type = "Get";
        else if (request.message_type == SDFS_PUT)
            type = "Put";
        std::cout << "version " << index << ": " << type << " file " << request.sdfsfilename << std::endl;
    }
    cout << "================ " << endl;
}

// bool FileRequest::elementInQueue(string sdfsfilename, MESSAGE_TYPE message_type)
// {
//     queue<pair<int, SDFSRequest>> tempQueue = FileRequest::allQueue[sdfsfilename];
//     while (!tempQueue.empty())
//     {
//         if (tempQueue.front().second.message_type == message_type)
//         {
//             return true;
//         }
//         tempQueue.pop();
//     }
//     return false;
// }

void FileRequest::processQueue(int version, SDFSRequest request)
{
    FileRequest::QuqueMutex.lock();
    pair<int, SDFSRequest> version_request = make_pair(version, request);
    FileRequest::allQueue[request.sdfsfilename].push(version_request); // put the sdfsfile-request into quque no matter what.
    FileRequest::QuqueMutex.unlock();

    cout << "version: " << version << "putCount " << FileRequest::putCount[request.sdfsfilename] << endl;
    // when a put or get ready to execute, check the getAccu and putAccu, if one of them == 4, reorder the allQueue
    // for put
    if (getAccu[request.sdfsfilename] == 4) // next run has to be put
    {
        cout << "Now getAccu is 4, has to reorder the queue." << endl;
        FileRequest::reorderPutToFront(request.sdfsfilename);
    }
    else if (putAccu[request.sdfsfilename] == 4) // next run has to be get
    {
        cout << "Now putAccu is 4, has to reorder the queue." << endl;
        FileRequest::reorderGetToFront(request.sdfsfilename);
    }

    if (FileRequest::getCount[request.sdfsfilename] == 0 && FileRequest::putCount[request.sdfsfilename] == 0 && FileRequest::allQueue.empty()) // no one is processing this sdfs file
    {
        if (request.message_type == SDFS_PUT_LEADER && !FileList::Recoverstart)
        {
            FileRequest::QuqueMutex.lock();
            FileRequest::putCount[request.sdfsfilename]++;
            FileRequest::getAccu[request.sdfsfilename] = 0;
            // if (FileRequest::elementInQueue(request.sdfsfilename, SDFS_GET))
            FileRequest::putAccu[request.sdfsfilename]++;
            FileRequest::allQueue[request.sdfsfilename].pop();
            FileRequest::QuqueMutex.unlock();
        }
        else if (request.message_type == SDFS_GET_LEADER)
        {
            FileRequest::QuqueMutex.lock();
            FileRequest::getCount[request.sdfsfilename]++;
            FileRequest::putAccu[request.sdfsfilename] = 0;
            // if (FileRequest::elementInQueue(request.sdfsfilename, SDFS_PUT))
            FileRequest::getAccu[request.sdfsfilename]++;
            allQueue[request.sdfsfilename].pop();
            FileRequest::QuqueMutex.unlock();
        }
    }
    else if (getCount[request.sdfsfilename] < 2 && putCount[request.sdfsfilename] == 0 && request.message_type == SDFS_GET_LEADER && allQueue.empty()) // only one get is processing this sdfs file, get can execute but put cannot
    {
        FileRequest::QuqueMutex.lock();
        FileRequest::getCount[request.sdfsfilename]++;
        // if (FileRequest::elementInQueue(request.sdfsfilename, SDFS_PUT))
        FileRequest::getAccu[request.sdfsfilename]++;
        FileRequest::putAccu[request.sdfsfilename] = 0;
        FileRequest::allQueue[request.sdfsfilename].pop();
        FileRequest::QuqueMutex.unlock();
    }
    else // one put is processing this sdfs file or the queue is not empty, everone has to wait (should consider consequence get and put)
    {
        if (request.message_type == SDFS_PUT_LEADER)
        {
            // FileRequest::QuqueMutex.lock();
            int front_version = FileRequest::allQueue[request.sdfsfilename].front().first;
            // FileRequest::QuqueMutex.unlock();
            while (!(FileRequest::getCount[request.sdfsfilename] == 0 && FileRequest::putCount[request.sdfsfilename] == 0 && front_version == version))
            {
                cout << "put front version:" << front_version << "my version: " << version << "getCount:" << FileRequest::getCount[request.sdfsfilename] << "putCount:" << FileRequest::putCount[request.sdfsfilename] << " in the waiting line....." << endl;
                // FileRequest::QuqueMutex.lock();
                front_version = FileRequest::allQueue[request.sdfsfilename].front().first;
                // FileRequest::QuqueMutex.unlock();
                this_thread::sleep_for(std::chrono::seconds(1));
            }
            // if jump out the while loop, that means no one is get or put on this sdfs file!
            FileRequest::QuqueMutex.lock();
            FileRequest::putCount[request.sdfsfilename]++;
            // if (FileRequest::elementInQueue(request.sdfsfilename, SDFS_GET))
            FileRequest::putAccu[request.sdfsfilename]++;
            FileRequest::getAccu[request.sdfsfilename] = 0;
            FileRequest::allQueue[request.sdfsfilename].pop();
            FileRequest::QuqueMutex.unlock();
        }
        else if (request.message_type == SDFS_GET_LEADER)
        {
            // FileRequest::QuqueMutex.lock();
            int front_version = FileRequest::allQueue[request.sdfsfilename].front().first;
            // FileRequest::QuqueMutex.unlock();
            while (!(getCount[request.sdfsfilename] < 2 && putCount[request.sdfsfilename] == 0 && front_version == version && getAccu[request.sdfsfilename] != 4))
            {
                cout << "put front version:" << front_version << "my version: " << version << "getCount:" << FileRequest::getCount[request.sdfsfilename] << "putCount:" << FileRequest::putCount[request.sdfsfilename] << " in the waiting line....." << endl;
                // FileRequest::QuqueMutex.lock();
                front_version = FileRequest::allQueue[request.sdfsfilename].front().first;
                // FileRequest::QuqueMutex.unlock();
                this_thread::sleep_for(std::chrono::seconds(1));
            }
            FileRequest::QuqueMutex.lock();
            FileRequest::getCount[request.sdfsfilename]++;
            // if (FileRequest::elementInQueue(request.sdfsfilename, SDFS_PUT))
            FileRequest::getAccu[request.sdfsfilename]++;
            FileRequest::putAccu[request.sdfsfilename] = 0;
            FileRequest::allQueue[request.sdfsfilename].pop();
            FileRequest::QuqueMutex.unlock();
        }
    }

    cout << "Ready to run version " << version << "file:" << request.sdfsfilename << endl;
    cout << "getAccu: " << getAccu[request.sdfsfilename] << endl;
    cout << "putAccu: " << putAccu[request.sdfsfilename] << endl;
    // when a put or get ready to execute, check the getAccu and putAccu, if one of them == 4, reorder the allQueue
    // for get
    if (getAccu[request.sdfsfilename] == 4) // next run has to be put
    {
        cout << "Now getAccu is 4, has to reorder the queue." << endl;
        FileRequest::reorderPutToFront(request.sdfsfilename);
    }
    else if (putAccu[request.sdfsfilename] == 4) // next run has to be get
    {
        cout << "Now putAccu is 4, has to reorder the queue." << endl;
        FileRequest::reorderGetToFront(request.sdfsfilename);
    }
}

void FileRequest::reorderPutToFront(string sdfsfilename)
{
    cout << "start Put reorder!!!!" << endl;
    FileRequest::QuqueMutex.lock();
    bool done = false;
    queue<pair<int, SDFSRequest>> OriginRequests;
    queue<pair<int, SDFSRequest>> TempRequests;
    queue<pair<int, SDFSRequest>> OutRequests;
    pair<int, SDFSRequest> target;

    while (!FileRequest::allQueue[sdfsfilename].empty())
    {
        pair<int, SDFSRequest> front = FileRequest::allQueue[sdfsfilename].front();
        FileRequest::allQueue[sdfsfilename].pop();
        OriginRequests.push(front);
        if (front.second.message_type == SDFS_PUT_LEADER && done == false)
        {
            target = front;
            done = true;
        }
        else
        {
            TempRequests.push(front);
            cout << "temp:" << front.first << endl;
        }
    }
    if (done == true)
    {

        OutRequests.push(target);
        cout << "out:" << target.first << endl;
        while (!TempRequests.empty())
        {
            OutRequests.push(TempRequests.front());
            cout << "out:" << TempRequests.front().first << endl;
            TempRequests.pop();
        }
        FileRequest::allQueue[sdfsfilename] = OutRequests;
    }
    else // no get waiting in allQueue, let the put keep doing
    {
        FileRequest::getAccu[sdfsfilename]--;
        FileRequest::allQueue[sdfsfilename] = OriginRequests;
    }
    FileRequest::QuqueMutex.unlock();
}

void FileRequest::reorderGetToFront(string sdfsfilename)
{
    cout << "start Get reorder!!!!" << endl;
    FileRequest::QuqueMutex.lock();
    bool done = false;
    queue<pair<int, SDFSRequest>> OriginRequests;
    queue<pair<int, SDFSRequest>> TempRequests;
    queue<pair<int, SDFSRequest>> OutRequests;
    pair<int, SDFSRequest> target;

    while (!FileRequest::allQueue[sdfsfilename].empty())
    {
        pair<int, SDFSRequest> front = FileRequest::allQueue[sdfsfilename].front();
        FileRequest::allQueue[sdfsfilename].pop();
        OriginRequests.push(front);
        if (front.second.message_type == SDFS_GET_LEADER && done == false)
        {
            target = front;
            done = true;
        }
        else
        {
            TempRequests.push(front);
            cout << "temp:" << front.first << endl;
        }
    }
    if (done == true)
    {

        OutRequests.push(target);
        cout << "out:" << target.first << endl;
        while (!TempRequests.empty())
        {
            OutRequests.push(TempRequests.front());
            cout << "out:" << TempRequests.front().first << endl;
            TempRequests.pop();
        }
        FileRequest::allQueue[sdfsfilename] = OutRequests;
    }
    else // no get waiting in allQueue, let the put keep doing
    {
        FileRequest::putAccu[sdfsfilename]--;
        FileRequest::allQueue[sdfsfilename] = OriginRequests;
    }
    FileRequest::QuqueMutex.unlock();
}

void FileRequest::FinishQueue(SDFSRequest request) // TODO: PAUSE WHEN RECOVERING FROM FAILURE
{
    FileRequest::QuqueMutex.lock();
    if (request.message_type == SDFS_PUT_LEADER)
    {
        cout << "file:" << request.sdfsfilename << "done!" << endl;
        FileRequest::putCount[request.sdfsfilename]--;
        cout << "putCount:" << FileRequest::putCount[request.sdfsfilename] << "done!" << endl;
    }
    else if (request.message_type == SDFS_GET_LEADER)
    {
        cout << "file:" << request.sdfsfilename << "done!" << endl;
        FileRequest::getCount[request.sdfsfilename]--;
        cout << "getCount:" << FileRequest::getCount[request.sdfsfilename] << "done!" << endl;
    }
    if (FileRequest::allQueue[request.sdfsfilename].empty()) // everone is done // TODO: erase the key?
    {
        FileRequest::getAccu[request.sdfsfilename] = 0;
        FileRequest::putAccu[request.sdfsfilename] = 0;
    }
    FileRequest::QuqueMutex.unlock();
}