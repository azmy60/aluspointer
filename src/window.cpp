#include "aluspointer.h"
#include "common.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unordered_map>
#include <stdexcept>
#include <cairo-xlib.h>
#include <iostream>
#include <thread>
#include <algorithm>
#include <mutex>
#include <condition_variable>

namespace aluspointer // FIX free() invalid pointer when program terminates 
{
    Atom _NET_CLIENT_LIST, _NET_WM_WINDOW_TYPE_NORMAL, _NET_ACTIVE_WINDOW;
//        ,_NET_WM_STATE, _NET_WM_STATE_HIDDEN, ;
  
    Atom _NET_WM_WINDOW_TYPE;
    
    const unsigned char *get_window_property(Window win, Atom atom, long &length,
        Atom &type, int &size)
    {
        Atom actual_type;
        int actual_format;
        ulong n_items, bytes_after;//, n_bytes;
        unsigned char *prop;
        
        auto status = XGetWindowProperty(display_, win, atom, 0, ~0, false, 
            AnyPropertyType, &actual_type, &actual_format, &n_items,
            &bytes_after, &prop);
        
        if(status == BadWindow)
        {
            std::cerr << "Window id " << win << "does not exists!" <<std::endl;
            return nullptr;
        }
        else if(status != Success)
        {
            std::cerr << "XGetWindowProperty failed!" <<std::endl;
            return nullptr;
        }
        
        length = n_items;
        size = actual_format;
        type = actual_type;
        
        return prop;
    }
    
    const std::string get_name(Window win)
    {
        long length;
        Atom type;
        int size;
        char *name = (char *)get_window_property(win, XA_WM_NAME, length, type, size);
        return std::string(name);
    }
    
    struct window_info_t
    {
        Window win;
        std::string name;
        Visual *visual;
    };
    
    std::unordered_map<uint8_t, std::shared_ptr<window_info_t>> window_info_mapper;
    
    int id_to_listen;
    Window win_to_listen;
    bool start_listening;
    std::mutex m;
    std::condition_variable cv;
    
    // Blocking function
    void start_listen_to_updated_windows(std::function<void(const int)> callback)
    {
        XEvent event;
        while(true)
        {
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, []{ return start_listening; });
            
            XSelectInput(display_, win_to_listen, FocusChangeMask);
            
            XNextEvent(display_, &event);
            
            if(event.xfocus.type == FocusOut)
            {
                std::cout << "focus out = " << (event.xfocus.type == FocusOut) << std::endl;
                callback(id_to_listen);
            }
            
            start_listening = false;
            
            lk.unlock();
            cv.notify_one();
        }
    }
    
    void listen_to_window(int id)
    {
        {
            std::lock_guard<std::mutex> lk(m);
            id_to_listen = id;
            win_to_listen = window_info_mapper[id]->win;
            start_listening = true;            
        }
        cv.notify_one();
    }
    
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
        int x_tmp, y_tmp;
        unsigned int width, height, border_width_tmp, depth_tmp;
        Window root_tmp;
        
        auto status = XGetGeometry(display_, info->win, &root_tmp, &x_tmp, 
            &y_tmp, &width, &height, &border_width_tmp, &depth_tmp);
        
        if(!status)
            return bitmap_buffer_temp; // empty
            
        auto surface = cairo_xlib_surface_create(display_, info->win, 
            info->visual, width, height);
        
        cairo_surface_write_to_png_stream(surface, write_data_png, 
            &bitmap_buffer_temp); // TODO use external library for writing
        
        cairo_surface_destroy(surface);
        
        return bitmap_buffer_temp;
    }
    
    std::unique_ptr<Window> get_window_list(long &len)
    {
        long length;
        int size;
        Atom type;
        Window *window_list = (Window *)get_window_property(root_, 
            _NET_CLIENT_LIST, length, type, size);
        len = length;
        return std::unique_ptr<Window>(window_list);
    }
    
    bool check_window(Window win, uint8_t map_state)
    {
        Atom actual_type;
        int actual_format;
        ulong n_items, bytes_after;
        unsigned char *prop;
        
        auto status = XGetWindowProperty(display_, win, _NET_WM_WINDOW_TYPE, 0, 
            1024, false, XA_ATOM, &actual_type, &actual_format, &n_items, 
            &bytes_after, &prop);
        
        if(status != Success && actual_format != 32)
            return false;
        
        auto prop_scoped = x_ptr<unsigned char>(prop);
        
        Atom *win_type = (Atom *)(prop);
        
        return  map_state == IsViewable && 
                *win_type == _NET_WM_WINDOW_TYPE_NORMAL;
    }
    
    // TODO not thread-safe
    std::vector<window_client_t> update_window_list()
    {
        std::vector<window_client_t> win_client_list; // TODO store this client list locally and return const to the user 
        
        window_info_mapper.clear();
        
        long window_list_len;
        auto window_list_scoped = get_window_list(window_list_len);
        Window *window_list = window_list_scoped.get();
        
        uint8_t id = 0;
        for(int i = 0; i < window_list_len; i++)
        {
            if(!window_list[i])
                continue;
            try
            {
                XWindowAttributes attr;
                auto status = XGetWindowAttributes(display_, window_list[i], &attr);
                
                if(status && check_window(window_list[i], attr.map_state))
                {
                    auto name = get_name(window_list[i]);
                    
                    auto win_client = window_client_t { id, name };
                    win_client_list.push_back(win_client);
                    
                    auto info = std::make_shared<window_info_t>();
                    
                    info->name      = name;
                    info->win       = window_list[i];
                    info->visual    = attr.visual;
                    
                    window_info_mapper[id] = info;
                    
                    id++;
                }
            }
            catch(...)
            {
            }
        }
        
//        std::cout << win_client_list.size() << std::endl;
        
        return win_client_list;
    }
    
    const bool is_window_active(Window win)
    {
        long length;
        int size;
        Atom type;
        Window *active_win = (Window *)get_window_property(root_, 
            _NET_ACTIVE_WINDOW, length, type, size);
//        std::cout << (*active_win == win) << std::endl;
        return (*active_win == win);
    }
    
    // https://stackoverflow.com/a/30256233/10012118
    void focus_win(Window win)
    {
        XClientMessageEvent ev;
        ev.type = ClientMessage;
        ev.window = win;
        ev.message_type = _NET_ACTIVE_WINDOW;
        ev.format = 32;
        ev.data.l[0] = 1;
        ev.data.l[1] = CurrentTime;
        ev.data.l[2] = ev.data.l[3] = ev.data.l[4] = 0;
        XSendEvent(display_, root_, false,
            SubstructureRedirectMask | SubstructureNotifyMask, (XEvent *) &ev);
        
        flush();
    }
    
    void minimize_win(Window win)
    {
        XIconifyWindow(display_, win, screen_number);
        flush();
    }
    
    void toggle_win(Window win)
    {
        if(is_window_active(win))
            minimize_win(win);
        else
            focus_win(win);
    }
    
    void focus_window(uint8_t id)
    {
        try
        {
            focus_win(window_info_mapper.at(id)->win);
        }
        catch(const std::out_of_range &oor)
        {
        }
    }
    
    void minimize_window(uint8_t id)
    {
        try
        {
            minimize_win(window_info_mapper.at(id)->win);
        }
        catch(const std::out_of_range &oor)
        {
        }
    }
    
    void toggle_window(uint8_t id)
    {
        try
        {
            toggle_win(window_info_mapper.at(id)->win);
        }
        catch(const std::out_of_range &oor)
        {
        }
    }
}