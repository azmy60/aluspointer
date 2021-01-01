#ifndef COMMON_H
#define COMMON_H

#include <xcb/xcb.h>
#include <memory>
#include <string>

namespace aluspointer
{
    template<typename T>
    struct ReplyDeleter
    {
        void operator()(T* p) const
        {
            if(p) free(p);
        }
    };
    
    template<typename T>
    using reply_ptr = std::unique_ptr<T, ReplyDeleter<T>>;
    using screen_ptr = std::unique_ptr<xcb_screen_t>;
    
    xcb_connection_t *connection;
    screen_ptr screen;
    
    xcb_atom_t _NET_CLIENT_LIST;
    
    xcb_atom_t locate_atom(std::string name);
}

#endif // COMMON_H