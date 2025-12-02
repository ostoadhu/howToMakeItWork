#include <iostream>

class BrowserLauncher {
private:
    int result = -1;
    std::string command = "NULL";

public:
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