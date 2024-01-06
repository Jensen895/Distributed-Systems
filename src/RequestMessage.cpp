#include "RequestMessage.h"
#include <stdexcept>
#include <iostream>
using namespace std;

// const string CUSTOM_DELIMITER = "@@@chhuang5~~~yy63~~~@@@"; // Define your custom delimiter character here

RequestMessage::RequestMessage(MESSAGE_TYPE message_type, string pattern, bool useRegex, bool unit_test)
    : message_type(message_type), pattern(pattern), useRegex(useRegex), unit_test(unit_test)
{
}

const string RequestMessage::serialize()
{
    // Serialize the struct into a string
    string serialized = to_string(message_type) + CUSTOM_DELIMITER + pattern + CUSTOM_DELIMITER + (useRegex ? "1" : "0") + CUSTOM_DELIMITER + (unit_test ? "1" : "0");
    // std::cout << "start serialize: useRegex is " << useRegex << " " << serialized << std::endl;
    return serialized;
}

RequestMessage RequestMessage::deserialize(const string &serializedData)
{
    // Static method to deserialize a string into a RequestMessage
    size_t delimiterPos = serializedData.find(CUSTOM_DELIMITER);
    if (delimiterPos != string::npos && delimiterPos < serializedData.length() - 1)
    {
        size_t delimiterPos2 = serializedData.find(CUSTOM_DELIMITER, delimiterPos + CUSTOM_DELIMITER.length());
        if (delimiterPos2 != std::string::npos)
        {
            MESSAGE_TYPE message_type = (MESSAGE_TYPE)stoi(serializedData.substr(0, delimiterPos));
            string pattern = serializedData.substr(delimiterPos + CUSTOM_DELIMITER.length(), delimiterPos2 - delimiterPos - CUSTOM_DELIMITER.length());
            bool useRegex = (serializedData[delimiterPos2 + CUSTOM_DELIMITER.length()] == '1');
            bool unit_test = (serializedData[delimiterPos2 + 2 * CUSTOM_DELIMITER.length() + 1] == '1');
            return RequestMessage(message_type, pattern, useRegex, unit_test);
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