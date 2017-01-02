// Copyright (c) 2016, Coveo Solutions Inc.
// Distributed under the MIT license (see LICENSE).

// C++ implementation of .NET's IEnumerable<T>.

#ifndef COVEO_ENUMERABLE_H
#define COVEO_ENUMERABLE_H

#include <coveo/enumerable/detail/enumerable_detail.h>

#include <cl/optional.h>

#include <cstddef>
#include <iterator>
#include <functional>
#include <memory>
#include <type_traits>

namespace coveo {

// Wrapper for an immutable, multipass, forward-only sequence of const elements.
// Uses a delegate to fetch the elements to return. The sequence supports iteration
// via the standard begin/cbegin and end/cend methods.
template<typename T>
class enumerable
{
public:
    // Types for elements in the sequence.
    typedef typename detail::seq_element_traits<T>::const_value_type    value_type;     // Type of element in the sequence.
    typedef typename detail::seq_element_traits<T>::raw_value_type      raw_value_type; // Raw /value_type/, not cv-qualified.
    typedef typename detail::seq_element_traits<T>::const_pointer       pointer;        // Pointer to a sequence element.
    typedef typename detail::seq_element_traits<T>::const_reference     reference;      // Reference to a sequence element.

    // Delegate that returns next element in sequence, or nullptr when done.
    // Receives a stable optional<T> each time that can be used to store next value.
    typedef std::function<pointer(cl::optional<T>&)>                    next_delegate;

    // Forward declaration of iterator class.
    class const_iterator;

private:
    next_delegate zero_;    // Next delegate which we will clone to iterate sequence.

private:
    // Trait to identify enumerable objects
    template<typename _T> struct is_enumerable : std::false_type { };
    template<typename _T> struct is_enumerable<enumerable<_T>> : std::true_type { };

public:
    // Default constructor over empty sequence
    enumerable()
        : zero_([](auto&&) -> pointer { return nullptr; }) { }

    // Constructor with next delegate
    template<typename F,
             typename = std::enable_if_t<!is_enumerable<std::decay_t<F>>::value, void>>
    enumerable(F&& next)
        : zero_(std::forward<F>(next)) { }

    // Access to beginning or end of sequence
    const_iterator begin() const {
        return const_iterator(*this, false);
    }
    const_iterator end() const {
        return const_iterator(*this, true);
    }
    
    const_iterator cbegin() const {
        return begin();
    }
    const_iterator cend() const {
        return end();
    }

public:
    // Iterator for the elements in an enumerable's sequence.
    class const_iterator : public std::iterator<std::forward_iterator_tag, value_type, std::ptrdiff_t, pointer, reference>
    {
    public:
        // Redefinition of types from std::iterator
        typedef typename std::iterator<std::forward_iterator_tag,
                                       typename enumerable<T>::value_type,
                                       std::ptrdiff_t,
                                       typename enumerable<T>::pointer,
                                       typename enumerable<T>::reference>::pointer      pointer;
        typedef typename std::iterator<std::forward_iterator_tag,
                                       typename enumerable<T>::value_type,
                                       std::ptrdiff_t,
                                       typename enumerable<T>::pointer,
                                       typename enumerable<T>::reference>::reference    reference;

    private:
        const enumerable<T>* pparent_;  // Parent enumerable or nullptr if unbound
        mutable next_delegate next_;    // Delegate to use to fetch elements
        mutable cl::optional<T> opt_;   // Optional storage passed to the delegate
        mutable pointer pcur_;          // Pointer to current element or nullptr when at end of sequence
        mutable bool has_cur_;          // Whether pcur_ has been loaded for current element
        size_t pos_;                    // Position in sequence, to compare iterators

        // Fetches value of pcur_ when copying/assigning from another iterator
        static pointer get_copied_pcur(const cl::optional<T>& our_opt,
                                       const cl::optional<T>& copied_opt,
                                       pointer copied_pcur)
        {
            pointer pcur = copied_pcur;
            if (copied_opt.has_value()) {
                reference rcopied = *copied_opt;
                pointer pcopied = std::addressof(rcopied);
                if (copied_pcur == pcopied) {
                    // Other iterator points to its internal object, point to ours
                    reference rours = *our_opt;
                    pcur = std::addressof(rours);
                }
            }
            return pcur;
        }

