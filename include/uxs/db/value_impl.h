#pragma once

#include "value.h"

#include "uxs/algorithm.h"
#include "uxs/stringcvt.h"

#include <cmath>

namespace uxs {
namespace db {

// --------------------------

template<typename CharT, typename Alloc>
UXS_EXPORT bool operator==(const basic_value<CharT, Alloc>& lhs, const basic_value<CharT, Alloc>& rhs) noexcept {
    if (lhs.type_ != rhs.type_) { return false; }
    switch (lhs.type_) {
        case dtype::null: return true;
        case dtype::boolean: return lhs.value_.b == rhs.value_.b;
        case dtype::integer: return lhs.value_.i == rhs.value_.i;
        case dtype::unsigned_integer: return lhs.value_.u == rhs.value_.u;
        case dtype::long_integer: return lhs.value_.i64 == rhs.value_.i64;
        case dtype::unsigned_long_integer: return lhs.value_.u64 == rhs.value_.u64;
        case dtype::double_precision: return lhs.value_.dbl == rhs.value_.dbl;
        case dtype::string: return lhs.str_view() == rhs.str_view();
        case dtype::array: {
            auto range1 = lhs.as_array();
            auto range2 = rhs.as_array();
            return range1.size() == range2.size() && equal(range1, range2.begin());
        } break;
        case dtype::record: {
            if (lhs.value_.rec->size != rhs.value_.rec->size) { return false; }
            return equal(lhs.as_record(), rhs.as_record().begin());
        } break;
        default: break;
    }
    return false;
}

// --------------------------

namespace detail {

template<typename Alloc, typename Ty,
         typename = std::enable_if_t<
             std::is_trivially_move_constructible<Ty>::value ||
             std::is_same<typename std::allocator_traits<Alloc>::template rebind_alloc<Ty>, std::allocator<Ty>>::value>>
static void move_values(const Ty* first, const Ty* last, Ty* dest) {
    std::memcpy(static_cast<void*>(dest), static_cast<const void*>(first), (last - first) * sizeof(Ty));
}

template<typename Alloc, typename Ty, typename... Dummy>
static void move_values(const Ty* first, const Ty* last, Ty* dest, Dummy&&...) {
    for (; first != last; ++first, ++dest) { new (dest) Ty(std::move(*first)); }
}

template<typename Alloc, typename Ty,
         typename = std::enable_if_t<
             std::is_trivially_destructible<Ty>::value ||
             std::is_same<typename std::allocator_traits<Alloc>::template rebind_alloc<Ty>, std::allocator<Ty>>::value>>
static void destruct_moved_values(Ty* first, Ty* last) {}

template<typename Alloc, typename Ty, typename... Dummy>
static void destruct_moved_values(Ty* first, Ty* last, Dummy&&...) {
    for (; first != last; ++first) { first->~Ty(); };
}

template<typename CharT, typename Alloc, bool store_values>
/*static*/ auto flexarray_t<CharT, Alloc, store_values>::alloc(alloc_type& arr_al, std::size_t cap) -> flexarray_t* {
    const std::size_t alloc_sz = get_alloc_sz(cap);
    flexarray_t* arr = arr_al.allocate(alloc_sz);
    arr->capacity = (alloc_sz * sizeof(flexarray_t) - offsetof(flexarray_t, x)) / sizeof(array_value_t);
    assert(arr->capacity >= cap && get_alloc_sz(arr->capacity) == alloc_sz);
    return arr;
}

template<typename CharT, typename Alloc, bool store_values>
/*static*/ auto flexarray_t<CharT, Alloc, store_values>::grow(alloc_type& arr_al, flexarray_t* arr,
                                                              std::size_t extra) -> flexarray_t* {
    std::size_t delta_sz = std::max(extra, arr->size >> 1);
    const std::size_t max_sz = max_size(arr_al);
    if (delta_sz > max_sz - arr->size) {
        if (extra > max_sz - arr->size) { throw std::length_error("too much to reserve"); }
        delta_sz = std::max(extra, (max_sz - arr->size) >> 1);
    }
    flexarray_t* new_arr = alloc(arr_al, std::max<std::size_t>(arr->size + delta_sz, start_capacity));
    detail::move_values<alloc_type>(&(*arr)[0], &(*arr)[arr->size], &(*new_arr)[0]);
    new_arr->size = arr->size;
    detail::destruct_moved_values<alloc_type>(&(*arr)[0], &(*arr)[arr->size]);
    dealloc(arr_al, arr);
    return new_arr;
}

// --------------------------

template<typename CharT, typename Alloc>
void record_t<CharT, Alloc>::init() {
    dllist_make_cycle(&head);
    node_traits::set_head(&head, &head);
    for (auto& item : est::as_span(hashtbl, bucket_count)) { item = nullptr; }
    size = 0;
}

template<typename CharT, typename Alloc>
void record_t<CharT, Alloc>::destroy(alloc_type& rec_al, list_links_t* node) {
    while (node != &head) {
        list_links_t* next = node->next;
        delete_node(rec_al, node);
        node = next;
    }
}

template<typename CharT, typename Alloc>
list_links_t* record_t<CharT, Alloc>::find(std::basic_string_view<CharT> key, std::size_t hash_code) const {
    list_links_t* next_bucket = hashtbl[hash_code % bucket_count];
    while (next_bucket) {
        if (node_t::from_links(next_bucket)->hash_code_ == hash_code &&
            node_traits::get_value(next_bucket).key() == key) {
            return next_bucket;
        }
        next_bucket = node_t::from_links(next_bucket)->next_bucket_;
    }
    return &head;
}

template<typename CharT, typename Alloc>
std::size_t record_t<CharT, Alloc>::count(std::basic_string_view<CharT> key) const {
    std::size_t count = 0;
    const std::size_t hash_code = hasher{}(key);
    list_links_t* next_bucket = hashtbl[hash_code % bucket_count];
    while (next_bucket) {
        if (node_t::from_links(next_bucket)->hash_code_ == hash_code &&
            node_traits::get_value(next_bucket).key() == key) {
            ++count;
        }
        next_bucket = node_t::from_links(next_bucket)->next_bucket_;
    }
    return count;
}

template<typename CharT, typename Alloc>
/*static*/ record_t<CharT, Alloc>* record_t<CharT, Alloc>::create(
    alloc_type& rec_al, const std::initializer_list<basic_value<CharT, Alloc>>& init) {
    record_t* rec = alloc(rec_al, init.size() ? std::min(init.size(), max_size(rec_al)) : 1);
    rec->init();
    try {
        for_each(init, [&rec_al, &rec](const basic_value<CharT, Alloc>& v) {
            const std::basic_string_view<CharT> key = (*v.value_.arr)[0].str_view();
            list_links_t* node = rec->new_node(rec_al, key, (*v.value_.arr)[1]);
            rec = insert(rec_al, rec, hasher{}(key), node);
        });
        return rec;
    } catch (...) {
        rec->destroy(rec_al, rec->head.next);
        dealloc(rec_al, rec);
        throw;
    }
}

template<typename CharT, typename Alloc>
/*static*/ record_t<CharT, Alloc>* record_t<CharT, Alloc>::create(
    alloc_type& rec_al,
    const std::initializer_list<std::pair<std::basic_string_view<CharT>, basic_value<CharT, Alloc>>>& init) {
    record_t* rec = alloc(rec_al, init.size() ? std::min(init.size(), max_size(rec_al)) : 1);
    rec->init();
    try {
        for_each(init, [&rec_al, &rec](const std::pair<std::basic_string_view<CharT>, basic_value<CharT, Alloc>>& v) {
            list_links_t* node = rec->new_node(rec_al, v.first, v.second);
            rec = insert(rec_al, rec, hasher{}(v.first), node);
        });
        return rec;
    } catch (...) {
        rec->destroy(rec_al, rec->head.next);
        dealloc(rec_al, rec);
        throw;
    }
}

template<typename CharT, typename Alloc>
/*static*/ record_t<CharT, Alloc>* record_t<CharT, Alloc>::create(alloc_type& rec_al, const record_t& src) {
    record_t* rec = alloc(rec_al, src.size ? std::min(src.size, max_size(rec_al)) : 0);
    rec->init();
    try {
        list_links_t* node = src.head.next;
        while (node != &src.head) {
            const auto& v = node_traits::get_value(node);
            rec = insert(rec_al, rec, node_t::from_links(node)->hash_code_, rec->new_node(rec_al, v.key(), v.value()));
            node = node->next;
        }
        return rec;
    } catch (...) {
        rec->destroy(rec_al, rec->head.next);
        dealloc(rec_al, rec);
        throw;
    }
}

template<typename CharT, typename Alloc>
/*static*/ record_t<CharT, Alloc>* record_t<CharT, Alloc>::assign(alloc_type& rec_al, record_t* rec,
                                                                  const record_t& src) {
    rec->clear(rec_al);
    list_links_t* node = src.head.next;
    while (node != &src.head) {
        const auto& v = node_traits::get_value(node);
        rec = insert(rec_al, rec, node_t::from_links(node)->hash_code_, rec->new_node(rec_al, v.key(), v.value()));
        node = node->next;
    }
    return rec;
}

template<typename CharT, typename Alloc>
void record_t<CharT, Alloc>::add_to_hash(list_links_t* node, std::size_t hash_code) {
    list_links_t** slot = &hashtbl[hash_code % bucket_count];
    node_t::from_links(node)->next_bucket_ = *slot;
    *slot = node;
}

template<typename CharT, typename Alloc>
/*static*/ record_t<CharT, Alloc>* record_t<CharT, Alloc>::insert(alloc_type& rec_al, record_t* rec,
                                                                  std::size_t hash_code, list_links_t* node) {
    assert(rec->size <= rec->bucket_count);
    if (rec->size == rec->bucket_count) {
        const std::size_t sz = next_bucket_count(rec_al, rec->size);
        if (sz > rec->size) { rec = record_t::rehash(rec_al, rec, sz); }
    }
    node_traits::set_head(node, &rec->head);
    node_t::from_links(node)->hash_code_ = hash_code;
    rec->add_to_hash(node, hash_code);
    dllist_insert_before(&rec->head, node);
    ++rec->size;
    return rec;
}

template<typename CharT, typename Alloc>
list_links_t* record_t<CharT, Alloc>::erase(alloc_type& rec_al, list_links_t* node) {
    list_links_t** p_next_bucket = &hashtbl[node_t::from_links(node)->hash_code_ % bucket_count];
    while (*p_next_bucket != node) {
        assert(*p_next_bucket);
        p_next_bucket = &node_t::from_links(*p_next_bucket)->next_bucket_;
    }
    *p_next_bucket = node_t::from_links(*p_next_bucket)->next_bucket_;
    list_links_t* next = dllist_remove(node);
    delete_node(rec_al, node);
    --size;
    return next;
}

template<typename CharT, typename Alloc>
std::size_t record_t<CharT, Alloc>::erase(alloc_type& rec_al, std::basic_string_view<CharT> key) {
    const std::size_t old_sz = size;
    const std::size_t hash_code = hasher{}(key);
    list_links_t** p_next_bucket = &hashtbl[hash_code % bucket_count];
    while (*p_next_bucket) {
        if (node_t::from_links(*p_next_bucket)->hash_code_ == hash_code &&
            node_traits::get_value(*p_next_bucket).key() == key) {
            list_links_t* erased_node = *p_next_bucket;
            *p_next_bucket = node_t::from_links(*p_next_bucket)->next_bucket_;
            --size;
            dllist_remove(erased_node);
            delete_node(rec_al, erased_node);
        } else {
            p_next_bucket = &node_t::from_links(*p_next_bucket)->next_bucket_;
        }
    }
    return old_sz - size;
}

template<typename CharT, typename Alloc>
/*static*/ std::size_t record_t<CharT, Alloc>::next_bucket_count(const alloc_type& rec_al, std::size_t sz) {
    std::size_t delta_sz = std::max<std::size_t>(min_bucket_count_inc, sz >> 1);
    const std::size_t max_sz = (std::allocator_traits<alloc_type>::max_size(rec_al) * sizeof(record_t) -
                                offsetof(record_t, hashtbl)) /
                               sizeof(list_links_t*);
    if (delta_sz > max_sz - sz) {
        if (sz == max_sz) { return sz; }
        delta_sz = std::max<std::size_t>(1, (max_sz - sz) >> 1);
    }
    return sz + delta_sz;
}

template<typename CharT, typename Alloc>
/*static*/ record_t<CharT, Alloc>* record_t<CharT, Alloc>::alloc(alloc_type& rec_al, std::size_t bckt_cnt) {
    const std::size_t alloc_sz = get_alloc_sz(bckt_cnt);
    record_t* rec = rec_al.allocate(alloc_sz);
    rec->bucket_count = (alloc_sz * sizeof(record_t) - offsetof(record_t, hashtbl)) / sizeof(list_links_t*);
    assert(rec->bucket_count >= bckt_cnt && get_alloc_sz(rec->bucket_count) == alloc_sz);
    return rec;
}

template<typename CharT, typename Alloc>
/*static*/ record_t<CharT, Alloc>* record_t<CharT, Alloc>::rehash(alloc_type& rec_al, record_t* rec,
                                                                  std::size_t bckt_cnt) {
    assert(rec->size);
    record_t* new_rec = alloc(rec_al, bckt_cnt);
    new_rec->head = rec->head;
    new_rec->size = rec->size;
    dealloc(rec_al, rec);
    node_traits::set_head(&new_rec->head, &new_rec->head);
    for (auto& item : est::as_span(new_rec->hashtbl, new_rec->bucket_count)) { item = nullptr; }
    list_links_t* node = new_rec->head.next;
    node->prev = &new_rec->head;
    new_rec->head.prev->next = &new_rec->head;
    while (node != &new_rec->head) {
        node_traits::set_head(node, &new_rec->head);
        new_rec->add_to_hash(node, node_t::from_links(node)->hash_code_);
        node = node->next;
    }
    return new_rec;
}

template<typename CharT, typename Alloc>
/*static*/ record_value<CharT, Alloc>* record_value<CharT, Alloc>::alloc_checked(alloc_type& node_al,
                                                                                 std::basic_string_view<CharT> key) {
    if (key.size() > max_name_size(node_al)) { throw std::length_error("too much to reserve"); }
    const std::size_t alloc_sz = get_alloc_sz(key.size());
    record_value* node = node_al.allocate(alloc_sz);
    node->key_sz_ = key.size();
    std::copy_n(key.data(), key.size(), node->key_chars_);
    return node;
}

// --------------------------

template<typename CharT, typename Alloc>
bool is_record(const std::initializer_list<basic_value<CharT, Alloc>>& init) {
    return all_of(init,
                  [](const basic_value<CharT, Alloc>& v) { return v.is_array() && v.size() == 2 && v[0].is_string(); });
}

}  // namespace detail

template<typename CharT, typename Alloc>
basic_value<CharT, Alloc>::basic_value(std::initializer_list<basic_value> init, const Alloc& al)
    : alloc_type(al), type_(dtype::array) {
    if (detail::is_record(init)) {
        typename record_t::alloc_type rec_al(*this);
        value_.rec = record_t::create(rec_al, init);
        type_ = dtype::record;
    } else {
        value_.arr = alloc_array(init.size(), init.begin());
    }
}

template<typename CharT, typename Alloc>
void basic_value<CharT, Alloc>::assign(std::initializer_list<basic_value> init) {
    if (detail::is_record(init)) {
        typename record_t::alloc_type rec_al(*this);
        if (type_ != dtype::record) {
            if (type_ != dtype::null) { destroy(); }
            value_.rec = record_t::create(rec_al, init);
            type_ = dtype::record;
        } else {
            value_.rec->clear(rec_al);
            for_each(init, [this, &rec_al](const basic_value& v) {
                const std::basic_string_view<char_type> key = (*v.value_.arr)[0].str_view();
                detail::list_links_t* node = value_.rec->new_node(rec_al, key, (*v.value_.arr)[1]);
                value_.rec = record_t::insert(rec_al, value_.rec, typename record_t::hasher{}(key), node);
            });
        }
    } else if (type_ != dtype::array) {
        if (type_ != dtype::null) { destroy(); }
        value_.arr = alloc_array(init.size(), init.begin());
        type_ = dtype::array;
    } else {
        assign_array(init.size(), init.begin());
    }
}

template<typename CharT, typename Alloc>
void basic_value<CharT, Alloc>::insert(std::size_t pos, std::initializer_list<basic_value> init) {
    if (type_ != dtype::array) {
        if (type_ != dtype::null) { throw database_error("not an array"); }
        value_.arr = alloc_array(init.size(), init.begin());
        type_ = dtype::array;
    } else if (init.size()) {
        append_array(init.size(), init.begin());
        if (pos < value_.arr->size - init.size()) {
            std::rotate(&(*value_.arr)[pos], &(*value_.arr)[value_.arr->size - init.size()],
                        &(*value_.arr)[value_.arr->size]);
        }
    }
}

template<typename CharT, typename Alloc>
void basic_value<CharT, Alloc>::insert(
    std::initializer_list<std::pair<std::basic_string_view<char_type>, basic_value>> init) {
    typename record_t::alloc_type rec_al(*this);
    if (type_ != dtype::record) {
        if (type_ != dtype::null) { throw database_error("not a record"); }
        value_.rec = record_t::create(rec_al, init);
        type_ = dtype::record;
    } else {
        for_each(init, [this, &rec_al](const std::pair<std::basic_string_view<char_type>, basic_value>& v) {
            detail::list_links_t* node = value_.rec->new_node(rec_al, v.first, v.second);
            value_.rec = record_t::insert(rec_al, value_.rec, typename record_t::hasher{}(v.first), node);
        });
    }
}

// --------------------------

namespace detail {
inline bool is_integral(double d) {
    double integral_part = 0.;
    return std::modf(d, &integral_part) == 0.;
}
}  // namespace detail

template<typename CharT, typename Alloc>
est::optional<bool> basic_value<CharT, Alloc>::get_bool() const {
    switch (type_) {
        case dtype::null: return est::nullopt();
        case dtype::boolean: return value_.b;
        case dtype::integer: return value_.i != 0;
        case dtype::unsigned_integer: return value_.u != 0;
        case dtype::long_integer: return value_.i64 != 0;
        case dtype::unsigned_long_integer: return value_.u64 != 0;
        case dtype::double_precision: return value_.dbl != 0;
        case dtype::string: {
            est::optional<bool> result(est::in_place());
            return basic_stoval(str_view(), *result) ? result : est::nullopt();
        } break;
        case dtype::array: return est::nullopt();
        case dtype::record: return est::nullopt();
        default: UXS_UNREACHABLE_CODE;
    }
}

template<typename CharT, typename Alloc>
est::optional<std::int32_t> basic_value<CharT, Alloc>::get_int() const {
    switch (type_) {
        case dtype::null: return est::nullopt();
        case dtype::boolean: return value_.b ? 1 : 0;
        case dtype::integer: return value_.i;
        case dtype::unsigned_integer:
            return value_.u <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) ?
                       est::make_optional(static_cast<std::int32_t>(value_.u)) :
                       est::nullopt();
        case dtype::long_integer:
            return value_.i64 >= std::numeric_limits<std::int32_t>::min() &&
                           value_.i64 <= std::numeric_limits<std::int32_t>::max() ?
                       est::make_optional(static_cast<std::int32_t>(value_.i64)) :
                       est::nullopt();
        case dtype::unsigned_long_integer:
            return value_.u64 <= static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) ?
                       est::make_optional(static_cast<std::int32_t>(value_.u64)) :
                       est::nullopt();
        case dtype::double_precision:
            return value_.dbl >= std::numeric_limits<std::int32_t>::min() &&
                           value_.dbl <= std::numeric_limits<std::int32_t>::max() ?
                       est::make_optional(static_cast<std::int32_t>(value_.dbl)) :
                       est::nullopt();
        case dtype::string: {
            est::optional<std::int32_t> result(est::in_place());
            return basic_stoval(str_view(), *result) ? result : est::nullopt();
        } break;
        case dtype::array: return est::nullopt();
        case dtype::record: return est::nullopt();
        default: UXS_UNREACHABLE_CODE;
    }
}

