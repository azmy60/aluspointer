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

    extern xcb_connection_t *connection;
    extern xcb_screen_t *screen;
    
    
    inline void flush()
    {
        xcb_flush(connection);
    }
    
    extern xcb_atom_t _NET_CLIENT_LIST;
    extern xcb_atom_t _NET_WM_WINDOW_TYPE;
    extern xcb_atom_t _NET_WM_WINDOW_TYPE_NORMAL;
    
    xcb_atom_t locate_atom(std::string name);
}

#endif // COMMON_H