#ifndef TO_STRING
#define TO_STRING

#include <string>
#include <iostream>

template<typename T>
std::string to_string(T message)
{
    std::string res;
    for(auto field: message.fields)
    {
        res += field.first + ": " + field.second + "\n";
    }
    return res;
}
template<typename T>
void dump(T message)
{
    std::string res = to_string(message);
    std::cout << res + "\n\n";
}
#endif