template<typename CharT, typename Alloc>
est::optional<std::uint32_t> basic_value<CharT, Alloc>::get_uint() const {
    switch (type_) {
        case dtype::null: return est::nullopt();
        case dtype::boolean: return value_.b ? 1 : 0;
        case dtype::integer:
            return value_.i >= 0 ? est::make_optional(static_cast<std::uint32_t>(value_.i)) : est::nullopt();
        case dtype::unsigned_integer: return value_.u;
        case dtype::long_integer:
            return value_.i64 >= 0 &&
                           value_.i64 <= static_cast<std::int64_t>(std::numeric_limits<std::uint32_t>::max()) ?
                       est::make_optional(static_cast<std::uint32_t>(value_.i64)) :
                       est::nullopt();
        case dtype::unsigned_long_integer:
            return value_.u64 <= std::numeric_limits<std::uint32_t>::max() ?
                       est::make_optional(static_cast<std::uint32_t>(value_.u64)) :
                       est::nullopt();
        case dtype::double_precision:
            return value_.dbl >= 0 && value_.dbl <= std::numeric_limits<std::uint32_t>::max() ?
                       est::make_optional(static_cast<std::uint32_t>(value_.dbl)) :
                       est::nullopt();
        case dtype::string: {
            est::optional<std::uint32_t> result(est::in_place());
            return basic_stoval(str_view(), *result) ? result : est::nullopt();
        } break;
        case dtype::array: return est::nullopt();
        case dtype::record: return est::nullopt();
        default: UXS_UNREACHABLE_CODE;
    }
}

