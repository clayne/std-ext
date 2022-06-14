#pragma once

#include "allocator.h"
#include "rbtree.h"

namespace uxs {

namespace detail {

//-----------------------------------------------------------------------------
// Node handle implementation

template<typename NodeTraits, typename Alloc, typename NodeHandle, typename = void>
class rbtree_node_handle_getters
    : protected std::allocator_traits<Alloc>::template rebind_alloc<typename NodeTraits::node_t> {
 protected:
    using node_traits = NodeTraits;
    using alloc_type = typename std::allocator_traits<Alloc>::template rebind_alloc<typename node_traits::node_t>;

 public:
    using value_type = typename node_traits::value_type;
    rbtree_node_handle_getters() NOEXCEPT_IF(std::is_nothrow_default_constructible<alloc_type>::value)
        : alloc_type(Alloc()) {}
    explicit rbtree_node_handle_getters(const alloc_type& alloc) NOEXCEPT : alloc_type(alloc) {}
    ~rbtree_node_handle_getters() = default;
    rbtree_node_handle_getters(const rbtree_node_handle_getters&) = delete;
    rbtree_node_handle_getters& operator=(const rbtree_node_handle_getters&) = delete;
    rbtree_node_handle_getters(rbtree_node_handle_getters&& other) NOEXCEPT : alloc_type(std::move(other)) {}
    rbtree_node_handle_getters& operator=(rbtree_node_handle_getters&& other) NOEXCEPT {
        alloc_type::operator=(std::move(other));
        return *this;
    }
    value_type& value() const { return node_traits::get_value(static_cast<const NodeHandle*>(this)->node_); }
};

template<typename NodeTraits, typename Alloc, typename NodeHandle>
class rbtree_node_handle_getters<NodeTraits, Alloc, NodeHandle, std::void_t<typename NodeTraits::mapped_type>>
    : protected std::allocator_traits<Alloc>::template rebind_alloc<typename NodeTraits::node_t> {
 protected:
    using node_traits = NodeTraits;
    using alloc_type = typename std::allocator_traits<Alloc>::template rebind_alloc<typename node_traits::node_t>;

 public:
    using key_type = typename node_traits::key_type;
    using mapped_type = typename node_traits::mapped_type;
    rbtree_node_handle_getters() NOEXCEPT_IF(std::is_nothrow_default_constructible<alloc_type>::value)
        : alloc_type(Alloc()) {}
    explicit rbtree_node_handle_getters(const alloc_type& alloc) NOEXCEPT : alloc_type(alloc) {}
    ~rbtree_node_handle_getters() = default;
    rbtree_node_handle_getters(const rbtree_node_handle_getters&) = delete;
    rbtree_node_handle_getters& operator=(const rbtree_node_handle_getters&) = delete;
    rbtree_node_handle_getters(rbtree_node_handle_getters&& other) NOEXCEPT : alloc_type(std::move(other)) {}
    rbtree_node_handle_getters& operator=(rbtree_node_handle_getters&& other) NOEXCEPT {
        alloc_type::operator=(std::move(other));
        return *this;
    }
    key_type& key() const {
        return const_cast<key_type&>(node_traits::get_value(static_cast<const NodeHandle*>(this)->node_).first);
    }
    mapped_type& mapped() const { return node_traits::get_value(static_cast<const NodeHandle*>(this)->node_).second; }
};

template<typename NodeTraits, typename Alloc>
class rbtree_node_handle : public rbtree_node_handle_getters<NodeTraits, Alloc, rbtree_node_handle<NodeTraits, Alloc>> {
 private:
    using super = rbtree_node_handle_getters<NodeTraits, Alloc, rbtree_node_handle>;
    using node_traits = NodeTraits;
    using alloc_type = typename super::alloc_type;
    using alloc_traits = std::allocator_traits<alloc_type>;

 public:
    using allocator_type = Alloc;

    rbtree_node_handle() NOEXCEPT_IF(noexcept(super())) : super() {}
    rbtree_node_handle(rbtree_node_handle&& nh) NOEXCEPT : super(std::move(nh)) {
        node_ = nh.node_;
        nh.node_ = nullptr;
    }

    rbtree_node_handle& operator=(rbtree_node_handle&& nh) NOEXCEPT {
        assert(std::addressof(nh) != this);
        if (std::addressof(nh) == this) { return *this; }
        if (node_) { tidy(); }
        super::operator=(std::move(nh));
        node_ = nh.node_;
        nh.node_ = nullptr;
        return *this;
    }

    ~rbtree_node_handle() {
        if (node_) { tidy(); }
    }

    allocator_type get_allocator() const { return allocator_type(*this); }
    bool empty() const NOEXCEPT { return node_ == nullptr; }
    explicit operator bool() const NOEXCEPT { return node_ != nullptr; }
    void swap(rbtree_node_handle& nh) NOEXCEPT {
        if (std::addressof(nh) == this) { return; }
        std::swap(static_cast<alloc_type&>(*this), static_cast<alloc_type&>(nh));
        std::swap(node_, nh.node_);
    }

 protected:
    rbtree_node_t* node_ = nullptr;

    template<typename, typename, typename, typename>
    friend class rbtree_node_handle_getters;
    template<typename, typename, typename>
    friend class rbtree_base;
    template<typename, typename, typename>
    friend class rbtree_unique;
    template<typename, typename, typename>
    friend class rbtree_multi;

    explicit rbtree_node_handle(const alloc_type& alloc) NOEXCEPT : super(alloc) {}
    rbtree_node_handle(const alloc_type& alloc, rbtree_node_t* node) NOEXCEPT : super(alloc), node_(node) {}

    void tidy() {
        alloc_traits::destroy(*this, std::addressof(node_traits::get_value(node_)));
        alloc_traits::deallocate(*this, static_cast<typename node_traits::node_t*>(node_), 1);
    }
};

}  // namespace detail

}  // namespace uxs

namespace std {
template<typename NodeTraits, typename Alloc>
void swap(uxs::detail::rbtree_node_handle<NodeTraits, Alloc>& nh1,
          uxs::detail::rbtree_node_handle<NodeTraits, Alloc>& nh2) NOEXCEPT_IF(NOEXCEPT_IF(nh1.swap(nh2))) {
    nh1.swap(nh2);
}
}  // namespace std