        // Fetches value of pcur_, late-loading it if needed
        pointer get_pcur() const {
            if (!has_cur_) {
                pcur_ = next_(opt_);
                has_cur_ = true;
            }
            return pcur_;
        }
    
    public:
        // Default constructor
        const_iterator()
            : pparent_(nullptr), next_(), opt_(), pcur_(nullptr), has_cur_(true), pos_(0) { }

        // Constructor from enumerable
        const_iterator(const enumerable<T>& parent, const bool is_end)
            : pparent_(&parent), next_(!is_end ? parent.zero_ : next_delegate()),
              opt_(), pcur_(nullptr), has_cur_(is_end), pos_(0) { }

        // Copy/move semantics
        const_iterator(const const_iterator& obj)
            : pparent_(obj.pparent_), next_(obj.next_), opt_(obj.opt_),
              pcur_(get_copied_pcur(opt_, obj.opt_, obj.pcur_)),
              has_cur_(obj.has_cur_), pos_(obj.pos_) { }
        const_iterator(const_iterator&& obj)
            : pparent_(obj.pparent_), next_(std::move(obj.next_)), opt_(std::move(obj.opt_)),
              pcur_(get_copied_pcur(opt_, obj.opt_, obj.pcur_)),
              has_cur_(obj.has_cur_), pos_(obj.pos_)
        {
            obj.pparent_ = nullptr;
            obj.opt_.reset();
            obj.pcur_ = nullptr;
            obj.has_cur_ = true;
            obj.pos_ = 0;
        }

        const_iterator& operator=(const const_iterator& obj) {
            if (this != &obj) {
                pparent_ = obj.pparent_;
                next_ = obj.next_;
                opt_ = obj.opt_;
                pcur_ = get_copied_pcur(opt_, obj.opt_, obj.pcur_);
                has_cur_ = obj.has_cur_;
                pos_ = obj.pos_;
            }
            return *this;
        }
        const_iterator& operator=(const_iterator&& obj) {
            pparent_ = obj.pparent_;
            next_ = std::move(obj.next_);
            opt_ = std::move(obj.opt_);
            pcur_ = get_copied_pcur(opt_, obj.opt_, obj.pcur_);
            has_cur_ = obj.has_cur_;
            pos_ = obj.pos_;

            obj.pparent_ = nullptr;
            obj.opt_.reset();
            obj.pcur_ = nullptr;
            obj.has_cur_ = true;
            obj.pos_ = 0;

            return *this;
        }
        
        // Element access
        reference operator*() const {
            return *get_pcur();
        }
        pointer operator->() const {
            return get_pcur();
        }
        
        // Move to next element (pre/post versions)
        const_iterator& operator++() {
            if (has_cur_) {
                pcur_ = nullptr;
                has_cur_ = false;
            } else {
                next_(opt_);
            }
            ++pos_;
            return *this;
        }
        const_iterator operator++(int) {
            const_iterator it(*this);
            ++*this;
            return it;
        }
        
        // Iterator comparison
        bool operator==(const const_iterator& obj) const {
            return pparent_ == obj.pparent_ &&
                   (get_pcur() == nullptr) == (obj.get_pcur() == nullptr) &&
                   (get_pcur() == nullptr || pos_ == obj.pos_);
        }
        bool operator!=(const const_iterator& obj) const {
            return !(*this == obj);
        }
    };

public:
    // Helper static methods

    // Returns enumerable over empty sequence.
    static enumerable<T> empty() {
        return enumerable<T>();
    }

    // Returns enumerable over sequence of one element. Stores the element (moving it if possible).
    template<typename U>
    static enumerable<T> for_one(U&& obj) {
        return [spobj = std::make_shared<value_type>(std::forward<U>(obj)),
                available = true](auto&&) mutable {
            pointer pobj = nullptr;
            if (available) {
                pobj = spobj.get();
                available = false;
            }
            return pobj;
        };
    }

