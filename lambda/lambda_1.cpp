#include <iostream>
#include <algorithm>
#include <vector>

// g++ -std=c++20 lambda_1.cpp -Wall -Wextra -o .prog
 
int main()
{
    std::vector<int> v{1,2,3,4,5};


    std::for_each( v.begin(),
                   v.end(),
                   [](auto a){ std::cout << a;}
    );

/*
    for (int &value : v)
    {
        std::cout << value << ",";
    }
*/

    std::cout << "\n";

    return 0;
}