template<typename CharT, typename Alloc>
est::optional<std::int64_t> basic_value<CharT, Alloc>::get_int64() const {
    switch (type_) {
        case dtype::null: return est::nullopt();
        case dtype::boolean: return value_.b ? 1 : 0;
        case dtype::integer: return value_.i;
        case dtype::unsigned_integer: return static_cast<std::int64_t>(value_.u);
        case dtype::long_integer: return value_.i64;
        case dtype::unsigned_long_integer:
            return value_.u64 <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ?
                       est::make_optional(static_cast<std::int64_t>(value_.u64)) :
                       est::nullopt();
        case dtype::double_precision:
            // Note that double(2^63 - 1) will be rounded up to 2^63, so maximum is excluded
            return value_.dbl >= static_cast<double>(std::numeric_limits<std::int64_t>::min()) &&
                           value_.dbl < static_cast<double>(std::numeric_limits<std::int64_t>::max()) ?
                       est::make_optional(static_cast<std::int64_t>(value_.dbl)) :
                       est::nullopt();
        case dtype::string: {
            est::optional<std::int64_t> result(est::in_place());
            return basic_stoval(str_view(), *result) ? result : est::nullopt();
        } break;
        case dtype::array: return est::nullopt();
        case dtype::record: return est::nullopt();
        default: UXS_UNREACHABLE_CODE;
    }
}

