#include "connection.h"
#include "authenticator.h"
#include "msgpack_serializer.h"


namespace qflow{

template class session<client::connection_ptr, msgpack_serializer, wampcra_authenticator>;
}
