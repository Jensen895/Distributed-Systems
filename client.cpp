#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <future>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <random>
#include <arpa/inet.h>
#include <chrono>
#include <netdb.h>

#include "src/RequestMessage.h"
#include "src/ResponseMessage.h"
#include "src/unit_test.h"
#include "src/MembershipRequest.h"
#include "src/MJRequest.h"
#include "src/Messagefunction.h"
#include "src/Message.h"

#include "src/SDFSRequest.h"

#define PORT "3491"     // the port client will be connecting to
#define MAXDATASIZE 100 // max number of bytes we can get at once

using namespace std;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

Message *Querycommand(vector<string> args)
{
    MESSAGE_TYPE message_type;
    string pattern;
    bool useRegex;
    bool unit_test;

    if (args.size() < 1)
    {
        cerr << "Usage: ./client grep [-r] <pattern> or ./client unit_test" << endl;
        exit(1);
    }
    if (args.size() == 3 && args[1] == "-r")
    {
        useRegex = true;
        pattern = args[2];
        unit_test = false;
    }
    else if (args.size() == 2)
    {
        useRegex = false;
        pattern = args[1];
        unit_test = false;
    }
    else if (args.size() == 1 && args[0] == "unit_test")
    {
        useRegex = false;
        pattern = "";
        unit_test = true;
    }
    else
    {
        cerr << "Usage: ./client grep [-r] <pattern>" << endl;
        exit(1);
    }
    message_type = QUERY_LOG;
    return new RequestMessage(message_type, pattern, useRegex, unit_test);
}

Message *Membershipcommand(vector<string> args)
{
    if (args.size() < 2)
    {
        cerr << "Usage: ./client membership [join|leave|print_list] " << endl;
        exit(1);
    }
    MESSAGE_TYPE message_type;
    string hostname;
    string port;
    if (args[1] == "join")
    {
        message_type = JOIN;
        if (args.size() == 4)
        {
            hostname = args[2];
            port = args[3];
            return new MembershipRequest(message_type, hostname, port);
        }
        else
        {
            cerr << "Usage: ./client membership join <new hostname> <port>" << endl;
            exit(1);
        }
    }
    else if (args[1] == "leave")
    {
        message_type = LEAVE;
        if (args.size() == 4)
        {
            hostname = args[2];
            port = args[3];
            return new MembershipRequest(message_type, hostname, port);
        }
        else
        {
            cerr << "Usage: ./client membership leave <leave hostname> <port>" << endl;
            exit(1);
        }
    }
    else if (args[1] == "print_list")
    {
        message_type = MBRLIST;
        if (args.size() == 4)
        {
            hostname = args[2];
            port = args[3];
            return new MembershipRequest(message_type, hostname, port);
        }
        else
        {
            cerr << "Usage: ./client membership print_list <target hostname> <port>" << endl;
            exit(1);
        }
    }
    else if (args[1] == "suspicion")
    {
        message_type = SUSPICION;
        if (args.size() == 4)
        {
            hostname = args[2];
            port = args[3];
            return new MembershipRequest(message_type, hostname, port);
        }
        else
        {
            cerr << "Usage: ./client membership print_list <target hostname> <port>" << endl;
            exit(1);
        }
    }
    else
    {
        cerr << "Usage: ./client membership [join|leave|print_list|suspicion]" << endl;
        exit(1);
    }
}