template<typename CharT, typename Alloc>
est::optional<std::uint64_t> basic_value<CharT, Alloc>::get_uint64() const {
    switch (type_) {
        case dtype::null: return est::nullopt();
        case dtype::boolean: return value_.b ? 1 : 0;
        case dtype::integer:
            return value_.i >= 0 ? est::make_optional(static_cast<std::uint64_t>(value_.i)) : est::nullopt();
        case dtype::unsigned_integer: return value_.u;
        case dtype::long_integer:
            return value_.i64 >= 0 ? est::make_optional(static_cast<std::uint64_t>(value_.i64)) : est::nullopt();
        case dtype::unsigned_long_integer: return value_.u64;
        case dtype::double_precision:
            // Note that double(2^64 - 1) will be rounded up to 2^64, so maximum is excluded
            return value_.dbl >= 0 && value_.dbl < static_cast<double>(std::numeric_limits<std::uint64_t>::max()) ?
                       est::make_optional(static_cast<std::uint64_t>(value_.dbl)) :
                       est::nullopt();
        case dtype::string: {
            est::optional<std::uint64_t> result(est::in_place());
            return basic_stoval(str_view(), *result) ? result : est::nullopt();
        } break;
        case dtype::array: return est::nullopt();
        case dtype::record: return est::nullopt();
        default: UXS_UNREACHABLE_CODE;
    }
}

