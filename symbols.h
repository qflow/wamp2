#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <string>

namespace qflow{
const std::string KEY_SUBSCRIPTION_ON_DELETE = "wamp.subscription.on_delete";
const std::string KEY_SUBSCRIPTION_ON_CREATE = "wamp.subscription.on_create";
const std::string KEY_SUBSCRIPTION_ON_SUBSCRIBE = "wamp.subscription.on_subscribe";
const std::string KEY_SUBSCRIPTION_ON_UNSUBSCRIBE = "wamp.subscription.on_unsubscribe";
const std::string KEY_REGISTRATION_ON_DELETE = "wamp.registration.on_delete";
const std::string KEY_REGISTRATION_ON_CREATE = "wamp.registration.on_create";
const std::string KEY_GET_SUBSCRIPTION = "wamp.subscription.get";
const std::string KEY_COUNT_SUBSCRIBERS = "wamp.subscription.count_subscribers";
const std::string KEY_DEFINE_SCHEMA = "wamp.schema.define";
const std::string KEY_DESCRIBE_SCHEMA = "wamp.schema.describe";
const std::string KEY_LOOKUP_REGISTRATION = "wamp.registration.lookup";
const std::string KEY_LIST_REGISTRATION_URIS = "wamp.registration.list_uris";
const std::string KEY_ERR_NOT_AUTHORIZED = "wamp.error.not_authorized";
const std::string KEY_ERR_NOT_AUTHENTICATED = "wamp.error.not_authenticated";
const std::string KEY_ERR_NO_SUCH_PROCEDURE = "wamp.error.no_such_procedure";
const std::string KEY_ERR_NO_SUCH_REGISTRATION = "wamp.error.no_such_registration";
const std::string KEY_ERR_NO_SUCH_SUBSCRIPTION = "wamp.error.no_such_subscription";
const std::string KEY_ERR_NO_SUCH_REALM = "wamp.error.no_such_realm";
const std::string KEY_WAMP_JSON_SUB = "wamp.2.json";
const std::string KEY_WAMP_MSGPACK_SUB = "wamp.2.msgpack";

enum WampMsgCode : int {
    HELLO = 1,
    WELCOME = 2,
    ABORT = 3,
    CHALLENGE = 4,
    AUTHENTICATE = 5,
    GOODBYE = 6,
    HEARTBEAT = 7,
    ERROR = 8,
    PUBLISH = 16,
    PUBLISHED = 17,
    SUBSCRIBE = 32,
    SUBSCRIBED = 33,
    UNSUBSCRIBE = 34,
    UNSUBSCRIBED = 35,
    EVENT = 36,
    CALL = 48,
    CANCEL = 49,
    RESULT = 50,
    REGISTER = 64,
    REGISTERED = 65,
    UNREGISTER = 66,
    UNREGISTERED = 67,
    INVOCATION = 68,
    INTERRUPT = 69,
    YIELD = 70
};
}
#endif
