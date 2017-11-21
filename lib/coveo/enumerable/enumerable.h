// Copyright (c) 2016, Coveo Solutions Inc.
// Distributed under the MIT license (see LICENSE).

// C++ implementation of .NET's IEnumerable<T>.

#ifndef COVEO_ENUMERABLE_H
#define COVEO_ENUMERABLE_H

#include <coveo/enumerable/detail/enumerable_detail.h>

#include <cstddef>
#include <iterator>
#include <functional>
#include <memory>
#include <type_traits>

namespace coveo {

// Wrapper for a multipass, forward-only sequence of elements.
// Uses a delegate to fetch the elements to return. The sequence
// supports iteration via the standard begin/end methods.
template<typename T>
class enumerable
{
    // Allows compatible enumerables to be friend with each other
    template<typename> friend class enumerable;

public:
    // Types for elements in the sequence.
    typedef typename detail::seq_element_traits<T>::value_type          value_type;         // Type of element in the sequence.
    typedef typename detail::seq_element_traits<T>::raw_value_type      raw_value_type;     // Raw /value_type/, not cv-qualified.
    typedef typename detail::seq_element_traits<T>::pointer             pointer;            // Pointer to a sequence element.
    typedef typename detail::seq_element_traits<T>::reference           reference;          // Reference to a sequence element.

    typedef typename detail::seq_element_traits<T>::const_value_type    const_value_type;   // Const version of /value_type/.
    typedef typename detail::seq_element_traits<T>::const_pointer       const_pointer;      // Const version of /pointer/.
    typedef typename detail::seq_element_traits<T>::const_reference     const_reference;    // Const version of /reference/.

    // Delegate that returns next element in sequence, or nullptr when done.
    // Receives a stable unique_ptr<raw_value_type> each time that can be used to store next value.
    typedef std::function<pointer(std::unique_ptr<raw_value_type>&)>    next_delegate;

    // Delegate that returns number of elements in sequence.
    typedef std::function<std::size_t()>                                size_delegate;

    // Forward declaration of iterator classes.
    class iterator;
    class const_iterator;

private:
    next_delegate zero_;    // Next delegate which we will clone to iterate sequence.
    size_delegate size_;    // Optional size delegate.

public:
    // Default constructor over empty sequence
    enumerable()
        : zero_([](std::unique_ptr<raw_value_type>&) -> pointer { return nullptr; }),
          size_([]() -> std::size_t { return 0; }) { }

    // Constructor with next delegate and optional size delegate
    template<typename F,
             typename _S = std::nullptr_t,
             typename = typename std::enable_if<!detail::is_enumerable<typename std::decay<F>::type>::value &&
                                                (!detail::has_begin<typename std::decay<F>::type>::value ||
                                                 !detail::has_end<typename std::decay<F>::type>::value ||
                                                 !detail::has_size_const_method<typename std::decay<F>::type>::value), void>::type>
    enumerable(F&& next, _S&& siz = nullptr)
        : zero_(std::forward<F>(next)), size_(std::forward<_S>(siz)) { }

    // Constructor with container
    template<typename C,
             typename = typename std::enable_if<!detail::is_enumerable<typename std::decay<C>::type>::value &&
                                                detail::has_begin<typename std::decay<C>::type>::value &&
                                                detail::has_end<typename std::decay<C>::type>::value &&
                                                detail::has_size_const_method<typename std::decay<C>::type>::value, void>::type>
    enumerable(C&& cnt)
        : zero_(), size_() { *this = for_container(std::forward<C>(cnt)); }

    // Constructors allowing non-const to const construction
    template<typename U,
             typename = typename std::enable_if<!std::is_const<U>::value &&
                                                std::is_same<T, typename std::add_const<U>::type>::value, void>::type>
    enumerable(const enumerable<U>& e)
        : zero_(e.zero_), size_(e.size_) { }
    template<typename U,
             typename = typename std::enable_if<!std::is_const<U>::value &&
                                                std::is_same<T, typename std::add_const<U>::type>::value, void>::type>
    enumerable(enumerable<U>&& e)
        : zero_(std::move(e.zero_)), size_(std::move(e.size_)) { }