    // Returns enumerable over sequence of one external element.
    static enumerable<T> for_one_ref(const T& obj) {
        return [&obj, available = true](auto&&) mutable {
            pointer pobj = nullptr;
            if (available) {
                pobj = std::addressof(obj);
                available = false;
            }
            return pobj;
        };
    }

    // Returns enumerable over sequence bound by two iterators.
    template<typename ItBeg, typename ItEnd>
    static enumerable<T> for_range(ItBeg&& ibeg, ItEnd&& iend) {
        return [it = std::forward<ItBeg>(ibeg),
                end = std::forward<ItEnd>(iend)](auto&&) mutable {
            pointer pobj = nullptr;
            if (it != end) {
                reference robj = *it;
                pobj = std::addressof(robj);
                ++it;
            }
            return pobj;
        };
    }

    // Returns enumerable over a container, optionally copying it.
    template<typename C>
    static enumerable<T> for_container(const C& cnt, const bool copy_cnt = false) {
        std::shared_ptr<const C> spcnt;
        const C* pcnt = std::addressof(cnt);
        if (copy_cnt) {
            spcnt = std::make_shared<const C>(cnt);
            pcnt = spcnt.get();
        }
        return [spcnt, it = std::begin(*pcnt),
                end = std::end(*pcnt)](auto&&) mutable {
            pointer pobj = nullptr;
            if (it != end) {
                reference robj = *it;
                pobj = std::addressof(robj);
                ++it;
            }
            return pobj;
        };
    }

    // Returns enumerable over a container, storing it internally (by moving it).
    template<typename C,
             typename = std::enable_if_t<!std::is_reference<C>::value, void>>
    static enumerable<T> for_container(C&& cnt) {
        auto spcnt = std::make_shared<const C>(std::move(cnt));
        return [spcnt, it = std::begin(*spcnt),
                end = std::end(*spcnt)](auto&&) mutable {
            pointer pobj = nullptr;
            if (it != end) {
                reference robj = *it;
                pobj = std::addressof(robj);
                ++it;
            }
            return pobj;
        };
    }

    // Returns enumerable over dynamic array.
    static enumerable<T> for_array(pointer const parr, const size_t siz) {
        // This works even if siz is 0 or if parr is nullptr.
        return for_range(parr, parr + siz);
    }
};

// Helper functions corresponding to the helper static methods in enumerable.

// Returns enumerable for sequence of one element, stored internally (moved if possible).
template<typename U>
auto enumerate_one(U&& obj) {
    return enumerable<typename detail::seq_element_traits<std::decay_t<U>>::raw_value_type>::for_one(std::forward<U>(obj));
}

// Returns enumerable for sequence of one element, stored externally.
template<typename T>
auto enumerate_one_ref(const T& obj) {
    return enumerable<T>::for_one_ref(obj);
}

// Returns enumerable for sequence defined by two iterators.
template<typename ItBeg, typename ItEnd>
auto enumerate_range(ItBeg&& ibeg, ItEnd&& iend) {
    return enumerable<typename detail::seq_element_traits<decltype(*std::declval<ItBeg>())>::raw_value_type>::for_range(
        std::forward<ItBeg>(ibeg), std::forward<ItEnd>(iend));
}

// Returns enumerable for container, optionally copying container internally.
template<typename C>
auto enumerate_container(const C& cnt, const bool copy_cnt = false) {
    return enumerable<typename detail::seq_element_traits<decltype(*std::begin(std::declval<const C>()))>::raw_value_type>::for_container(
        cnt, copy_cnt);
}

// Returns enumerable for container, stored internally (by moving it).
template<typename C,
         typename = std::enable_if_t<!std::is_reference<C>::value, void>>
auto enumerate_container(C&& cnt) {
    return enumerable<typename detail::seq_element_traits<decltype(*std::begin(std::declval<const C>()))>::raw_value_type>::for_container(
        std::move(cnt));
}

// Returns enumerable for dynamic array.
template<typename T>
auto enumerate_array(const T* const parr, const size_t siz) {
    // This works even if siz is 0 or if parr is nullptr.
    return enumerate_range(parr, parr + siz);
}

} // coveo

#endif // COVEO_ENUMERABLE_H