Message *SDFScommand(vector<string> args)
{
    if (args.size() < 2)
    {
        cerr << "Usage: ./client sdfs [put|get|delete|ls|store] " << endl;
        exit(1);
    }

    SDFSRequest *sdfsRequest;
    MESSAGE_TYPE message_type;
    string hostname;
    string port;
    string localfilename;
    string sdfsfilename;
    int version;
    if (args[1] == "put")
    {
        message_type = SDFS_PUT;
        if (args.size() == 6)
        {

            hostname = args[2];
            port = args[3];
            localfilename = args[4];
            sdfsfilename = args[5];

            return new SDFSRequest(message_type, hostname, port, localfilename, sdfsfilename, 0);
        }
        else
        {
            cerr << "Usage: ./client sdfs put <member host> <member port> <localfilename> <sdfsfilename>" << endl;
            exit(1);
        }
    }
    else if (args[1] == "get")
    {
        message_type = SDFS_GET;
        if (args.size() == 6)
        {
            hostname = args[2];
            port = args[3];
            localfilename = args[5];
            sdfsfilename = args[4];
            return new SDFSRequest(message_type, hostname, port, localfilename, sdfsfilename, 0);
        }
        else
        {
            cerr << "Usage: ./client sdfs get <member host> <member port> <sdfsfilename> <localfilename>" << endl;
            exit(1);
        }
    }
    else if (args[1] == "delete")
    {
        message_type = SDFS_DELETE;
        if (args.size() == 5)
        {
            hostname = args[2];
            port = args[3];
            localfilename = "";
            sdfsfilename = args[4];
            return new SDFSRequest(message_type, hostname, port, localfilename, sdfsfilename, 0);
        }
        else
        {
            cerr << "Usage: ./client delete <member host> <member port> <sdfsfilename>" << endl;
            exit(1);
        }
    }
    else if (args[1] == "ls")
    {
        message_type = SDFS_LS;
        if (args.size() == 5)
        {
            hostname = args[2];
            port = args[3];
            sdfsfilename = args[4];
            return new SDFSRequest(message_type, hostname, port, "", sdfsfilename, 0);
        }
        else
        {
            cerr << "Usage: ./client sdfs ls <member host> <member port> <sdfsfilename>" << endl;
            exit(1);
        }
    }
    else if (args[1] == "store")
    {
        message_type = SDFS_STORE;
        if (args.size() == 4)
        {
            hostname = args[2];
            port = args[3];
            return new SDFSRequest(message_type, hostname, port, "", "", 0);
        }
        else
        {
            cerr << "Usage: ./client sdfs store <member host> <member port>" << endl;
            exit(1);
        }
    }
    else if (args[1] == "print_global")
    {
        message_type = SDFS_PRINT_GLOBAL;
        if (args.size() == 4)
        {
            hostname = args[2];
            port = args[3];
            return new SDFSRequest(message_type, hostname, port, "", "", 0);
        }
        else
        {
            cerr << "Usage: ./client print_global <member host> <member port>" << endl;
            exit(1);
        }
    }
    else
    {
        cerr << "Usage: ./client sdfs [put|get|delete|ls|store]" << endl;
        exit(1);
    }
}

Message *MAPLEJUICEcommand(vector<string> args)
{
    if (args.size() < 2)
    {
        cerr << "Usage: ./client [maple|juice] " << endl;
        exit(1);
    }
    MESSAGE_TYPE message_type;
    string exe_file;
    string num_workers;
    string sdfs_intermediate_filename;
    string src_dict;
    string dest_file;
    string delete_input;
    if (args[0] == "maple")
    {
        message_type = MAPLE;
        if (args.size() == 5)
        {
            exe_file = args[1];
            num_workers = args[2];
            sdfs_intermediate_filename = args[3];
            src_dict = args[4];
            return new MJRequest(message_type, exe_file, num_workers, sdfs_intermediate_filename, src_dict);
        }
        else
        {
            cerr << "Usage: ./client maple <maple_exe> <num_maples> <sdfs_intermediate_filename_prefix> <sdfs_src_directory>" << endl;
            exit(1);
        }
    }
    else if (args[0] == "juice")
    {
        message_type = JUICE;
        if (args.size() == 6)
        {
            exe_file = args[1];
            num_workers = args[2];
            sdfs_intermediate_filename = args[3];
            dest_file = args[4];
            delete_input = args[6];
            return new MJRequest(message_type, exe_file, num_workers, sdfs_intermediate_filename, dest_file, delete_input);
        }
        else
        {
            cerr << "Usage: ./client juice <juice_exe> <num_juices> <sdfs_intermediate_filename_prefix> <sdfs_dest_filename> delete_input={0,1}" << endl;
            exit(1);
        }
    }
    else
    {
        cerr << "Usage: ./client [maple|juice]" << endl;
        exit(1);
    }
}

Message *checkcommand(int argc, char const *argv[])
{
    if (argc > 1)
    {
        vector<string> all_args;
        all_args.assign(argv + 1, argv + argc);
        if (all_args[0] == "grep" || all_args[0] == "unit_test")
        {
            return Querycommand(all_args);
        }
        else if (all_args[0] == "membership")
        {
            return Membershipcommand(all_args);
        }
        else if (all_args[0] == "sdfs")
        {
            return SDFScommand(all_args);
        }
        else if (all_args[0] == "maple" || all_args[0] == "juice")
        {
            return MAPLEJUICEcommand(all_args);
        }
    }
    cout << "Usage: ./client [grep|unit_test|membership|sdfs|maple|juice]" << endl;
    exit(1);
}

int main(int argc, char const *argv[])
{
    auto start = chrono::high_resolution_clock::now();

    Message *message = checkcommand(argc, argv);
    Messagefunction::implement_request(message);

    delete message;

    // Calculate the time for the report
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
    float time = float(duration.count()) / 1000000.0;
    cout << "Execution time: " << time << " seconds" << endl;

    return 0;
}
