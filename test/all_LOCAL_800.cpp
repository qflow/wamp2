#include "connection.h"
#include <vector>
#include <string>
#include <iostream>
#include <cassert>

int main()
{
    //qflow::connection c;

    auto obj = std::make_tuple(std::string("ahoj"), 5);
    qflow::msgpack_serializer serializer;
    std::string ss = serializer.serialize(obj);
    auto des = serializer.deserialize(ss);
    std::vector<msgpack::object> arr = adapters::as<std::vector<msgpack::object>>(des);
    std::string s = adapters::as<std::string>(arr[0]);
    assert(s == std::string("ahoj"));
}
