/*
    Copyright (C) 2009 Graham Daws
*/
#ifdef BOOST_BUILD_PCH_ENABLED
#include "pch.hpp"
#endif

#include "hopmod.hpp"
#include "scoped_setting.hpp"

#include <fungu/script/lua/object_wrapper.hpp>
#include <fungu/script/lua/lua_function.hpp>
#include <fungu/dynamic_cast_derived.hpp>
#include <fungu/script/variable.hpp>
#include <fungu/script/parse_array.hpp>
using namespace fungu;

#include <sstream>
#include <iostream>
#include <time.h>

static void create_server_namespace(lua_State *);
static void bind_object_to_lua(const_string, script::env::object *);
static inline int server_interface_index(lua_State *);
static inline int server_interface_newindex(lua_State *);
static inline void push_server_table(lua_State *);
static inline void register_to_server_namespace(lua_State *,lua_CFunction,const char *);
static int parse_list(lua_State *);

static script::env * env = NULL;

static const char * const SERVER_NAMESPACE = "server";

static int server_namespace_ref = 0;
static int server_namespace_index_ref = 0;

static bool binding_object_to_lua = false;
static bool binding_object_to_cubescript = false;

void init_scripting()
{
    // Initialize our CubeScript environment
    assert(!env);
    env = new script::env;
    // Load CubeScript's core library functions into env
    script::load_corelib(*env);
    
    // Initialize our Lua environment
    lua_State * L = luaL_newstate();
    luaL_openlibs(L);
    env->set_lua_state(L);
    
    // Global objects created and bound in our CubeScript environment are also
    // bound to the Lua environment in the server namespace.
    env->set_bind_observer(bind_object_to_lua);
    
    // Required for Lua to call CubeScript functions
    lua_pushlightuserdata(L, env);
    lua_setfield(L, LUA_REGISTRYINDEX, "fungu_script_env");
    
    create_server_namespace(L);

    // Utility functions
    register_lua_function(&parse_list, "parse_list");
}

static void create_server_namespace(lua_State * L)
{
    lua_newtable(L);
    
    lua_newtable(L);
    
    lua_pushcclosure(L, server_interface_index, 0);
    lua_setfield(L, -2, "__index");
    
    lua_pushcclosure(L, server_interface_newindex, 0);
    lua_setfield(L, -2, "__newindex");
    
    lua_setmetatable(L, -2);
    
    lua_pushvalue(L, -1);
    lua_setglobal(L, SERVER_NAMESPACE);
    
    // Create a reference to the server namespace table for quick access later
    lua_pushvalue(L, -1);
    server_namespace_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    lua_newtable(L);
    lua_pushstring(L, "index");
    lua_pushvalue(L, -2);
    lua_rawset(L, -4);
    
    server_namespace_index_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    lua_pop(L, 1); // pop server table
}

void shutdown_scripting()
{
    assert(env->get_lua_state());
    lua_State * L = env->get_lua_state();
    env->set_lua_state(NULL);
    
    delete env;
    env = NULL;
    
    luaL_unref(L, LUA_REGISTRYINDEX, server_namespace_index_ref);
    luaL_unref(L, LUA_REGISTRYINDEX, server_namespace_ref);
    
    lua_close(L);
}

script::env & get_script_env()
{
    return *env;
}

int server_interface_index(lua_State * L)
{
    // index(table, key)
    const char * key = luaL_checkstring(L, 2);
    
    lua_rawgeti(L, LUA_REGISTRYINDEX, server_namespace_index_ref);
    if(lua_type(L,-1) != LUA_TTABLE) return luaL_error(L, "missing server.index table");
    
    lua_getfield(L, -1, key);
    
    script::env::object * obj = script::lua::get_object(L, -1);
    
    if(obj && obj->get_object_type() == script::env::object::DATA_OBJECT)
    {
        lua_pop(L, 1);
        obj->value(L); //push value of object onto lua stack
    }
    
    return 1;
}

