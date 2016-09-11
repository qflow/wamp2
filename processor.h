#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <string>
#include <functional>

namespace qflow{
template<typename serializer>
class processor
{
public:
    template<typename Function>
    void set_send_callback(Function&& f)
    {
        _sendCallback = f;
    }
    void post_message(const std::string str)
    {
        auto message = s.deserialize(str);
        message_received(message);
    }
private:
    serializer s;
    std::function<void(std::string)> _sendCallback;
    template<typename message>
    void message_received(message m)
    {
        std::vector<typename serializer::variant_type> arr =
                adapters::as<std::vector<typename serializer::variant_type>>(m);
    }
    template<typename message>
    void send(message m)
    {
        _sendCallback(s.serialize(m));
    }

};
}

#endif
