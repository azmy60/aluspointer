#ifndef COMMON_H
#define COMMON_H

#include <X11/Xlib.h>
#include <memory>
#include <string>

namespace aluspointer
{
    template <typename T>
    struct XDeleter
    {
        void operator()(T *p) const
        {
            if(p) XFree(p);
        }
    };
    
    template <typename T>
    using x_ptr = std::unique_ptr<T, XDeleter<T>>;
    
    extern Display *display_;
    extern Window root_;
    extern int screen_number;
    
    inline void flush()
    {
        XFlush(display_);
    }
//    
    extern Atom _NET_CLIENT_LIST;
    extern Atom _NET_WM_WINDOW_TYPE;
    extern Atom _NET_WM_WINDOW_TYPE_NORMAL;
//    extern Atom _NET_WM_STATE;
//    extern Atom _NET_WM_STATE_HIDDEN;
    extern Atom _NET_ACTIVE_WINDOW;
    
    inline Atom locate_atom(std::string name)
    {
        return XInternAtom(display_, name.c_str(), true);
    } 
}

#endif // COMMON_H