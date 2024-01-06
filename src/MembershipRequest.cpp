#include "MembershipRequest.h"
#include <stdexcept>
#include <iostream>
using namespace std;

// MembershipRequest::MembershipRequest(MESSAGE_TYPE) : message_type(message_type){}

MembershipRequest::MembershipRequest(MESSAGE_TYPE message_type)
    : message_type(message_type)
{
}

MembershipRequest::MembershipRequest(MESSAGE_TYPE message_type, string hostname, string port)
    : message_type(message_type), hostname(hostname), port(port)
{
}

const string MembershipRequest::serialize()
{
    // Serialize the struct into a string
    string serialized = to_string(message_type) + CUSTOM_DELIMITER + hostname + CUSTOM_DELIMITER + port + CUSTOM_DELIMITER + memberid;
    // std::cout << "start serialize: useRegex is " << useRegex << " " << serialized << std::endl;
    return serialized;
}

MembershipRequest MembershipRequest::deserialize(const string &serializedData)
{
    // Static method to deserialize a string into a RequestMessage
    size_t delimiterPos = serializedData.find(CUSTOM_DELIMITER);
    if (delimiterPos != string::npos && delimiterPos < serializedData.length() - 1)
    {
        size_t delimiterPos2 = serializedData.find(CUSTOM_DELIMITER, delimiterPos + CUSTOM_DELIMITER.length());
        if (delimiterPos2 != std::string::npos)
        {

            MESSAGE_TYPE message_type = (MESSAGE_TYPE)stoi(serializedData.substr(0, delimiterPos));
            string hostname = serializedData.substr(delimiterPos + CUSTOM_DELIMITER.length(), delimiterPos2 - delimiterPos - CUSTOM_DELIMITER.length());
            string port = serializedData.substr(delimiterPos2 + CUSTOM_DELIMITER.length());
            return MembershipRequest(message_type, hostname, port);
        }
        else
        {
            throw std::runtime_error("Invalid data format during deserialization.");
        }
    }
    else
    {
        throw runtime_error("Invalid data format during deserialization.");
    }
}