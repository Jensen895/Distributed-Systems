#ifndef SDFS_FILELIST
#define SDFS_FILELIST

#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <algorithm>
#include <set>
#include "Message.h"
using namespace std;

class FileMetadata
{ // the information of a sdfs file
public:
    unordered_map<int, vector<string>> version_replicaids; // set of replica server_ids that sdfs store at
    void updateKey(int oldKey, int newKey)
    {
        if (version_replicaids.find(oldKey) != version_replicaids.end())
        {
            std::vector<std::string> value = version_replicaids[oldKey];
            version_replicaids.erase(oldKey);
            version_replicaids[newKey] = value;
        }
    }
};

// class FileVersionMetadata
// {
// public:
//     unordered_map<int, string> version_filenames; // set of version and filename
// };

class FileList
{
public:
    static const int REPLICA_NUMBER = 4;

    static unordered_map<string, FileMetadata *> FileMetadataMap; //<sdfsfilename, info of this file>
    static unordered_map<string, int> FileMap;
    static unordered_set<string> RecoveredFailureServers;
    static bool Recoverstart;
    static int VersionNumber;

    static mutex GlobalMutex;
    static mutex LocalMutex;

    static void CreateContext();

    static string GlobalFile_serialize();
    static void CompareGlobalFile(const string &serializedData);

    static bool CheckFileExistGlobal(const string filename);
    static bool CheckFileExistLocal(const string filename);
    static void InsertFileGlobal(const int version, const string filename, const vector<string> replicas);
    static void InsertFileLocal(const string filename);
    static void DeleteFileGlobal(const string filename);
    static void DeleteFileLocal(const string filename);
    static void UpdateFileGlobal(const int version, const string filename);
    static vector<string> GetFileReplicas(const string filename);
    static void PrintFileList();
    static void PrintFileListinClient(const string &serializedData);
    static void PrintFileLocation(const string filename); // LS
    static void PrintFileStoreLocal();                    // STORE
    static unordered_map<string, set<string>> CheckRecovery(const set<string> faillist);
    static void InsertRecover(string server);
    static void DeleteServerGlobal(const string server);
};

#endif
