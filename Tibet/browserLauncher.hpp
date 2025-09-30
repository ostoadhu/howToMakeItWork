#include <iostream>

class BrowserLauncher {
private:
    // Need constructor BC we have states (member variables)
    int result = -1;
    std::string command = "NULL";

public:
    // Constructor
    BrowserLauncher() : result{-1}, command{"NULL"} {};

    // public methods
    void launch(const std::string& url) 
    {
        command = "chromium " + url + " &";
        result = std::system(command.c_str());
    }

    int getResult() const {
        return result;
    }

    std::string getCommand() const {
        return command;
    }
};