#ifndef HOPMOD_LUA_EVENT_HPP
#define HOPMOD_LUA_EVENT_HPP

#include "push_function.hpp"
#include <cassert>
#include <iostream>

namespace lua{

class event_base
{
public:
    event_base(const char * text_id, int numeric_id = -1);
    const char * text_id()const;
    int numeric_id()const;
    static int assign_numeric_id();
private:
    const char * m_text_id;
    int m_numeric_id;
};

class event_environment
{
public:
    event_environment(lua_State *, 
                      void (* log_error_function)(const char *) = NULL, 
                      lua_CFunction error_function = NULL);
    void register_event_idents(event_base **);
    lua_State * push_listeners_table(const char * text_id, int num_id);
    bool push_error_function();
    void log_error(const char * text_id, const char * message);
    bool is_ready();
private:
    lua_State * m_state;
    int m_text_id_index;
    int m_numeric_id_index;
    void (* m_log_error_function)(const char *);
    lua_CFunction m_error_function;
};

template<typename Tuple>
class event:public event_base
{
public:
    event(const char * text_id, int numeric_id = -1)
     :event_base(text_id, numeric_id)
    {
        
    }
    
    bool operator()(event_environment & environment, const Tuple & args)
    {
        if(environment.is_ready() == false) return false;
        
        lua_State * L = environment.push_listeners_table(text_id(), numeric_id());
        assert(L && lua_type(L, -1) == LUA_TTABLE);
        
        bool using_error_function = environment.push_error_function();
        int error_function = (using_error_function ? lua_gettop(L) : 0);
        
        bool prevent_default = false;
        
        lua_pushnil(L);
        while (lua_next(L, -2) != 0)
        {
            if(lua_type(L, -1) == LUA_TFUNCTION)
            {
                lua::push(L, args);
                if(lua_pcall(L, boost::tuples::length<Tuple>::value, 1, error_function) == 0)
                    prevent_default = prevent_default || lua_toboolean(L, -1);
                else
                    environment.log_error(text_id(), lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }
        
        if(using_error_function)
            lua_pop(L, 1);
        
        return prevent_default;
    }
};

} //namespace lua

#endif

