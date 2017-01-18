#ifndef TRANSLATE_H
#define TRANSLATE_H

#include <regex>

template<typename Map>
std::string translate(std::string input, Map map)
{
    for(auto entry: map)
    {
        std::regex regex(entry.first);
        std::smatch match;
        if(std::regex_match(input, match, regex))
        {
            std::string output = entry.second;
            if (match.size() > 1) {
                for(int i=1; i<match.size(); i++)
                {
                    std::ssub_match sub_match = match[i];
                    std::string str = sub_match.str();
                }
            }
            return output;
        }
    }

    return std::string();
}

#endif
