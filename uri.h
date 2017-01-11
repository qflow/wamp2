#ifndef URI_H
#define URI_H

#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

namespace qflow {
class uri
{
public:
    uri(const std::string& uri_str)
    {
        auto i = uri_str.find("://");

        std::string str = uri_str;
        if(i != std::string::npos)
        {
            scheme_ = uri_str.substr(0, i);
            str = str.erase(0, i + 3);
        }
        i = str.find('?');
        if(i != std::string::npos)
        {
            query_ = str.substr(i + 1);
            str = str.substr(0, i);
        }
        i = str.find('/');
        if(i > 0)
        {
            host_ = str.substr(0, i);
            str = str.erase(0, i);
        }
        path_ = str;

        if(query_.size() > 0)
        {
            std::stringstream query_ss(query_);
            std::string query_item;
            while (std::getline(query_ss, query_item, '&'))
            {
                std::stringstream item_ss(query_item);
                std::string s;
                std::getline(item_ss, s, '=');
                std::string first = s;
                std::getline(item_ss, s, '=');
                query_items_[first] = s;
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
        return query_items_.find( key ) != query_items_.end();
    }
    std::string query_item_value(const std::string& key) const
    {
        return query_items_.at(key);
    }
    std::string path() const
    {
        return path_;
    }
    std::string host() const
    {
        return host_;
    }
    std::string scheme() const
    {
        return scheme_;
    }
    std::string url_no_query() const
    {
        return scheme_ + "://" + host_ + path_;
    }
    std::string url_path_query() const
    {
        return path_ + "?" + query_;
    }
    static std::string url_encode(const std::string &value) {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
            std::string::value_type c = (*i);

            // Keep alphanumeric and other accepted characters intact
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                escaped << c;
                continue;
            }

            // Any other characters are percent-encoded
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << int((unsigned char) c);
            escaped << std::nouppercase;
        }

        return escaped.str();
    }
private:
    std::string host_;
    std::string scheme_;
    std::string path_;
    std::string query_;
    std::unordered_map<std::string, std::string> query_items_;
    std::vector<std::string> path_items;
};
}
#endif
