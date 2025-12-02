#include <iostream>


// user defined headers to see protypes and types
#include "browserLauncher.hpp"

//g++ -Wall *.cpp -o app
// ./app

// number of arguments : argc
// argv is an array of pointers to char types
int main(int argc, char *argv[])
{
    std::cout << "The Application" << std::endl;

    // initialize launcher object
    BrowserLauncher launcher;

    // Open a particular webpage
    launcher.launch("https://google.com");




    return 0;
}
