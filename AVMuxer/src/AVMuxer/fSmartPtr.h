#pragma once

#include "macros.h"

#include <stdexcept>
#include <type_traits>

template <typename T, std::size_t Type = 0>
struct fAllocator
{
    typedef typename std::remove_const<T>::type value_type;
    typedef value_type *                        point_type;

    void _free(point_type ptr) {
        // Do nothing!
        UNUSED_VAR(ptr);
    }
};

template <typename T, std::size_t Type = 0>
class fRefCount
{
public:
    typedef typename std::remove_const<T>::type value_type;
    typedef value_type *                        point_type;
    typedef value_type &                        reference_type;
    typedef fAllocator<T, Type>                 allocator_t;
    typedef fRefCount<T, Type>                  this_type;

    typedef intptr_t counter_t;

    fRefCount(point_type ptr = nullptr) : ptr_(ptr), count_(0) {
    }

    ~fRefCount() {
        release();
    }

    point_type ptr() {
        return ptr_;
    }

    const point_type ptr() const {
        return ptr_;
    }

    counter_t count() const {
        return count_;
    }

    counter_t add_ref() {
        counter_t old = count_;
        ++count_;
        return old;
    }

    void release() {
        --count_;
        if (count_ == 0) {
            _free(ptr_);
            ptr_ = nullptr;
        }
    }

    void _free(point_type ptr) {
        allocator_._free(ptr);
    }

    point_type  ptr_;
    counter_t   count_;
    allocator_t allocator_;
};

template <typename T>
struct is_dual_pointer
{
    static constexpr bool value = false;
};

template <typename T>
struct is_dual_pointer<T *>
{
    static constexpr bool value = true;
};

template <typename T, std::size_t Type = 0>
class fSmartPtr
{
public:
    typedef fRefCount<T, Type>                  manager_t;
    typedef typename manager_t::value_type      value_type;
    typedef typename manager_t::point_type      point_type;
    typedef typename manager_t::reference_type  reference_type;
    typedef typename manager_t::counter_t       counter_t;
    typedef fSmartPtr<T, Type>                  this_type;

    static constexpr bool kIsDualPointer = is_dual_pointer<T>::value;

    fSmartPtr(point_type ptr = nullptr) : mgr_(nullptr) {
        mgr_ = new manager_t(ptr);
        mgr_->add_ref();
    }
    fSmartPtr(const this_type & src) : mgr_(src.mgr()) noexcept {
        if (this != (this_type *)(&src)) {
            if (mgr_ != nullptr) {
                mgr_->add_ref();
            }
        }
    }
    fSmartPtr(this_type && src) : mgr_(src.mgr()) noexcept {
        if (this != (this_type *)(&std::forward<this_type>(src))) {
            src.mgr_ = nullptr;
        }
    }

    ~fSmartPtr() {
        release();
    }

    bool is_valid() const {
        return (mgr_ != nullptr);
    }

    const manager_t * mgr() const noexcept {
        return mgr_;
    }

    manager_t * mgr() noexcept {
        return mgr_;
    }

    point_type ptr() noexcept {
        if (mgr_ != nullptr)
            return mgr_->ptr();
        else
            return nullptr;
    }

    const point_type ptr() const noexcept {
        if (mgr_ != nullptr)
            return mgr_->ptr();
        else
            return nullptr;
    }

    counter_t count() noexcept {
        if (mgr_ != nullptr)
            return mgr_->count();
        else
            return -1;
    }

    counter_t add_ref() {
        if (mgr_ != nullptr) {
            mgr_->add_ref();
        }
        else {
            throw std::runtime_error("Can not add_def() on a empty object!");
        }
    }

    void release(bool force_delete = false) {
        if (mgr_ != nullptr) {
            mgr_->release();
            if (mgr_->count() == 0 || force_delete) {
                delete mgr_;
                mgr_ = nullptr;
            }
        }
    }

    this_type & operator = (const this_type & rhs) {
        if (this != (this_type *)(&rhs)) {
            mgr_ = rhs.mgr();
            if (mgr_ != nullptr) {
                add_ref();
            }
        }
        return *this;
    }