int server_interface_newindex(lua_State * L)
{
    // newindex(table, key, value)
    bool is_func = (lua_type(L, -1) == LUA_TFUNCTION);
    std::string key = lua_tostring(L, -2);
    
    lua_rawgeti(L, LUA_REGISTRYINDEX, server_namespace_index_ref);
    if(lua_type(L, -1) != LUA_TTABLE) return luaL_error(L, "missing server.index table");
    
    lua_pushvalue(L, -2);
    lua_setfield(L, -2, key.c_str()); // server.index[key]=value

    if(binding_object_to_lua) return 0;
    
    script::env::object * hangingObj = NULL; //object to delete if exception is thrown
    
    try
    {
        if(is_func)
        {
            scoped_setting<bool> setting(binding_object_to_cubescript, true);
            hangingObj = new script::lua::lua_function(L, -1, key.c_str());
            hangingObj->set_adopted();
            env->bind_global_object(hangingObj, const_string(key));
            hangingObj = NULL;
        }
        else
        {
            lua_pushvalue(L, -2);
            script::env::object * obj = env->lookup_global_object(key);
            if(obj) obj->assign(script::lua::get_argument_value(L));
            else
            {
                scoped_setting<bool> setting(binding_object_to_cubescript, true);
                script::any_variable * newvar = new script::any_variable;
                hangingObj = newvar;
                newvar->set_adopted();
                newvar->assign(script::lua::get_argument_value(L));
                env->bind_global_object(newvar, const_string(key));
                hangingObj = NULL;
            }
        }
    }
    catch(script::error_trace * errinfo)
    {
        delete hangingObj;
        return luaL_error(L,get_script_error_message(errinfo).c_str());
    }
    catch(script::error err)
    {
        delete hangingObj;
        return luaL_error(L,err.get_error_message().c_str());
    }
    
    return 0;
}

void push_server_table(lua_State * L)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, server_namespace_ref);
}

void register_to_server_namespace(lua_State * L, lua_CFunction func, const char * name)
{
    push_server_table(L);
    lua_pushcclosure(L, func, 0);
    lua_setfield(L, -2, name);
    lua_pop(L, 1);
}

void bind_object_to_lua(const_string id, script::env::object * obj)
{
    if(binding_object_to_cubescript) return;
    lua_State * L = env->get_lua_state();
    assert(!binding_object_to_lua);
    scoped_setting<bool> setting(binding_object_to_lua, true);
    push_server_table(L);
    script::lua::register_object(L, obj, id.copy().c_str());
    lua_pop(L, 1);
}

std::string get_script_error_message(script::error_trace * errinfo)
{
    const script::source_context * source = errinfo->get_root_info()->get_source_context();
    std::stringstream out;
    script::error error = errinfo->get_error();
    error.get_error_message();
    
    char timestamp[32];
    time_t now = time(NULL);
    strftime(timestamp, sizeof(timestamp), "[%a %d %b %X] ", localtime(&now));
    
    out<<timestamp;
    if(source) out<<source->get_location()<<":"<<source->get_line_number()<<": ";
    out<<error.get_error_message();
    
    delete errinfo;
    return out.str();
}

void report_script_error(script::error_trace * errinfo)
{
    std::cerr<<get_script_error_message(errinfo)<<std::endl;
}

/*
    Register Lua C Function to server.index table and is only accessible from Lua
*/
void register_lua_function(lua_CFunction func, const char * name)
{
    lua_State * L = env->get_lua_state();
    
    lua_rawgeti(L, LUA_REGISTRYINDEX, server_namespace_index_ref);
    
    lua_pushcclosure(L, func, 0);
    lua_setfield(L, -2, name);
    
    lua_pop(L, 1);
}

/*
    Convert an array/list of elements written in CubeScript code to a Lua array.
*/
int parse_list(lua_State * L)
{
    size_t liststrlen;
    const char * liststr = luaL_checklstring(L, 1, &liststrlen);
    
    std::vector<const_string> elements;
    
    try
    {
        script::env::frame callframe(env);
        script::parse_array<std::vector<const_string>,true>(const_string(liststr,liststr + liststrlen -1), &callframe, elements);
    }
    catch(script::error err)
    {
        return luaL_error(L, err.get_error_message().c_str());
    }
    
    lua_newtable(L);
    int i = 1;
    for(std::vector<const_string>::const_iterator it = elements.begin(); it != elements.end(); it++, i++)
    {
        lua_pushinteger(L, i);
        lua_pushlstring(L, it->begin(), it->length());
        lua_settable(L, -3);
    }
    
    return 1;
}

void unset_global(const char * name)
{
    script::env::symbol * symbol = env->lookup_symbol(const_string(name, name + strlen(name) - 1));
    if(symbol) symbol->unset_global_object();
}
