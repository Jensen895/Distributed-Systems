#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <regex>
#include <cstdlib>
#include <random>
#include "unit_test.h"
using namespace std;

UnitTest::UnitTest() {}

const int UnitTest::generateLogFileAndGrep()
{
    string filename = "unit_test.log";
    vector<string> frequentknownLines = {"Error: Something went wrong", "Warning: This is a warning message"};
    string somewhatknownLine = "Log in! Time";
    string rareknownLine = "I am rare!";
    ofstream file(filename);
    if (!file.is_open())
    {
        cerr << "Failed to open file: " << filename << endl;
        return 0;
    }

    // Write known lines
    for (int i = 1; i <= 100000; ++i)
    {
        for (const string &line : frequentknownLines)
        {
            file << line << endl;
        }
        if (i % 100 == 0)
        {
            file << somewhatknownLine << endl;
        }
        if (i % 50000 == 0)
        {
            file << rareknownLine << endl;
        }
    }

    // Generate and write random lines
    random_device rd;
    mt19937 gen(rd());

    // Define the character set for the random string
    string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    uniform_int_distribution<int> distribution(0, charset.length() - 1);

    int randomLineCount = 1000;
    for (int i = 0; i < randomLineCount; ++i)
    {
        std::string randomLine;
        for (int j = 0; j < 10; ++j)
        { // Generate a random string of length 10
            randomLine += charset[distribution(gen)];
        }
        file << randomLine << std::endl;
    }
    file.close();

    int testSum = performGrepAndVerify("Warning: This") + // frequent
                  performGrepAndVerify("Log in!") +       // somewhat frequent
                  performGrepAndVerify("am rare");        // rare
    return testSum;
};

const int UnitTest::performGrepAndVerify(const string &pattern)
{
    string filename = "unit_test.log";
    string grepCommand = "grep -c '" + pattern + "' " + filename;
    FILE *pipe = popen(grepCommand.c_str(), "r");
    int actualCount;
    if (pipe)
    {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            // Capture the matching line
            actualCount = stoi(buffer);
        }
        pclose(pipe);
    }
    else
    {
        std::cerr << "Error executing grep command." << std::endl;
    }
    return actualCount;
}

const bool UnitTest::unit_test(const int &testSum)
{
    bool allTestsPassed = true;
    // int testPassed = generateLogFileAndGrep();
    allTestsPassed = (testSum == 101002);

    // if (allTestsPassed)
    // {
    //     cout << "All tests passed!" << endl;
    // }
    // else
    // {
    //     cout << "Some tests failed." << endl;
    // }

    return allTestsPassed;
}
