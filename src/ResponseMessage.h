#ifndef RESPONSEMESSAGE_H
#define RESPONSEMESSAGE_H

#include <string>

struct ResponseMessage
{
    int matching_line_count;
    std::string in_file;
    int unittest_result;

    ResponseMessage(int matching_line_count = 0, std::string in_file = "", int unittest_result = 0);

    const std::string serialize();

    static ResponseMessage deserialize(const std::string &data);
};

#endif // RESPONSEMESSAGE_H
