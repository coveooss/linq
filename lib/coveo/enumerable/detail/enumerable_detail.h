// Copyright (c) 2016, Coveo Solutions Inc.
// Distributed under the MIT license (see LICENSE).

// Implementation details of coveo::enumerable.

#ifndef COVEO_ENUMERABLE_DETAIL_H
#define COVEO_ENUMERABLE_DETAIL_H

#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace coveo {

// Forward declaration of enumerable, for type traits
template<typename> class enumerable;

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

// Traits class used to identify enumerable objects.
template<typename> struct is_enumerable : std::false_type { };
template<typename T> struct is_enumerable<coveo::enumerable<T>> : std::true_type { };

// Traits class used to identify reference_wrappers.
template<typename> struct is_reference_wrapper : std::false_type { };
template<typename T> struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type { };

// Type trait that can be used to know if std::begin(T) is valid.
// Detects both std::begin specializations and begin methods.
template<typename T>
class has_begin
{
    static_assert(sizeof(std::int_least8_t) != sizeof(std::int_least32_t),
                  "has_begin only works if int_least8_t has a different size than int_least32_t");

    template<typename C> static std::int_least8_t  test(decltype(std::begin(std::declval<C>()))*);  // Will be selected if std::begin(C) works
    template<typename C> static std::int_least32_t test(...);                                       // Will be selected otherwise
public:
    static const bool value = sizeof(test<T>(nullptr)) == sizeof(std::int_least8_t);
};

// Type trait that can be used to know if std::end(T) is valid.
// Detects both std::end specializations and end methods.
template<typename T>
class has_end
{
    static_assert(sizeof(std::int_least8_t) != sizeof(std::int_least32_t),
                  "has_end only works if int_least8_t has a different size than int_least32_t");

    template<typename C> static std::int_least8_t  test(decltype(std::end(std::declval<C>()))*);    // Will be selected if std::end(C) works
    template<typename C> static std::int_least32_t test(...);                                       // Will be selected otherwise
public:
    static const bool value = sizeof(test<T>(nullptr)) == sizeof(std::int_least8_t);
};

// Type trait that can be used to know if a type has a size() const method
// that returns a non-void result.
template<typename T>
class has_size_const_method
{
    static_assert(sizeof(std::int_least8_t) != sizeof(std::int_least32_t),
                  "has_size_const_method only works if int_least8_t has a different size than int_least32_t");

    template<typename C> static std::int_least8_t  test(typename std::enable_if<!std::is_void<decltype(std::declval<const C>().size())>::value, void*>::type);  // Will be selected if C has size() that does not return void
    template<typename C> static std::int_least32_t test(...);                                                                                                   // Will be selected otherwise
public:
    static const bool value = sizeof(test<T>(nullptr)) == sizeof(std::int_least8_t);
};

// Copies content of upopt_ if possible. Used by enumerable's iterators.
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

// Assigns a value in a unique_ptr, by copying to an existing value if possible,
// otherwise by creating a new instance if not assignable.
void assign_in_upopt(...);
template<typename T, typename U>
auto assign_in_upopt(std::unique_ptr<T>& upopt, U&& obj) -> typename std::enable_if<(std::is_lvalue_reference<U>::value && std::is_copy_assignable<typename std::decay<U>::type>::value) ||
                                                                                    (!std::is_reference<U>::value && std::is_move_assignable<typename std::decay<U>::type>::value), void>::type
{
    if (upopt != nullptr) {
        *upopt = std::forward<U>(obj);
    } else {
        upopt.reset(new T(std::forward<U>(obj)));
    }
}
template<typename T, typename U>
auto assign_in_upopt(std::unique_ptr<T>& upopt, U&& obj) -> typename std::enable_if<(!std::is_copy_assignable<typename std::decay<U>::type>::value || !std::is_lvalue_reference<U>::value) &&
                                                                                    (!std::is_move_assignable<typename std::decay<U>::type>::value || std::is_reference<U>::value), void>::type
{
    // Can't assign, create new instance
    upopt.reset(new T(std::forward<U>(obj)));
}

// Fetches reference from given iterator and returns a pointer to it.
// If iterator doesn't return references, fetches a copy and stores it in given unique_ptr.
void get_ref_from_iterator(...);
template<typename Ref, typename Ptr, typename T, typename It>
auto get_ref_from_iterator(It&& it, std::unique_ptr<T>&) -> typename std::enable_if<std::is_reference<decltype(*it)>::value ||
                                                                                    is_reference_wrapper<decltype(*it)>::value, Ptr>::type
{
    Ref robj = *it;
    return std::addressof(robj);
}
template<typename Ref, typename Ptr, typename T, typename It>
auto get_ref_from_iterator(It&& it, std::unique_ptr<T>& upopt) -> typename std::enable_if<!std::is_reference<decltype(*std::forward<It>(it))>::value &&
                                                                                          !is_reference_wrapper<decltype(*std::forward<It>(it))>::value, Ptr>::type
{
    assign_in_upopt(upopt, *std::forward<It>(it));
    return upopt.get();
}

// Returns a size delegate for a sequence if iterators can provide that information quickly.
void get_size_delegate_for_iterators(...);
template<typename It>
auto get_size_delegate_for_iterators(const It& beg, const It& end) -> typename std::enable_if<std::is_base_of<std::random_access_iterator_tag,
                                                                                                              typename std::iterator_traits<It>::iterator_category>::value,
                                                                                              std::function<std::size_t()>>::type
{
    return [beg, end]() -> std::size_t { return std::distance(beg, end); };
};
template<typename It>
auto get_size_delegate_for_iterators(const It&, const It&) -> typename std::enable_if<!std::is_base_of<std::random_access_iterator_tag,
                                                                                                       typename std::iterator_traits<It>::iterator_category>::value,
                                                                                      std::function<std::size_t()>>::type
{
    // No way to quickly determine size, don't try
    return nullptr;
};

} // detail
} // coveo

#endif // COVEO_ENUMERABLE_DETAIL_H
