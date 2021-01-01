#include "common.h"

namespace aluspointer
{
    xcb_atom_t locate_atom(std::string name)
    {
        auto cookie = xcb_intern_atom(connection, 1, name.size(), name.c_str());
        auto reply = reply_ptr<xcb_intern_atom_reply_t>
            (xcb_intern_atom_reply(connection, cookie, nullptr));
        if(!reply) return 0;
        return reply->atom;
    }    
}