template<typename CharT, typename Alloc>
est::optional<double> basic_value<CharT, Alloc>::get_double() const {
    switch (type_) {
        case dtype::null: return est::nullopt();
        case dtype::boolean: return value_.b ? 1. : 0.;
        case dtype::integer: return value_.i;
        case dtype::unsigned_integer: return value_.u;
        case dtype::long_integer: return static_cast<double>(value_.i64);
        case dtype::unsigned_long_integer: return static_cast<double>(value_.u64);
        case dtype::double_precision: return value_.dbl;
        case dtype::string: {
            est::optional<double> result(est::in_place());
            return basic_stoval(str_view(), *result) ? result : est::nullopt();
        } break;
        case dtype::array: return est::nullopt();
        case dtype::record: return est::nullopt();
        default: UXS_UNREACHABLE_CODE;
    }
}

template<typename CharT, typename Alloc>
est::optional<std::basic_string<CharT>> basic_value<CharT, Alloc>::get_string() const {
    switch (type_) {
        case dtype::null: {
            return est::make_optional<std::basic_string<CharT>>(string_literal<CharT, 'n', 'u', 'l', 'l'>{}());
        } break;
        case dtype::boolean: {
            return est::make_optional<std::basic_string<CharT>>(value_.b ?
                                                                    string_literal<CharT, 't', 'r', 'u', 'e'>{}() :
                                                                    string_literal<CharT, 'f', 'a', 'l', 's', 'e'>{}());
        } break;
        case dtype::integer: {
            inline_basic_dynbuffer<CharT> buf;
            to_basic_string(buf, value_.i);
            return est::make_optional<std::basic_string<CharT>>(buf.data(), buf.size());
        } break;
        case dtype::unsigned_integer: {
            inline_basic_dynbuffer<CharT> buf;
            to_basic_string(buf, value_.u);
            return est::make_optional<std::basic_string<CharT>>(buf.data(), buf.size());
        } break;
        case dtype::long_integer: {
            inline_basic_dynbuffer<CharT> buf;
            to_basic_string(buf, value_.i64);
            return est::make_optional<std::basic_string<CharT>>(buf.data(), buf.size());
        } break;
        case dtype::unsigned_long_integer: {
            inline_basic_dynbuffer<CharT> buf;
            to_basic_string(buf, value_.u64);
            return est::make_optional<std::basic_string<CharT>>(buf.data(), buf.size());
        } break;
        case dtype::double_precision: {
            inline_basic_dynbuffer<CharT> buf;
            to_basic_string(buf, value_.dbl, fmt_opts{fmt_flags::json_compat});
            return est::make_optional<std::basic_string<CharT>>(buf.data(), buf.size());
        } break;
        case dtype::string: return est::make_optional<std::basic_string<CharT>>(str_view());
        case dtype::array: return est::nullopt();
        case dtype::record: return est::nullopt();
        default: UXS_UNREACHABLE_CODE;
    }
}

