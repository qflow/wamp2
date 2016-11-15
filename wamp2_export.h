
#ifndef WAMP2_EXPORT_H
#define WAMP2_EXPORT_H

#ifdef WAMP2_STATIC_DEFINE
#  define WAMP2_EXPORT
#  define WAMP2_NO_EXPORT
#else
#  ifndef WAMP2_EXPORT
#    ifdef wamp2_EXPORTS
        /* We are building this library */
#      define WAMP2_EXPORT 
#    else
        /* We are using this library */
#      define WAMP2_EXPORT 
#    endif
#  endif

#  ifndef WAMP2_NO_EXPORT
#    define WAMP2_NO_EXPORT 
#  endif
#endif

#ifndef WAMP2_DEPRECATED
#  define WAMP2_DEPRECATED __declspec(deprecated)
#endif

#ifndef WAMP2_DEPRECATED_EXPORT
#  define WAMP2_DEPRECATED_EXPORT WAMP2_EXPORT WAMP2_DEPRECATED
#endif

#ifndef WAMP2_DEPRECATED_NO_EXPORT
#  define WAMP2_DEPRECATED_NO_EXPORT WAMP2_NO_EXPORT WAMP2_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef WAMP2_NO_DEPRECATED
#    define WAMP2_NO_DEPRECATED
#  endif
#endif

#endif
