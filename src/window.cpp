#include "aluspointer.h"
#include "common.h"
#include <unordered_map>
#include <stdexcept>
#include <cairo-xcb.h>

namespace aluspointer // FIX free() invalid pointer when program terminates 
{
    xcb_atom_t _NET_CLIENT_LIST, _NET_WM_WINDOW_TYPE, _NET_WM_WINDOW_TYPE_NORMAL;
    
    template <typename T>
    int get_atom_value(xcb_window_t wid, xcb_atom_t property, xcb_atom_t type, 
    uint32_t long_len, T &data)
    {
        auto cookie = xcb_get_property(connection, 0, wid, property, 
                                        type, 0, long_len);
        auto reply = xcb_get_property_reply(connection, cookie, nullptr);

        if(!reply)
            return 0;
        
        data = (T)xcb_get_property_value(reply);

        int len = xcb_get_property_value_length(reply);
        
        free(reply);
        
        return len;
    }
    
    inline std::string get_name(xcb_window_t wid)
    {
        char *c_name;
        int len = get_atom_value<char*>(wid, XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
            100, c_name);
        if(!len)
        {
            // TODO get the atom with COMPOUND_TEXT value type
        }
        return std::string(c_name, len);
    }
    
    xcb_visualtype_t *lookup_visual(xcb_visualid_t visual_id)
    {
        auto depth_iter = xcb_screen_allowed_depths_iterator(screen);
        for(; depth_iter.rem; xcb_depth_next(&depth_iter)){
            auto visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
            for(; visual_iter.rem; xcb_visualtype_next(&visual_iter))
                if(visual_id == visual_iter.data->visual_id)
                    return visual_iter.data;
        }
        return nullptr;
    }
    
    struct window_info_t
    {
        xcb_window_t wid;
        std::string name;
        xcb_visualtype_t *visual_type;
    };
    
    std::unordered_map<uint8_t, std::shared_ptr<window_info_t>> window_info_mapper;
    
    // Reserves 170kb of memory
    std::vector<unsigned char> bitmap_buffer_temp(1024 * 170); 
    
    static cairo_status_t write_data_png(void *closure, const unsigned char *data, 
    unsigned int length)
    {
        auto p = (std::vector<unsigned char> *) closure;
        p->insert(p->end(), data, data + length);
        
        return CAIRO_STATUS_SUCCESS;
    }
    
    const std::vector<unsigned char> get_window_image(uint8_t id)
    {
        bitmap_buffer_temp.clear();
        
        auto info = window_info_mapper[id];
        int absx, absy, width, height, x, y;

        auto geo_cookie = xcb_get_geometry(connection, info->wid);
        auto geo_reply = reply_ptr<xcb_get_geometry_reply_t>
            (xcb_get_geometry_reply(connection, geo_cookie, nullptr));
        
        if(!geo_reply)
            return bitmap_buffer_temp;
            
        auto surface = cairo_xcb_surface_create(connection, info->wid, 
            info->visual_type, geo_reply->width, geo_reply->height);
        
        auto status = cairo_surface_write_to_png_stream(surface, write_data_png, 
            &bitmap_buffer_temp); // TODO use external library for the write
        
        cairo_surface_destroy(surface);
        
        return bitmap_buffer_temp;
    }
    
    inline xcb_get_property_reply_t *get_window_property_reply
        (xcb_window_t wid, xcb_atom_t property, xcb_atom_t type)
    {
        return xcb_get_property_reply(connection, 
            xcb_get_property(connection, 0, wid, property, type, 0, 100), 
            nullptr);
    }
    
    bool check_window(xcb_window_t wid, uint8_t map_state)
    {
        auto win_type_reply_scoped = reply_ptr<xcb_get_property_reply_t>
            (get_window_property_reply(wid, _NET_WM_WINDOW_TYPE, XCB_ATOM_ATOM));
        
        auto win_type = (xcb_atom_t *)xcb_get_property_value(win_type_reply_scoped.get());
        
        return  map_state == XCB_MAP_STATE_VIEWABLE && 
                (win_type && *win_type == _NET_WM_WINDOW_TYPE_NORMAL);
    }
    
    // TODO not thread-safe
    std::vector<window_client_t> update_window_list()
    {
        std::vector<window_client_t> win_client_list; // TODO store this client list locally and return const to the user 
        
        window_info_mapper.clear();
        
        auto prop_reply = get_window_property_reply(screen->root, _NET_CLIENT_LIST, XCB_ATOM_WINDOW);
        if(!prop_reply)
            return win_client_list;
        
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
                    
                if(attr_reply && check_window(window_list[i], attr_reply->map_state))
                {
                    auto name = get_name(window_list[i]);
                    
                    auto win_client = window_client_t { id, name };
                    win_client_list.push_back(win_client);
                    
                    auto info = std::make_shared<window_info_t>();
                    
                    info->name        = name;
                    info->wid         = window_list[i];
                    info->visual_type = lookup_visual(attr_reply->visual);
                    
                    window_info_mapper[id] = info;
                    
                    id++;
                }
            }
            catch(...)
            {
            }
        }
        
        return win_client_list;
    }
    
    // https://stackoverflow.com/a/54382231/10012118
    void focus_wid(xcb_window_t wid)
    {
        uint32_t val[] = { XCB_STACK_MODE_ABOVE };
        
        xcb_configure_window(connection, wid, XCB_CONFIG_WINDOW_STACK_MODE, val);
        
        xcb_set_input_focus(connection, XCB_INPUT_FOCUS_POINTER_ROOT, wid, 
            XCB_CURRENT_TIME);
        
        flush();
    }
    
    void minimize_wid(xcb_window_t wid)
    {
        // TODO
        flush();
    }
    
    void focus_window(uint8_t id)
    {
        try
        {
            focus_wid(window_info_mapper.at(id)->wid);
        }
        catch(const std::out_of_range &oor)
        {
        }
    }
    
    void minimize_window(uint8_t id)
    {
        try
        {
            minimize_wid(window_info_mapper.at(id)->wid);
        }
        catch(const std::out_of_range &oor)
        {
        }
    }
}