template<typename CharT, typename Alloc>
est::optional<std::basic_string_view<CharT>> basic_value<CharT, Alloc>::get_string_view() const {
    return type_ == dtype::string ? est::make_optional(str_view()) : est::nullopt();
}

// --------------------------

template<typename CharT, typename Alloc>
bool basic_value<CharT, Alloc>::is_int() const noexcept {
    switch (type_) {
        case dtype::integer: return true;
        case dtype::unsigned_integer:
            return value_.u <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
        case dtype::long_integer:
            return value_.i64 >= std::numeric_limits<std::int32_t>::min() &&
                   value_.i64 <= std::numeric_limits<std::int32_t>::max();
        case dtype::unsigned_long_integer:
            return value_.u64 <= static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max());
        case dtype::double_precision:
            return value_.dbl >= std::numeric_limits<std::int32_t>::min() &&
                   value_.dbl <= std::numeric_limits<std::int32_t>::max() && detail::is_integral(value_.dbl);
        default: break;
    }
    return false;
}

template<typename CharT, typename Alloc>
bool basic_value<CharT, Alloc>::is_uint() const noexcept {
    switch (type_) {
        case dtype::integer: return value_.i >= 0;
        case dtype::unsigned_integer: return true;
        case dtype::long_integer:
            return value_.i64 >= 0 &&
                   value_.i64 <= static_cast<std::int64_t>(std::numeric_limits<std::uint32_t>::max());
        case dtype::unsigned_long_integer: return value_.u64 <= std::numeric_limits<std::uint32_t>::max();
        case dtype::double_precision:
            return value_.dbl >= 0 && value_.dbl <= std::numeric_limits<std::uint32_t>::max() &&
                   detail::is_integral(value_.dbl);
        default: break;
    }
    return false;
}

template<typename CharT, typename Alloc>
bool basic_value<CharT, Alloc>::is_int64() const noexcept {
    switch (type_) {
        case dtype::integer:
        case dtype::unsigned_integer:
        case dtype::long_integer: return true;
        case dtype::unsigned_long_integer:
            return value_.u64 <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
        case dtype::double_precision:
            // Note that double(2^63 - 1) will be rounded up to 2^63, so maximum is excluded
            return value_.dbl >= static_cast<double>(std::numeric_limits<std::int64_t>::min()) &&
                   value_.dbl < static_cast<double>(std::numeric_limits<std::int64_t>::max()) &&
                   detail::is_integral(value_.dbl);
        default: break;
    }
    return false;
}

template<typename CharT, typename Alloc>
bool basic_value<CharT, Alloc>::is_uint64() const noexcept {
    switch (type_) {
        case dtype::integer: return value_.i >= 0;
        case dtype::unsigned_integer: return true;
        case dtype::long_integer: return value_.i64 >= 0;
        case dtype::unsigned_long_integer: return true;
        case dtype::double_precision:
            // Note that double(2^64 - 1) will be rounded up to 2^64, so maximum is excluded
            return value_.dbl >= 0 && value_.dbl < static_cast<double>(std::numeric_limits<std::uint64_t>::max()) &&
                   detail::is_integral(value_.dbl);
        default: break;
    }
    return false;
}

