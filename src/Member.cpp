
#include "Member.h"

#define ID_LENGTH 4

using namespace std;
const string MEMBER_ID_DELIMITER = "@";

Member::Member(string hostname, int port)
{
    long localtime = (chrono::system_clock::now().time_since_epoch()).count();
    string localtime_str = to_string(localtime);
    this->time_id = localtime_str.substr(localtime_str.size() - ID_LENGTH);

    this->port = to_string(port);

    this->hostname = hostname;
}

Member::Member(string member_id)
{
    vector<string> tokens;
    size_t pos = 0;
    while ((pos = member_id.find(MEMBER_ID_DELIMITER)) != std::string::npos)
    {
        string token(member_id.substr(0, pos));
        tokens.push_back(token);
        member_id.erase(0, pos + MEMBER_ID_DELIMITER.length());
    }
    if (tokens.size() == 2)
    {
        port = member_id; // input string remains port
        time_id = tokens[0];
        hostname = tokens.back();
    }
    else
    {
        throw runtime_error("Error occurs when parsing member ID");
    }
}

string Member::to_member_id()
{
    return time_id + MEMBER_ID_DELIMITER + hostname + MEMBER_ID_DELIMITER + port;
}

string Member::generate_member(string host, int port)
{
    long localtime = (chrono::system_clock::now().time_since_epoch()).count();
    string localtime_str = to_string(localtime);
    string localtime_id = localtime_str.substr(localtime_str.size() - ID_LENGTH);
    // cout << "create member " << localtime_id << " at: " << localtime_str << endl;
    string memberid = localtime_id + MEMBER_ID_DELIMITER + host + MEMBER_ID_DELIMITER +
                      to_string(port);
    // cout << "memberid: " << memberid << endl;
    return memberid;
}

Member Member::parse_member_id(string member_id)
{
    return Member(member_id);
}