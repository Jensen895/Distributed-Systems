#include "Message.h"

vector<string> Message::parse_tokens_byte(const char *byte)
{
    string str_byte = string(byte);
    size_t next = 0, prev = 0;
    string token;
    vector<string> tokens;

    while ((next = str_byte.find(CUSTOM_DELIMITER, prev)) !=
           string::npos)
    {
        token = str_byte.substr(prev, next - prev);
        tokens.push_back(token);
        prev = next + CUSTOM_DELIMITER.size();
    }

    token = str_byte.substr(prev);
    tokens.push_back(token);

    return tokens;
}

vector<string> Message::parse_tokens(const string str_byte)
{

    size_t next = 0, prev = 0;
    string token;
    vector<string> tokens;

    while ((next = str_byte.find(CUSTOM_DELIMITER, prev)) !=
           string::npos)
    {
        token = str_byte.substr(prev, next - prev);
        tokens.push_back(token);
        prev = next + CUSTOM_DELIMITER.size();
    }

    token = str_byte.substr(prev);
    tokens.push_back(token);

    return tokens;
}