#pragma once

#include "util/io/iobuf.h"
#include "util/map.h"
#include "util/span.h"
#include "util/string_view.h"

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace util {
namespace db {

class exception : public std::runtime_error {
 public:
    explicit exception(const char* message) : std::runtime_error(message) {}
    explicit exception(const std::string& message) : std::runtime_error(message) {}
};

class UTIL_EXPORT value {
 private:
    using record = util::map<std::string, value, util::less<>>;

    template<typename Ty>
    struct dynarray {
        size_t size;
        size_t capacity;
        Ty data[1];
        span<const Ty> view() const { return as_span(data, size); }
        span<Ty> view() { return as_span(data, size); }
        static size_t get_alloc_sz(size_t cap) {
            return (offsetof(dynarray, data[cap]) + sizeof(dynarray) - 1) / sizeof(dynarray);
        }
        static dynarray* alloc(size_t cap);
        static dynarray* grow(dynarray* arr, size_t extra);
        static void dealloc(dynarray* arr);
    };

 public:
    enum class dtype {
        kNull = 0,
        kBoolean,
        kInteger,
        kUInteger,
        kInteger64,
        kUInteger64,
        kDouble,
        kString,
        kArray,
        kRecord,
    };

    using record_iterator = record::iterator;
    using const_record_iterator = record::const_iterator;

    value() : type_(dtype::kNull) { value_.i64 = 0; }
    value(bool b) : type_(dtype::kBoolean) { value_.b = b; }
    value(int32_t i) : type_(dtype::kInteger) { value_.i = i; }
    value(uint32_t u) : type_(dtype::kUInteger) { value_.u = u; }
    value(int64_t i) : type_(dtype::kInteger64) { value_.i64 = i; }
    value(uint64_t u) : type_(dtype::kUInteger64) { value_.u64 = u; }
    value(double d) : type_(dtype::kDouble) { value_.dbl = d; }
    value(std::string_view s) : type_(dtype::kString) { value_.str = copy_string(s); }
    value(const char* cstr) : type_(dtype::kString) { value_.str = copy_string(std::string_view(cstr)); }

    static value make_empty_array() {
        value v;
        v.value_.arr = nullptr;
        v.type_ = dtype::kArray;
        return v;
    }

    static value make_empty_record() {
        value v;
        v.value_.rec = new record();
        v.type_ = dtype::kRecord;
        return v;
    }

    ~value() {
        if (type_ != dtype::kNull) { destroy(); }
    }
    value(value&& other) : type_(other.type_), value_(other.value_) { other.type_ = dtype::kNull; }
    value& operator=(value&& other) {
        if (&other == this) { return *this; }
        if (type_ != dtype::kNull) { destroy(); }
        type_ = other.type_, value_ = other.value_;
        other.type_ = dtype::kNull;
        return *this;
    }
    value(const value& other) : type_(other.type_) { init_from(other); }
    value& operator=(const value& other) {
        if (&other == this) { return *this; }
        copy_from(other);
        return *this;
    }

#define PARSER_VALUE_IMPLEMENT_SCALAR_ASSIGNMENT(ty, id, field) \
    value& operator=(ty v) { \
        if (type_ != dtype::kNull) { destroy(); } \
        type_ = dtype::id, value_.field = v; \
        return *this; \
    }
    PARSER_VALUE_IMPLEMENT_SCALAR_ASSIGNMENT(bool, kBoolean, b)
    PARSER_VALUE_IMPLEMENT_SCALAR_ASSIGNMENT(int32_t, kInteger, i)
    PARSER_VALUE_IMPLEMENT_SCALAR_ASSIGNMENT(uint32_t, kUInteger, u)
    PARSER_VALUE_IMPLEMENT_SCALAR_ASSIGNMENT(int64_t, kInteger64, i64)
    PARSER_VALUE_IMPLEMENT_SCALAR_ASSIGNMENT(uint64_t, kUInteger64, u64)
    PARSER_VALUE_IMPLEMENT_SCALAR_ASSIGNMENT(double, kDouble, dbl)
#undef PARSER_VALUE_IMPLEMENT_SCALAR_ASSIGNMENT

    value& operator=(std::string_view s) {
        if (type_ != dtype::kString) {
            if (type_ != dtype::kNull) { destroy(); }
            value_.str = copy_string(s);
        } else {
            value_.str = assign_string(value_.str, s);
        }
        type_ = dtype::kString;
        return *this;
    }

    value& operator=(const char* cstr) { return (*this = std::string_view(cstr)); }

    friend bool operator==(const value& lhs, const value& rhs);
    friend bool operator!=(const value& lhs, const value& rhs) { return !(lhs == rhs); }

    dtype type() const { return type_; }

    template<typename Ty>
    bool is() const = delete;

    template<typename Ty>
    Ty as() const = delete;

    template<typename Ty>
    Ty get(Ty def) const = delete;

    template<typename Ty>
    Ty get(std::string_view name, Ty def) const = delete;

    bool is_null() const { return type_ == dtype::kNull; }
    bool is_bool() const { return type_ == dtype::kBoolean; }
    bool is_int() const;
    bool is_uint() const;
    bool is_int64() const;
    bool is_uint64() const;
    bool is_integral() const;
    bool is_float() const { return is_numeric(); }
    bool is_double() const { return is_numeric(); }
    bool is_numeric() const { return type_ >= dtype::kInteger && type_ <= dtype::kDouble; }
    bool is_string() const { return type_ == dtype::kString; }
    bool is_array() const { return type_ == dtype::kArray; }
    bool is_record() const { return type_ == dtype::kRecord; }

    bool as_bool(bool& res) const;
    bool as_int(int32_t& res) const;
    bool as_uint(uint32_t& res) const;
    bool as_int64(int64_t& res) const;
    bool as_uint64(uint64_t& res) const;
    bool as_float(float& res) const;
    bool as_double(double& res) const;
    bool as_string(std::string& res) const;
    bool as_string_view(std::string_view& res) const;

    bool as_bool() const;
    int32_t as_int() const;
    uint32_t as_uint() const;
    int64_t as_int64() const;
    uint64_t as_uint64() const;
    float as_float() const;
    double as_double() const;
    std::string as_string() const;
    std::string_view as_string_view() const;

    bool convert(dtype type);
    void print_scalar(iobuf& out) const;

    size_t size() const;
    bool empty() const;
    bool contains(std::string_view name) const;
    std::vector<std::string_view> members() const;
    explicit operator bool() const { return !is_null(); }

    span<const value> as_array() const;
    span<value> as_array();

    iterator_range<const_record_iterator> as_map() const;
    iterator_range<record_iterator> as_map();

    const value& operator[](size_t i) const;
    value& operator[](size_t i);

    const value& operator[](std::string_view name) const;
    value& operator[](std::string_view name);

    const value* find(std::string_view name) const;
    value* find(std::string_view name) { return const_cast<value*>(std::as_const(*this).find(name)); }

    void clear();
    void resize(size_t sz);
    void nullify() {
        if (type_ == dtype::kNull) { return; }
        destroy();
        type_ = dtype::kNull;
    }

    value& append(std::string_view s);
    void push_back(char ch);

    template<typename... Args>
    value& emplace_back(Args&&... args);
    void push_back(const value& v) { emplace_back(v); }
    void push_back(value&& v) { emplace_back(std::move(v)); }

    template<typename... Args>
    value& emplace(size_t pos, Args&&... args);
    void insert(size_t pos, const value& v) { emplace(pos, v); }
    void insert(size_t pos, value&& v) { emplace(pos, std::move(v)); }

    template<typename... Args>
    value& emplace(std::string name, Args&&... args);
    void insert(std::string name, const value& v) { emplace(std::move(name), v); }
    void insert(std::string name, value&& v) { emplace(std::move(name), std::move(v)); }

    void remove(size_t pos);
    void remove(size_t pos, value& removed);
    bool remove(std::string_view name);
    bool remove(std::string_view name, value& removed);

 private:
    enum : unsigned { kMinCapacity = 8 };

    dtype type_;

    union {
        bool b;
        int32_t i;
        uint32_t u;
        int64_t i64;
        uint64_t u64;
        double dbl;
        dynarray<char>* str;
        dynarray<value>* arr;
        record* rec;
    } value_;

    std::string_view str_view() const {
        return value_.str ? std::string_view(value_.str->data, value_.str->size) : std::string_view();
    }
    span<const value> array_view() const { return value_.arr ? value_.arr->view() : span<value>(); }
    span<value> array_view() { return value_.arr ? value_.arr->view() : span<value>(); }

    static dynarray<char>* copy_string(span<const char> s);
    static dynarray<char>* assign_string(dynarray<char>* str, span<const char> s);

    static dynarray<value>* copy_array(span<const value> v);
    static dynarray<value>* assign_array(dynarray<value>* arr, span<const value> v);

    void init_from(const value& other);
    void copy_from(const value& other);
    void destroy();

    void reserve_back();
    void rotate_back(size_t pos);
};

template<typename... Args>
value& value::emplace_back(Args&&... args) {
    if (type_ != dtype::kArray || !value_.arr || value_.arr->size == value_.arr->capacity) { reserve_back(); }
    value& v = *new (&value_.arr->data[value_.arr->size]) value(std::forward<Args>(args)...);
    ++value_.arr->size;
    return v;
}

template<typename... Args>
value& value::emplace(size_t pos, Args&&... args) {
    if (type_ != dtype::kArray || !value_.arr || value_.arr->size == value_.arr->capacity) { reserve_back(); }
    if (pos > value_.arr->size) { throw exception("index out of range"); }
    value& v = *new (&value_.arr->data[value_.arr->size]) value(std::forward<Args>(args)...);
    ++value_.arr->size;
    if (pos != value_.arr->size - 1) { rotate_back(pos); }
    return v;
}

template<typename... Args>
value& value::emplace(std::string name, Args&&... args) {
    if (type_ != dtype::kRecord) {
        if (type_ != dtype::kNull) { throw exception("not a record"); }
        value_.rec = new record();
        type_ = dtype::kRecord;
    }
    return value_.rec
        ->emplace(std::piecewise_construct, std::forward_as_tuple(std::move(name)),
                  std::forward_as_tuple(std::forward<Args>(args)...))
        .first->second;
}

#define PARSER_VALUE_IMPLEMENT_SCALAR_IS_GET(ty, is_func) \
    template<> \
    inline bool value::is<ty>() const { \
        return this->is_func(); \
    }
PARSER_VALUE_IMPLEMENT_SCALAR_IS_GET(bool, is_bool)
PARSER_VALUE_IMPLEMENT_SCALAR_IS_GET(int32_t, is_int)
PARSER_VALUE_IMPLEMENT_SCALAR_IS_GET(uint32_t, is_uint)
PARSER_VALUE_IMPLEMENT_SCALAR_IS_GET(int64_t, is_int64)
PARSER_VALUE_IMPLEMENT_SCALAR_IS_GET(uint64_t, is_uint64)
PARSER_VALUE_IMPLEMENT_SCALAR_IS_GET(float, is_float)
PARSER_VALUE_IMPLEMENT_SCALAR_IS_GET(double, is_double)
PARSER_VALUE_IMPLEMENT_SCALAR_IS_GET(std::string, is_string)
#undef PARSER_VALUE_IMPLEMENT_SCALAR_IS_GET

#define PARSER_VALUE_IMPLEMENT_SCALAR_AS_GET(ty, as_func) \
    inline ty value::as_func() const { \
        ty res; \
        if (!this->as_func(res)) { throw exception("not convertible to " #ty); } \
        return res; \
    } \
    template<> \
    inline ty value::as<ty>() const { \
        return this->as_func(); \
    } \
    template<> \
    inline ty value::get(ty def) const { \
        ty res(std::move(def)); \
        this->as_func(res); \
        return res; \
    } \
    template<> \
    inline ty value::get(std::string_view name, ty def) const { \
        ty res(std::move(def)); \
        if (const value* v = this->find(name)) { v->as_func(res); } \
        return res; \
    }

PARSER_VALUE_IMPLEMENT_SCALAR_AS_GET(bool, as_bool)
PARSER_VALUE_IMPLEMENT_SCALAR_AS_GET(int32_t, as_int)
PARSER_VALUE_IMPLEMENT_SCALAR_AS_GET(uint32_t, as_uint)
PARSER_VALUE_IMPLEMENT_SCALAR_AS_GET(int64_t, as_int64)
PARSER_VALUE_IMPLEMENT_SCALAR_AS_GET(uint64_t, as_uint64)
PARSER_VALUE_IMPLEMENT_SCALAR_AS_GET(float, as_float)
PARSER_VALUE_IMPLEMENT_SCALAR_AS_GET(double, as_double)
PARSER_VALUE_IMPLEMENT_SCALAR_AS_GET(std::string, as_string)
PARSER_VALUE_IMPLEMENT_SCALAR_AS_GET(std::string_view, as_string_view)
#undef PARSER_VALUE_IMPLEMENT_SCALAR_AS_GET

}  // namespace db
}  // namespace util