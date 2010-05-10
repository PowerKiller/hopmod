/*   
 *   The Fungu Scripting Engine Library
 *   
 *   Copyright (c) 2008-2009 Graham Daws.
 *
 *   Distributed under a BSD style license (see accompanying file LICENSE.txt)
 */
#ifndef FUNGU_DYNAMIC_CALL_HPP
#define FUNGU_DYNAMIC_CALL_HPP

#include "type_tag.hpp"

#include <exception>
#include <boost/type_traits.hpp>

namespace fungu{

#ifndef HAS_MISSING_ARGS_CLASS
#define HAS_MISSING_ARGS_CLASS
class missing_args:public std::exception
{
public:
    missing_args(int given,int needed)throw()
     :m_given(given),m_needed(needed){}
    ~missing_args()throw(){}
    const char * what()const throw(){return "";}
private:
    int m_given;
    int m_needed;
};
#endif

namespace detail{

template<int N> struct arity_tag{};

template<typename T> 
struct arg_holder{
    typedef typename boost::mpl::if_<
        boost::mpl::bool_<boost::is_reference<T>::value>,
        typename boost::remove_const<typename boost::remove_reference<T>::type>::type,
        T
    >::type type;
};

#define DYNAMIC_CALL_FUNCTION(nargs) \
    template<typename FunctionTraits,typename Functor,typename ArgumentsContainer,typename Serializer> \
    inline typename ArgumentsContainer::value_type dynamic_call(Functor function, ArgumentsContainer & args, \
    Serializer & serializer, type_tag<typename FunctionTraits::result_type>, arity_tag<nargs>)

#define DYNAMIC_CALL_VOID_FUNCTION(nargs) \
    template<typename FunctionTraits,typename Functor,typename ArgumentsContainer,typename Serializer> \
    inline typename ArgumentsContainer::value_type dynamic_call(Functor function, ArgumentsContainer & args, \
    Serializer & serializer, type_tag<void>, arity_tag<nargs>)

#define DYNAMIC_CALL_ARGUMENT(name) \
    typename arg_holder<typename FunctionTraits::name##_type>::type name = serializer.deserialize(args.front(), type_tag<typename arg_holder<typename FunctionTraits::name##_type>::type>()); \
    args.pop_front();

#define DYNAMIC_CALL_ARGS_SIZE_CHECK(n) \
    if(args.size() < n) throw missing_args(args.size(), n);

DYNAMIC_CALL_FUNCTION(0)
{
    return serializer.serialize(function());
}

DYNAMIC_CALL_VOID_FUNCTION(0)
{
    function();
    return serializer.get_void_value();
}

DYNAMIC_CALL_FUNCTION(1)
{
    DYNAMIC_CALL_ARGS_SIZE_CHECK(1);
    DYNAMIC_CALL_ARGUMENT(arg1);
    return serializer.serialize(function(arg1));
}

DYNAMIC_CALL_VOID_FUNCTION(1)
{
    DYNAMIC_CALL_ARGS_SIZE_CHECK(1);
    DYNAMIC_CALL_ARGUMENT(arg1);
    function(arg1);
    return serializer.get_void_value();
}

DYNAMIC_CALL_FUNCTION(2)
{
    DYNAMIC_CALL_ARGS_SIZE_CHECK(2);
    DYNAMIC_CALL_ARGUMENT(arg1);
    DYNAMIC_CALL_ARGUMENT(arg2);
    return serializer.serialize(function(arg1,arg2));
}

DYNAMIC_CALL_VOID_FUNCTION(2)
{
    DYNAMIC_CALL_ARGS_SIZE_CHECK(2);
    DYNAMIC_CALL_ARGUMENT(arg1);
    DYNAMIC_CALL_ARGUMENT(arg2);
    function(arg1,arg2);
    return serializer.get_void_value();
}

DYNAMIC_CALL_FUNCTION(3)
{
    DYNAMIC_CALL_ARGS_SIZE_CHECK(3);
    DYNAMIC_CALL_ARGUMENT(arg1);
    DYNAMIC_CALL_ARGUMENT(arg2);
    DYNAMIC_CALL_ARGUMENT(arg3);
    return serializer.serialize(function(arg1,arg2,arg3));
}

DYNAMIC_CALL_VOID_FUNCTION(3)
{
    DYNAMIC_CALL_ARGS_SIZE_CHECK(3);
    DYNAMIC_CALL_ARGUMENT(arg1);
    DYNAMIC_CALL_ARGUMENT(arg2);
    DYNAMIC_CALL_ARGUMENT(arg3);
    function(arg1,arg2,arg3);
    return serializer.get_void_value();
}

DYNAMIC_CALL_FUNCTION(4)
{
    DYNAMIC_CALL_ARGS_SIZE_CHECK(4);
    DYNAMIC_CALL_ARGUMENT(arg1);
    DYNAMIC_CALL_ARGUMENT(arg2);
    DYNAMIC_CALL_ARGUMENT(arg3);
    DYNAMIC_CALL_ARGUMENT(arg4);
    return serializer.serialize(function(arg1,arg2,arg3,arg4));
}

DYNAMIC_CALL_VOID_FUNCTION(4)
{
    DYNAMIC_CALL_ARGS_SIZE_CHECK(4);
    DYNAMIC_CALL_ARGUMENT(arg1);
    DYNAMIC_CALL_ARGUMENT(arg2);
    DYNAMIC_CALL_ARGUMENT(arg3);
    DYNAMIC_CALL_ARGUMENT(arg4);
    function(arg1,arg2,arg3,arg4);
    return serializer.get_void_value();
}

} //namespace detail

template<typename Signature,typename Functor,typename ArgumentsContainer,typename Serializer>
inline
typename ArgumentsContainer::value_type dynamic_call(
    Functor function, 
    ArgumentsContainer & args,
    Serializer & serializer)
{
    typedef boost::function_traits<Signature> FunctionTraits;
    return detail::dynamic_call<FunctionTraits>(function,args,serializer,type_tag<typename FunctionTraits::result_type>(),detail::arity_tag<FunctionTraits::arity>());
}

} //namespace fungu

#endif