template<typename CharT, typename Alloc>
bool basic_value<CharT, Alloc>::is_integral() const noexcept {
    switch (type_) {
        case dtype::integer:
        case dtype::unsigned_integer:
        case dtype::long_integer:
        case dtype::unsigned_long_integer: return true;
        case dtype::double_precision:
            // Note that double(2^64 - 1) will be rounded up to 2^64, so maximum is excluded
            return value_.dbl >= static_cast<double>(std::numeric_limits<std::int64_t>::min()) &&
                   value_.dbl < static_cast<double>(std::numeric_limits<std::uint64_t>::max()) &&
                   detail::is_integral(value_.dbl);
        default: break;
    }
    return false;
}

// --------------------------

template<typename CharT, typename Alloc>
std::size_t basic_value<CharT, Alloc>::size() const noexcept {
    switch (type_) {
        case dtype::null: return 0;
        case dtype::array: return value_.arr ? value_.arr->size : 0;
        case dtype::record: return value_.rec->size;
        default: break;
    }
    return 1;
}

template<typename CharT, typename Alloc>
basic_value<CharT, Alloc>& basic_value<CharT, Alloc>::operator[](std::basic_string_view<char_type> key) {
    typename record_t::alloc_type rec_al(*this);
    if (type_ != dtype::record) {
        if (type_ != dtype::null) { throw database_error("not a record"); }
        value_.rec = record_t::create(rec_al);
        type_ = dtype::record;
    }
    const std::size_t hash_code = typename record_t::hasher{}(key);
    detail::list_links_t* node = value_.rec->find(key, hash_code);
    if (node == &value_.rec->head) {
        node = value_.rec->new_node(rec_al, key, static_cast<const Alloc&>(*this));
        value_.rec = record_t::insert(rec_al, value_.rec, hash_code, node);
    }
    return record_t::node_traits::get_value(node).value();
}

template<typename CharT, typename Alloc>
void basic_value<CharT, Alloc>::clear() noexcept {
    if (type_ == dtype::record) {
        typename record_t::alloc_type rec_al(*this);
        value_.rec->clear(rec_al);
    } else if (type_ == dtype::array && value_.arr) {
        for_each(value_.arr->view(), [](basic_value& v) { v.~basic_value(); });
        value_.arr->size = 0;
    }
}

template<typename CharT, typename Alloc>
void basic_value<CharT, Alloc>::reserve(std::size_t sz) {
    if (type_ != dtype::array || !value_.arr) {
        if (type_ != dtype::array && type_ != dtype::null) { throw database_error("not an array"); }
        if (sz) {
            typename value_flexarray_t::alloc_type arr_al(*this);
            value_.arr = value_flexarray_t::alloc_checked(arr_al, sz);
            value_.arr->size = 0;
        } else {
            value_.arr = nullptr;
        }
        type_ = dtype::array;
    } else if (sz > value_.arr->capacity) {
        typename value_flexarray_t::alloc_type arr_al(*this);
        value_.arr = value_flexarray_t::grow(arr_al, value_.arr, sz - value_.arr->size);
    }
}

template<typename CharT, typename Alloc>
void basic_value<CharT, Alloc>::resize(std::size_t sz) {
    reserve(sz);
    if (!value_.arr) { return; }
    if (sz <= value_.arr->size) {
        std::for_each(&(*value_.arr)[sz], &(*value_.arr)[value_.arr->size], [](basic_value& v) { v.~basic_value(); });
    } else {
        std::for_each(&(*value_.arr)[value_.arr->size], &(*value_.arr)[sz],
                      [this](basic_value& v) { new (&v) basic_value(static_cast<const Alloc&>(*this)); });
    }
    value_.arr->size = sz;
}

template<typename CharT, typename Alloc>
void basic_value<CharT, Alloc>::erase(std::size_t pos) {
    if (type_ != dtype::array) { throw database_error("not an array"); }
    assert(value_.arr && pos < value_.arr->size);
    basic_value* v = &(*value_.arr)[pos];
    basic_value* v_end = &(*value_.arr)[--value_.arr->size];
    for (; v != v_end; ++v) { *v = std::move(*(v + 1)); }
    v->~basic_value();
}

template<typename CharT, typename Alloc>
auto basic_value<CharT, Alloc>::erase(const_iterator it) -> iterator {
    if (type_ != dtype::record) { throw database_error("not a record"); }
    if (!it.is_record()) { throw database_error("non-record iterator"); }
    detail::list_links_t* node = static_cast<detail::list_links_t*>(it.ptr_);
    uxs_iterator_assert(record_t::node_traits::get_head(node) == &value_.rec->head);
    assert(node != &value_.rec->head);
    typename record_t::alloc_type rec_al(*this);
    return iterator(value_.rec->erase(rec_al, node));
}

template<typename CharT, typename Alloc>
std::size_t basic_value<CharT, Alloc>::erase(std::basic_string_view<char_type> key) {
    if (type_ != dtype::record) { throw database_error("not a record"); }
    typename record_t::alloc_type rec_al(*this);
    return value_.rec->erase(rec_al, key);
}

