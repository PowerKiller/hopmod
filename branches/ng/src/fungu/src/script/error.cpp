/*   
 *   The Fungu Scripting Engine Library
 *   
 *   Copyright (c) 2008-2009 Graham Daws.
 *
 *   Distributed under a BSD style license (see accompanying file LICENSE.txt)
 */

#ifdef BOOST_BUILD_PCH_ENABLED
#include "fungu/script/pch.hpp"
#endif

#include "fungu/script/error.hpp"
#include "fungu/script/env.hpp"
#include <sstream>

namespace fungu{
namespace script{

error::error(error_code code)
 :m_code(code)
{
    
}

error_code error::get_error_code()const
{
    return m_code;
}

const boost::any & error::get_error_arguments()const
{
    return m_args;
}

std::string error::get_error_message()const
{
    std::stringstream message;
    
    switch(m_code)
    {
        case EXPECTED_SYMBOL:
        {
            boost::tuple<char> args = boost::any_cast<boost::tuple<char> >(m_args);
            message<<"parse error expecting '"<<boost::get<0>(args)<<"'";
            break;
        }
        case UNEXPECTED_SYMBOL:
        {
            boost::tuple<char> args = boost::any_cast<boost::tuple<char> >(m_args);
            message<<"parse error unexpected '"<<boost::get<0>(args)<<"'";
            break;
        }
        case UNEXPECTED_EOF: return "unexpected end of file";
        case EXTERNAL_PARSE_ERROR: return "external parse error";
        case NOT_ENOUGH_ARGUMENTS: return "not enough arguments were given";
        case TOO_MANY_ARGUMENTS: return "too many arguments were given";
        case MAXARGS: return "exceeded the script engine's argument list size limit";
        case BAD_CAST: return "value could not be type casted";
        case NO_CAST: return "type has no supported representation";
        case INVALID_TYPE: return "object of the wrong type was given";
        case INVALID_VALUE:
        {
            boost::tuple<std::string> args = boost::any_cast<boost::tuple<std::string> >(m_args);
            return boost::get<0>(args);
        }
        case INTEGER_OVERFLOW: return "integer overflow";
        case INTEGER_UNDERFLOW: return "integer underflow";
        case DIVIDE_BY_ZERO: return "divide by zero";
        case NO_VALUE: return "no value";
        case UNKNOWN_SYMBOL:
        {
            boost::tuple<std::string> args = boost::any_cast<boost::tuple<std::string> >(m_args);
            message<<"unknown symbol '"<<boost::get<0>(args)<<"'";
            break;
        }
        case NO_BIND: return "cannot override";
        case NO_WRITE: return "cannot write";
        case NO_READ: return "cannot read";
        case PERMISSION_DENIED: return "permission denied";
        case OPERATION_ERROR: return "function failure";
        case HIT_RECURSION_LIMIT: return "hit the recursion limit";
        case MEMBER_ACCESS_CHAIN_LIMIT: return "too many members being accessed";
        case SCRIPT_THROW: 
        {
            boost::tuple<std::string> args = boost::any_cast<boost::tuple<std::string> >(m_args);
            message<<boost::get<0>(args);
            break;
        }
        #ifdef FUNGU_WITH_LUA
        case LUA_ERROR:
        {
            boost::tuple<lua_State *> args = boost::any_cast<boost::tuple<lua_State *> >(m_args);
            message<<lua_tostring(boost::get<0>(args), -1);
            break;
        }
        #endif
        default: return "unknown error";
    }
    
    return message.str();
}

error_trace::error_trace(const error & key, const_string arg, source_context * src_ctx)
 :m_key(key),
  m_head_info(NULL),
  m_arg(arg),
  m_source_ctx(src_ctx)
{
    
}

error_trace::error_trace(error_trace * head_info, const_string arg, source_context * src_ctx)
 :m_key(head_info->m_key),
  m_head_info(head_info),
  m_arg(arg),
  m_source_ctx(src_ctx)
{
    
}

error_trace::~error_trace()
{
    delete m_head_info;
    delete m_source_ctx;
}

const error & error_trace::get_error()const
{
    return m_key;
}

const error_trace * error_trace::get_parent_info()const
{
    return m_head_info;
}

const error_trace * error_trace::get_root_info()const
{
    const error_trace * node = this;
    while(node->m_head_info) node = node->m_head_info;
    return node;
}

const source_context * error_trace::get_source_context()const
{
    return m_source_ctx;
}

} //namespace script
} //namespace fungu
