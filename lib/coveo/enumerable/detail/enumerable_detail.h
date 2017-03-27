// Copyright (c) 2016, Coveo Solutions Inc.
// Distributed under the MIT license (see LICENSE).

// Implementation details of coveo::enumerable.

#ifndef COVEO_ENUMERABLE_DETAIL_H
#define COVEO_ENUMERABLE_DETAIL_H

#include <memory>
#include <type_traits>

namespace coveo {
namespace detail {

// Traits class used by coveo::enumerable. Provides information about types in a sequence.
// For sequences of references or std::reference_wrapper's, provides information about
// the referred type instead.
template<typename T>
struct seq_element_traits
{
    typedef T                               value_type;         // Type of values stored in sequence.
    typedef const value_type                const_value_type;   // Same as value_type, but const.
    typedef typename std::remove_cv<value_type>::type
                                            raw_value_type;     // Same as value_type, without const/volatile.
    
    typedef value_type*                     pointer;
    typedef value_type&                     reference;

    typedef const_value_type*               const_pointer;
    typedef const_value_type&               const_reference;
};
template<typename T> struct seq_element_traits<T&> : seq_element_traits<T> { };
template<typename T> struct seq_element_traits<T&&> : seq_element_traits<T> { };
template<typename T> struct seq_element_traits<std::reference_wrapper<T>> : seq_element_traits<T> { };

// Copies content of upopt_ if possible. Used by enumerable::const_iterator
void get_copied_upopt(...);
template<typename T>
auto get_copied_upopt(const std::unique_ptr<T>& copied_upopt)
    -> typename std::enable_if<std::is_copy_constructible<T>::value, std::unique_ptr<T>>::type
{
    std::unique_ptr<T> upopt;
    if (copied_upopt != nullptr) {
        upopt.reset(new T(*copied_upopt));
    }
    return upopt;
}
template<typename T>
auto get_copied_upopt(const std::unique_ptr<T>&)
    -> typename std::enable_if<!std::is_copy_constructible<T>::value, std::unique_ptr<T>>::type
{
    // No way to copy, simply return an empty upopt.
    // Such objects won't be able to use temp storage.
    return std::unique_ptr<T>();
}

} // detail
} // coveo

#endif // COVEO_ENUMERABLE_DETAIL_H