    // Assignment operators for non-const to const assignment
    template<typename U,
             typename = typename std::enable_if<!std::is_const<U>::value &&
                                                std::is_same<T, typename std::add_const<U>::type>::value, void>::type>
    enumerable& operator=(const enumerable<U>& e) {
        zero_ = e.zero_;
        size_ = e.size_;
        return *this;
    }
    template<typename U,
             typename = typename std::enable_if<!std::is_const<U>::value &&
                                                std::is_same<T, typename std::add_const<U>::type>::value, void>::type>
    enumerable& operator=(enumerable<U>&& e) {
        zero_ = std::move(e.zero_);
        size_ = std::move(e.size_);
        return *this;
    }

    // Access to beginning or end of sequence
    iterator begin() const {
        return iterator(*this, false);
    }
    const_iterator cbegin() const {
        return const_iterator(*this, false);
    }

    iterator end() const {
        return iterator(*this, true);
    }
    const_iterator cend() const {
        return const_iterator(*this, true);
    }

    // Access to size of sequence
    bool has_fast_size() const {
        // If we have a delegate, size() should be reasonably fast
        return size_ != nullptr;
    }
    std::size_t size() const {
        // If we have a delegate, use it, otherwise use distance.
        return size_ != nullptr ? size_()
                                : static_cast<std::size_t>(std::distance(begin(), end()));
    }

    // Returns const version of this enumerable
    auto as_const() const -> enumerable<typename std::add_const<T>::type> {
        return enumerable<typename std::add_const<T>::type>(*this);
    }

public:
    // Iterator for the elements in an enumerable's sequence.
    class iterator
    {
    public:
        // Standard iterator typedefs, plus a few more
        typedef std::forward_iterator_tag               iterator_category;
        typedef typename enumerable<T>::value_type      value_type;
        typedef typename enumerable<T>::raw_value_type  raw_value_type;     // Non-standard
        typedef std::ptrdiff_t                          difference_type;
        typedef typename enumerable<T>::pointer         pointer;
        typedef typename enumerable<T>::reference       reference;

    private:
        const enumerable<T>* pparent_;                  // Parent enumerable or nullptr if unbound
        mutable next_delegate next_;                    // Delegate to use to fetch elements
        mutable std::unique_ptr<raw_value_type> upopt_; // Optional storage passed to the delegate
        mutable pointer pcur_;                          // Pointer to current element or nullptr when at end of sequence
        mutable bool has_cur_;                          // Whether pcur_ has been loaded for current element
        size_t pos_;                                    // Position in sequence, to compare iterators

        // Fetches value of pcur_ when copying/assigning from another iterator.
        static pointer get_copied_pcur(std::unique_ptr<raw_value_type>& our_upopt,
                                       const std::unique_ptr<raw_value_type>& copied_upopt,
                                       pointer copied_pcur)
        {
            pointer pcur = copied_pcur;
            if (copied_upopt != nullptr && our_upopt != nullptr) {
                reference rcopied = *copied_upopt;
                pointer pcopied = std::addressof(rcopied);
                if (copied_pcur == pcopied) {
                    // Other iterator points to its internal object, point to ours
                    reference rours = *our_upopt;
                    pcur = std::addressof(rours);
                }
            }
            return pcur;
        }

        // Fetches value of pcur_, late-loading it if needed
        pointer get_pcur() const {
            if (!has_cur_) {
                pcur_ = next_(upopt_);
                has_cur_ = true;
            }
            return pcur_;
        }
    
    public:
        // Default constructor
        iterator()
            : pparent_(nullptr), next_(), upopt_(), pcur_(nullptr), has_cur_(true), pos_(0) { }

        // Constructor from enumerable
        iterator(const enumerable<T>& parent, bool is_end)
            : pparent_(&parent), next_(!is_end ? parent.zero_ : next_delegate()),
              upopt_(), pcur_(nullptr), has_cur_(is_end), pos_(0) { }

        // Copy/move semantics
        iterator(const iterator& obj)
            : pparent_(obj.pparent_), next_(obj.next_), upopt_(detail::get_copied_upopt(obj.upopt_)),
              pcur_(get_copied_pcur(upopt_, obj.upopt_, obj.pcur_)), has_cur_(obj.has_cur_), pos_(obj.pos_) { }
        iterator(iterator&& obj)
            : pparent_(obj.pparent_), next_(std::move(obj.next_)), upopt_(std::move(obj.upopt_)),
              pcur_(obj.pcur_), has_cur_(obj.has_cur_), pos_(obj.pos_)
        {
            obj.pparent_ = nullptr;
            obj.pcur_ = nullptr;
            obj.has_cur_ = true;
            obj.pos_ = 0;
        }

