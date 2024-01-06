#include "MemberIdResponse.h"
#include <stdexcept>
#include <iostream>
using namespace std;

MemberIdResponse::MemberIdResponse(MESSAGE_TYPE message_type, string memberId, string membershiplist)
    : message_type(message_type), memberId(memberId), membershiplist(membershiplist)
{
}

const string MemberIdResponse::serialize() { return to_string(message_type) + CUSTOM_DELIMITER + memberId + CUSTOM_DELIMITER + membershiplist; }

MemberIdResponse MemberIdResponse::deserialize(const string &serializedData)
{
    // Static method to deserialize a string into a RequestMessage
    size_t delimiterPos = serializedData.find(CUSTOM_DELIMITER);
    if (delimiterPos != string::npos && delimiterPos < serializedData.length() - 1)
    {
        size_t delimiterPos2 = serializedData.find(CUSTOM_DELIMITER, delimiterPos + CUSTOM_DELIMITER.length());
        if (delimiterPos2 != std::string::npos)
        {
            MESSAGE_TYPE message_type = (MESSAGE_TYPE)stoi(serializedData.substr(0, delimiterPos));
            string memberId = serializedData.substr(delimiterPos + CUSTOM_DELIMITER.length(), delimiterPos2 - delimiterPos - CUSTOM_DELIMITER.length());
            string membershiplist = serializedData.substr(delimiterPos2 + CUSTOM_DELIMITER.length());
            return MemberIdResponse(message_type, memberId, membershiplist);
        }
        else
        {
            throw runtime_error("Invalid data format during deserialization.");
        }
    }
    else
    {
        throw runtime_error("Invalid data format during deserialization.");
    }
}