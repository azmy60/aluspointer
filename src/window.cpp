#include "aluspointer.h"
#include "common.h"
#include <unordered_map>

namespace aluspointer // FIX free() invalid pointer when program terminates 
{
    xcb_atom_t _NET_CLIENT_LIST;
    
    int get_atom_value(xcb_window_t wid, xcb_atom_t property, xcb_atom_t value, 
    uint32_t long_len, void **data)
    {
        auto cookie = xcb_get_property(connection, 0, wid, XCB_ATOM_WM_NAME, 
                                        XCB_ATOM_STRING, 0, 100);
        auto reply = xcb_get_property_reply(connection, cookie, nullptr);

        if(!reply)
            return 0;
        
        *data = xcb_get_property_value(reply);

        int len = xcb_get_property_value_length(reply);
        
        free(reply);
        
        return len;
    }
    
    inline std::string get_name(xcb_window_t wid)
    {
        char *c_name;
        int len = get_atom_value(wid, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 100, 
                                (void**)&c_name);
        if(!len)
        {
            // TODO get the atom with COMPOUND_TEXT value type
        }
        return std::string(c_name, len);
    }
    
    std::unordered_map<uint8_t, xcb_window_t> id_to_wid_mapper;
    std::unordered_map<xcb_window_t, std::unique_ptr<window_info>> window_info_container;
    
    // TODO not thread-safe
    std::vector<std::string> update_window_list()
    {
        // This is just my unreliable theory:
        // In order to -get all the windows- that exists on screen, we find the root
        // window from a screen first. The root window have a bunch of window
        // children. These children are what we are going to use.
        //
        // We iterate through the children, and then check the map_state if it's
        // viewable or not. If it is, then that is the window on screen.
        // 
        // I found 2 ways to get the list of these happy little windows.
        //
        // The first one is with xcb_query_tree_children. Using this in my laptop
        // gave me a list of about a hundred windows (or maybe even more).
        // 
        // The other way is by getting a specific value from the properties of a 
        // window (in this case is the root window). With this, i was given 2-3x
        // smaller list of windows. You still have to filter it, but with less
        // itteration, which is better.

        std::vector<std::string> names;
        
        id_to_wid_mapper.clear();
        window_info_container.clear();

        auto prop_cookie = xcb_get_property(connection, 0, screen->root, _NET_CLIENT_LIST, XCB_ATOM_WINDOW, 0, 100);
        auto prop_reply = xcb_get_property_reply(connection, prop_cookie, nullptr);
        if(!prop_reply)
            return names;
        
        auto prop_reply_scoped = reply_ptr<xcb_get_property_reply_t>(prop_reply);
        
        auto window_list_len = xcb_get_property_value_length(prop_reply);
        xcb_window_t *window_list = (xcb_window_t *)xcb_get_property_value(prop_reply);
        
        uint8_t id = 0;
        for(int i = 0; i < window_list_len; i++)
        {
            if(window_list[i] == 0)
                continue;
            try
            {
                auto attr_cookie = xcb_get_window_attributes(connection, window_list[i]);
                auto attr_reply = reply_ptr<xcb_get_window_attributes_reply_t>
                    (xcb_get_window_attributes_reply(connection, attr_cookie, nullptr));
                
                if(attr_reply && attr_reply->map_state == XCB_MAP_STATE_VIEWABLE)
                {
                    auto name = get_name(window_list[i]);
                    
                    names.push_back(name);
                    
                    auto info = std::make_unique<window_info>();
                    
                    info->wid = window_list[i];
                    info->name = name;
                    
                    window_info_container[window_list[i]] = std::move(info);
                    
                    id_to_wid_mapper[id] = window_list[i];
                    
                    id++;
                }
            }
            catch(...)
            {
            }
        }
        
        return names;
    }
    
    void focus_window(uint8_t id)
    {
        // TODO
    }
}