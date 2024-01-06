#include "ResponseMessage.h"

#include <stdexcept>
#include <iostream>
#include <string>

const std::string CUSTOM_DELIMITER = "@@@chhuang5~~~yy63~~~@@@"; // Define your custom delimiter character here

ResponseMessage::ResponseMessage(int matching_line_count, std::string in_file, int unittest_result)
    : matching_line_count(matching_line_count), in_file(in_file), unittest_result(unittest_result)
{
}

const std::string ResponseMessage::serialize()
{
    // Serialize the struct into a string
    std::string serialized = std::to_string(matching_line_count) + CUSTOM_DELIMITER + in_file + CUSTOM_DELIMITER + std::to_string(unittest_result);
    return serialized;
}

ResponseMessage ResponseMessage::deserialize(const std::string &serializedData)
{
    // Static method to deserialize a string into a ResponseMessage
    size_t delimiterPos1 = serializedData.find(CUSTOM_DELIMITER);
    if (delimiterPos1 != std::string::npos && delimiterPos1 < serializedData.length() - 1)
    {
        size_t delimiterPos2 = serializedData.find(CUSTOM_DELIMITER, delimiterPos1 + CUSTOM_DELIMITER.length());
        if (delimiterPos2 != std::string::npos)
        {
            int matching_line_count = stoi(serializedData.substr(0, delimiterPos1));
            std::string in_file = serializedData.substr(delimiterPos1 + CUSTOM_DELIMITER.length(), delimiterPos2 - delimiterPos1 - CUSTOM_DELIMITER.length());
            int unittest_result = stoi(serializedData.substr(delimiterPos2 + CUSTOM_DELIMITER.length()));
            return ResponseMessage(matching_line_count, in_file, unittest_result);
        }
        else
        {
            throw std::runtime_error("Invalid data format during deserialization.");
        }
    }
    else
    {
        throw std::runtime_error("Invalid data format during deserialization.");
    }
}