        iterator& operator=(const iterator& obj) {
            if (this != &obj) {
                pparent_ = obj.pparent_;
                next_ = obj.next_;
                upopt_ = detail::get_copied_upopt(obj.upopt_);
                pcur_ = get_copied_pcur(upopt_, obj.upopt_, obj.pcur_);
                has_cur_ = obj.has_cur_;
                pos_ = obj.pos_;
            }
            return *this;
        }
        iterator& operator=(iterator&& obj) {
            pparent_ = obj.pparent_;
            next_ = std::move(obj.next_);
            upopt_ = std::move(obj.upopt_);
            pcur_ = obj.pcur_;
            has_cur_ = obj.has_cur_;
            pos_ = obj.pos_;

            obj.pparent_ = nullptr;
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
        iterator& operator++() {
            if (has_cur_) {
                pcur_ = nullptr;
                has_cur_ = false;
            } else {
                next_(upopt_);
            }
            ++pos_;
            return *this;
        }
        iterator operator++(int) {
            iterator it(*this);
            ++*this;
            return it;
        }
        
        // Iterator comparison
        friend bool operator==(const iterator& left, const iterator& right) {
            return left.pparent_ == right.pparent_ &&
                   (left.get_pcur() == nullptr) == (right.get_pcur() == nullptr) &&
                   (left.get_pcur() == nullptr || left.pos_ == right.pos_);
        }
        friend bool operator!=(const iterator& left, const iterator& right) {
            return !(left == right);
        }
    };

    // Iterator for the elements in an enumerable's sequence, but returning const references.
    class const_iterator
    {
    public:
        // Standard iterator typedefs, plus a few more
        typedef std::forward_iterator_tag                   iterator_category;
        typedef typename enumerable<T>::const_value_type    value_type;
        typedef typename enumerable<T>::raw_value_type      raw_value_type;     // Non-standard
        typedef std::ptrdiff_t                              difference_type;
        typedef typename enumerable<T>::const_pointer       pointer;
        typedef typename enumerable<T>::const_reference     reference;

    private:
        iterator it_;   // Iterator implementation we're simply wrapping.
    
    public:
        // Default constructor
        const_iterator() = default;

        // Constructor from enumerable
        const_iterator(const enumerable<T>& parent, bool is_end)
            : it_(parent, is_end) { }
        
        // Element access
        reference operator*() const {
            return *it_;
        }
        pointer operator->() const {
            return it_.operator->();
        }
        
        // Move to next element (pre/post versions)
        const_iterator& operator++() {
            ++it_;
            return *this;
        }
        const_iterator operator++(int) {
            const_iterator it(*this);
            ++*this;
            return it;
        }
        
        // Iterator comparison
        friend bool operator==(const const_iterator& left, const const_iterator& right) {
            return left.it_ == right.it_;
        }
        friend bool operator!=(const const_iterator& left, const const_iterator& right) {
            return !(left == right);
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
        auto spobj = std::make_shared<value_type>(std::forward<U>(obj));
        bool available = true;
        return enumerable<T>([spobj, available](std::unique_ptr<raw_value_type>&) mutable {
            pointer pobj = nullptr;
            if (available) {
                pobj = spobj.get();
                available = false;
            }
            return pobj;
        }, []() -> std::size_t {
            return 1;
        });
    }

    // Returns enumerable over sequence of one external element.
    static enumerable<T> for_one_ref(reference obj) {
        bool available = true;
        return enumerable<T>([&obj, available](std::unique_ptr<raw_value_type>&) mutable {
            pointer pobj = nullptr;
            if (available) {
                pobj = std::addressof(obj);
                available = false;
            }
            return pobj;
        }, []() -> std::size_t {
            return 1;
        });
    }

