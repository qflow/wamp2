#ifndef URI_H
#define URI_H

#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>

namespace qflow {
class uri
{
public:
    uri(const std::string& uri_str)
    {
        std::stringstream ss(uri_str);
        std::vector<std::string> parts;
        std::string item;

        while (std::getline(ss, item, '?'))
        {
            parts.push_back(item);
        }
        path_ = parts[0];
        if(parts.size() == 2)
        {
            query = parts[1];
            std::stringstream query_ss(query);
            std::string query_item;
            while (std::getline(query_ss, query_item, ';'))
            {
                std::stringstream item_ss(query_item);
                std::string s;
                std::getline(item_ss, s, '=');
                std::string first = s;
                std::getline(item_ss, s, '=');
                query_items[first] = s;
            }
        }
        std::stringstream path_ss(path_);
        std::string path_item;
        while (std::getline(path_ss, path_item, '/'))
        {
            path_items.push_back(path_item);
        }
    }
    bool contains_query_item(const std::string& key) const
    {
        return query_items.find( key ) != query_items.end();
    }
    std::string query_item_value(const std::string& key) const
    {
        return query_items.at(key);
    }
    std::string path() const
    {
        return path_;
    }
private:
    std::string path_;
    std::string query;
    std::unordered_map<std::string, std::string> query_items;
    std::vector<std::string> path_items;
};
}
#endif
