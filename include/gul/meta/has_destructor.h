#ifndef GUL_META_HASDESTRUCTOR_H
#define GUL_META_HASDESTRUCTOR_H

#include <type_traits>

namespace gul
{
//

/**
 * @brief The has_destructor struct
 *
 * Taken from: https://stackoverflow.com/a/10722840
 *
 * This meta struct can be used to check if a class
 * has a destructor defined. Most classes will have a
 * destructor defined. But classes which are not
 * defined will return false, so you can use this
 * to check for the existance of a class
 *
 * eg:
 *
 * static_assert( gul::has_destructor< std::hash<MyClass> >::value, "T must be be hashable");
 */
template< typename T>
struct has_destructor
{
    /* Has destructor :) */
    template <typename A>
    static std::true_type test(decltype(std::declval<A>().~A()) *)
    {
        return std::true_type();
    }

    /* Has no destructor :( */
    template<typename A>
    static std::false_type test(...)
    {
        return std::false_type();
    }

    /* This will be either `std::true_type` or `std::false_type` */
    typedef decltype(test<T>(0)) type;

    static const bool value = type::value; /* Which is it? */
};

}

#endif
