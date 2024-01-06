#ifndef UNITTEST_H
#define UNITTEST_H

#include <string>
#include <iostream>

struct UnitTest
{

    UnitTest(); // Constructor

    const int generateLogFileAndGrep(); // Function to generate log file and perform grep

    const int performGrepAndVerify(const std::string &pattern); // Function to perform grep and verify results

    const bool unit_test(const int &testSum); // Function to run unit tests
};

#endif // UNITTEST_H