// --------------------------

template<typename CharT, typename Alloc>
auto basic_value<CharT, Alloc>::alloc_string(std::basic_string_view<char_type> s) -> char_flexarray_t* {
    if (!s.size()) { return nullptr; }
    typename char_flexarray_t::alloc_type arr_al(*this);
    char_flexarray_t* str = char_flexarray_t::alloc_checked(arr_al, s.size());
    str->size = s.size();
    std::copy_n(s.data(), s.size(), &(*str)[0]);
    return str;
}

template<typename CharT, typename Alloc>
void basic_value<CharT, Alloc>::assign_string(std::basic_string_view<char_type> s) {
    if (!value_.str || s.size() > value_.str->capacity) {
        if (!s.size()) { return; }
        typename char_flexarray_t::alloc_type arr_al(*this);
        char_flexarray_t* new_str = char_flexarray_t::alloc_checked(arr_al, s.size());
        if (value_.str) { char_flexarray_t::dealloc(arr_al, value_.str); }
        value_.str = new_str;
    }
    value_.str->size = s.size();
    std::copy_n(s.data(), s.size(), &(*value_.str)[0]);
}

// --------------------------

template<typename CharT, typename Alloc>
void basic_value<CharT, Alloc>::init_from(const basic_value& other) {
    switch (other.type_) {
        case dtype::string: value_.str = alloc_string(other.str_view()); break;
        case dtype::array: value_.arr = alloc_array(other.array_view()); break;
        case dtype::record: {
            typename record_t::alloc_type rec_al(*this);
            value_.rec = record_t::create(rec_al, *other.value_.rec);
        } break;
        default: value_ = other.value_; break;
    }
}

template<typename CharT, typename Alloc>
void basic_value<CharT, Alloc>::assign_impl(const basic_value& other) {
    if (type_ != other.type_) {
        if (type_ != dtype::null) { destroy(); }
        init_from(other);
        type_ = other.type_;
    } else {
        switch (other.type_) {
            case dtype::string: assign_string(other.str_view()); break;
            case dtype::array: assign_array(other.array_view()); break;
            case dtype::record: {
                typename record_t::alloc_type rec_al(*this);
                value_.rec = record_t::assign(rec_al, value_.rec, *other.value_.rec);
            } break;
            default: value_ = other.value_; break;
        }
    }
}

template<typename CharT, typename Alloc>
void basic_value<CharT, Alloc>::destroy() {
    switch (type_) {
        case dtype::string: {
            if (value_.str) {
                typename char_flexarray_t::alloc_type arr_al(*this);
                char_flexarray_t::dealloc(arr_al, value_.str);
            }
        } break;
        case dtype::array: {
            if (value_.arr) {
                typename value_flexarray_t::alloc_type arr_al(*this);
                for_each(value_.arr->view(), [](basic_value& v) { v.~basic_value(); });
                value_flexarray_t::dealloc(arr_al, value_.arr);
            }
        } break;
        case dtype::record: {
            typename record_t::alloc_type rec_al(*this);
            value_.rec->destroy(rec_al, value_.rec->head.next);
            record_t::dealloc(rec_al, value_.rec);
        } break;
        default: break;
    }
    type_ = dtype::null;
}

template<typename CharT, typename Alloc>
void basic_value<CharT, Alloc>::reserve_back() {
    typename value_flexarray_t::alloc_type arr_al(*this);
    if (type_ != dtype::array || !value_.arr) {
        value_flexarray_t* arr = value_flexarray_t::alloc(arr_al, value_flexarray_t::start_capacity);
        if (type_ != dtype::array && type_ != dtype::null) {
            basic_value* p = &(*arr)[0];
            new (p) basic_value(static_cast<const Alloc&>(*this));
            p->type_ = type_, p->value_ = value_;
            arr->size = 1;
        } else {
            arr->size = 0;
        }
        value_.arr = arr;
        type_ = dtype::array;
    } else {
        value_.arr = value_flexarray_t::grow(arr_al, value_.arr, 1);
    }
}

template<typename CharT, typename Alloc>
void basic_value<CharT, Alloc>::reserve_string(std::size_t extra) {
    typename char_flexarray_t::alloc_type arr_al(*this);
    if (type_ != dtype::string || !value_.str) {
        if (type_ != dtype::string && type_ != dtype::null) { throw database_error("not a string"); }
        value_.str = char_flexarray_t::alloc_checked(arr_al, extra);
        value_.str->size = 0;
        type_ = dtype::string;
    } else {
        value_.str = char_flexarray_t::grow(arr_al, value_.str, extra);
    }
}

template<typename CharT, typename Alloc>
void basic_value<CharT, Alloc>::rotate_back(std::size_t pos) {
    assert(pos != value_.arr->size - 1);
    basic_value* v = &(*value_.arr)[value_.arr->size];
    basic_value t(std::move(*(v - 1)));
    while (--v != &(*value_.arr)[pos]) { *v = std::move(*(v - 1)); }
    *v = std::move(t);
}

}  // namespace db
}  // namespace uxs