    this_type & operator = (this_type && rhs) noexcept {
        if (this != (this_type *)(&std::forward<this_type>(rhs))) {
            mgr_ = rhs.mgr();
            rhs.mgr_ = nullptr;
        }
        return *this;
    }

    this_type & operator = (point_type value) {
        if (mgr_ == nullptr) {
            mgr_ = new manager_t(value);
            mgr_->add_ref();
        }
        else {
            if (ptr() != value) {
                // Release and destory
                release(true);

                // Create new always
                mgr_ = new manager_t(value);
                mgr_->add_ref();
            }
        }
        return *this;
    }

    point_type * operator & () noexcept {
        if (mgr_ != nullptr) {
            return &(mgr_->ptr_);
        }
        else {
            return nullptr;
        }
    }

    const point_type * operator & () const noexcept {
        if (mgr_ != nullptr) {
            return &(mgr_->ptr_);
        }
        else {
            return nullptr;
        }
    }

    reference_type operator * () {
        if (mgr_ != nullptr) {
            return *(mgr_->ptr_);
        }
        else {
            throw std::runtime_error("Can not use the * operator on null pointers!");
        }
    }

    const reference_type operator * () const {
        if (mgr_ != nullptr) {
            return *(mgr_->ptr_);
        }
        else {
            throw std::runtime_error("Can not use the * operator on null pointers!");
        }
    }

    point_type operator -> () noexcept {
        return ptr();
    }

    const point_type operator -> () const noexcept {
        return ptr();
    }

    operator point_type () noexcept {
        return ptr();
    }

    operator const point_type () const noexcept {
        return ptr();
    }

    void swap(this_type & other) noexcept {
        std::swap(this->mgr_, other.mgr_);
    }

    friend inline void swap(this_type & lhs, this_type & rhs) noexcept {
        lhs.swap(rhs);
    }

    friend inline bool operator == (const this_type & lhs, const this_type & rhs) noexcept {
        return ((lhs.mgr() == rhs.mgr()) && (lhs.ptr() == rhs.ptr()));
    }

    friend inline bool operator != (const this_type & lhs, const this_type & rhs) noexcept {
        return !(lhs == rhs);
    }

    friend inline bool operator == (const this_type & lhs, int val) noexcept {
        return ((intptr_t)lhs.ptr() == (intptr_t)val);
    }

    friend inline bool operator != (const this_type & lhs, int val) noexcept {
        return ((intptr_t)lhs.ptr() != (intptr_t)val);
    }

    friend inline bool operator == (int val, const this_type & rhs) noexcept {
        return ((intptr_t)rhs.ptr() == (intptr_t)val);
    }

    friend inline bool operator != (int val, const this_type & rhs) noexcept {
        return ((intptr_t)rhs.ptr() != (intptr_t)val);
    }

    friend inline bool operator == (const this_type & lhs, nullptr_t) noexcept {
        return (lhs.ptr() == nullptr);
    }

    friend inline bool operator != (const this_type & lhs, nullptr_t) noexcept {
        return (lhs.ptr() != nullptr);
    }

    friend inline bool operator == (nullptr_t, const this_type & rhs) noexcept {
        return (rhs.ptr() == nullptr);
    }

    friend inline bool operator != (nullptr_t, const this_type & rhs) noexcept {
        return (rhs.ptr() != nullptr);
    }

    friend inline bool operator == (const this_type & lhs, void * ptr) noexcept {
        return ((void *)lhs.ptr() == ptr);
    }

    friend inline bool operator != (const this_type & lhs, void * ptr) noexcept {
        return ((void *)lhs.ptr() != ptr);
    }

    friend inline bool operator == (void * ptr, const this_type & rhs) noexcept {
        return ((void *)rhs.ptr() == ptr);
    }

    friend inline bool operator != (void * ptr, const this_type & rhs) noexcept {
        return ((void *)rhs.ptr() != ptr);
    }

protected:
    manager_t * mgr_;
};