    // Returns enumerable over sequence bound by two iterators.
    template<typename ItBeg, typename ItEnd>
    static enumerable<T> for_range(ItBeg&& ibeg, ItEnd&& iend) {
        auto it = std::forward<ItBeg>(ibeg);
        auto end = std::forward<ItEnd>(iend);
        return enumerable<T>([it, end](std::unique_ptr<raw_value_type>& upopt) mutable {
            pointer pobj = nullptr;
            if (it != end) {
                pobj = detail::get_ref_from_iterator<reference, pointer>(it, upopt);
                ++it;
            }
            return pobj;
        }, detail::get_size_delegate_for_iterators(it, end));
    }

    // Returns enumerable over a container, stored externally.
    template<typename C>
    static enumerable<T> for_container(C& cnt) {
        auto it = std::begin(cnt);
        auto end = std::end(cnt);
        return enumerable<T>([it, end](std::unique_ptr<raw_value_type>& upopt) mutable {
            pointer pobj = nullptr;
            if (it != end) {
                pobj = detail::get_ref_from_iterator<reference, pointer>(it, upopt);
                ++it;
            }
            return pobj;
        }, [&cnt]() -> std::size_t {
            return cnt.size();
        });
    }

    // Returns enumerable over a container, stored internally (by moving it).
    template<typename C,
             typename = typename std::enable_if<!std::is_reference<C>::value, void>::type>
    static enumerable<T> for_container(C&& cnt) {
        auto spcnt = std::make_shared<C>(std::move(cnt));
        auto it = std::begin(*spcnt);
        auto end = std::end(*spcnt);
        return enumerable<T>([spcnt, it, end](std::unique_ptr<raw_value_type>& upopt) mutable {
            pointer pobj = nullptr;
            if (it != end) {
                pobj = detail::get_ref_from_iterator<reference, pointer>(it, upopt);
                ++it;
            }
            return pobj;
        }, [spcnt]() -> std::size_t {
            return spcnt->size();
        });
    }

    // Returns enumerable over dynamic array.
    static enumerable<T> for_array(pointer parr, size_t siz) {
        // This works even if siz is 0 or if parr is nullptr.
        return for_range(parr, parr + siz);
    }
};

// Helper functions corresponding to the helper static methods in enumerable.

// Returns enumerable for sequence of one element, stored internally (moved if possible).
template<typename U>
auto enumerate_one(U&& obj)
    -> enumerable<typename detail::seq_element_traits<U>::const_value_type>
{
    return enumerable<typename detail::seq_element_traits<U>::const_value_type>::for_one(std::forward<U>(obj));
}

// Returns enumerable for sequence of one element, stored externally.
template<typename T>
auto enumerate_one_ref(T& obj) -> enumerable<T> {
    return enumerable<T>::for_one_ref(obj);
}

// Returns enumerable for sequence defined by two iterators.
template<typename ItBeg, typename ItEnd>
auto enumerate_range(ItBeg&& ibeg, ItEnd&& iend)
    -> enumerable<typename detail::seq_element_traits<decltype(*std::declval<ItBeg>())>::value_type>
{
    return enumerable<typename detail::seq_element_traits<decltype(*std::declval<ItBeg>())>::value_type>::for_range(
        std::forward<ItBeg>(ibeg), std::forward<ItEnd>(iend));
}

// Returns enumerable for container, stored externally.
template<typename C>
auto enumerate_container(C& cnt)
    -> enumerable<typename detail::seq_element_traits<decltype(*std::begin(std::declval<C>()))>::value_type>
{
    return enumerable<typename detail::seq_element_traits<decltype(*std::begin(std::declval<C>()))>::value_type>::for_container(
        cnt);
}

// Returns enumerable for container, stored internally (by moving it).
template<typename C,
         typename = typename std::enable_if<!std::is_reference<C>::value, void>::type>
auto enumerate_container(C&& cnt)
    -> enumerable<typename detail::seq_element_traits<decltype(*std::begin(std::declval<C>()))>::const_value_type>
{
    return enumerable<typename detail::seq_element_traits<decltype(*std::begin(std::declval<C>()))>::const_value_type>::for_container(
        std::move(cnt));
}

// Returns enumerable for dynamic array.
template<typename T>
auto enumerate_array(T* parr, size_t siz)
    -> enumerable<T>
{
    return enumerable<T>::for_array(parr, siz);
}

} // coveo

#endif // COVEO_ENUMERABLE_H
