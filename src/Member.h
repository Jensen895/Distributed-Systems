#ifndef MEMBER
#define MEMBER
#include <mutex>
#include <string>
#include <iostream>
#include <array>
#include <unordered_map>
#include <vector>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <set>

using namespace std;

class Member
{
public:
    string time_id;
    string hostname;
    string port;
    Member() {}
    Member(string hostname, int port);

    Member(string member_id);

    string to_member_id();
    static string generate_member(string host, int port);
    static Member parse_member_id(string member_id);
};

#endif
