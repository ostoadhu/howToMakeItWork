#include <iostream>


// user defined headers to see protypes and types
#include "browserLauncher.hpp"

//g++ -Wall *.cpp -o tibet

// number of arguments : argc
// argv is an array of pointers to char types
int main(int argc, char *argv[])
{
    std::cout << "Tibet Application" << std::endl;

    BrowserLauncher launcher;

    launcher.launch("https://google.com");




    return 0;
}
