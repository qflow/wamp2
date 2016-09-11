#ifndef SESSION_H
#define SESSION_H

namespace qflow{
enum class SESSION_STATE : int {
    OPENED = 1,
    CLOSED = -1
};
template<typename transport_connection_type, typename serializer, typename user>
class session
{
public:
    SESSION_STATE state() const;
private:
    SESSION_STATE _state;
};
}

#endif
