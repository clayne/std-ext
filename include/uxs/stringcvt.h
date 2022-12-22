#pragma once

#include "chars.h"
#include "iterator.h"
#include "string_view.h"

#include <algorithm>
#include <memory>
#include <stdexcept>

namespace std {
class locale;
}

namespace uxs {

// --------------------------

template<typename InputIt, typename InputFn = nofunc>
unsigned from_hex(InputIt in, unsigned n_digs, InputFn fn = InputFn{}, unsigned* n_valid = nullptr) {
    unsigned val = 0;
    if (n_valid) { *n_valid = n_digs; }
    while (n_digs) {
        unsigned dig = dig_v(fn(*in));
        if (dig < 16) {
            val = (val << 4) | dig;
        } else {
            if (n_valid) { *n_valid -= n_digs; }
            return val;
        }
        ++in, --n_digs;
    }
    return val;
}

template<typename OutputIt, typename OutputFn = nofunc>
void to_hex(unsigned val, OutputIt out, unsigned n_digs, bool upper = false, OutputFn fn = OutputFn{}) {
    const char* digs = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    unsigned shift = n_digs << 2;
    while (shift) {
        shift -= 4;
        *out++ = fn(digs[(val >> shift) & 0xf]);
    }
}

// --------------------------

enum class fmt_flags : unsigned {
    kDefault = 0,
    kDec = kDefault,
    kBin = 1,
    kOct = 2,
    kHex = 3,
    kBaseField = 3,
    kFixed = 4,
    kScientific = 8,
    kGeneral = 0xc,
    kFloatField = 0xc,
    kLeft = 0x10,
    kRight = 0x20,
    kInternal = 0x30,
    kAdjustField = 0x30,
    kLeadingZeroes = 0x40,
    kUpperCase = 0x80,
    kAlternate = 0x100,
    kSignNeg = kDefault,
    kSignPos = 0x200,
    kSignAlign = 0x400,
    kSignField = 0x600,
};
UXS_IMPLEMENT_BITWISE_OPS_FOR_ENUM(fmt_flags, unsigned);

struct fmt_opts {
    CONSTEXPR fmt_opts() = default;
    CONSTEXPR fmt_opts(fmt_flags fl) : flags(fl) {}
    CONSTEXPR fmt_opts(fmt_flags fl, int p) : flags(fl), prec(p) {}
    CONSTEXPR fmt_opts(fmt_flags fl, int p, const std::locale* l) : flags(fl), prec(p), loc(l) {}
    CONSTEXPR fmt_opts(fmt_flags fl, int p, const std::locale* l, unsigned w, int ch)
        : flags(fl), prec(p), loc(l), width(w), fill(ch) {}
    fmt_flags flags = fmt_flags::kDec;
    int prec = -1;
    const std::locale* loc = nullptr;
    unsigned width = 0;
    int fill = ' ';
};

// --------------------------

template<typename CharT, typename Appender>
class basic_appender_mixin {
 public:
    using value_type = CharT;
    template<typename Range, typename = std::void_t<decltype(std::declval<Range>().end())>>
    Appender& operator+=(const Range& r) {
        return static_cast<Appender&>(*this).append(r.begin(), r.end());
    }
    Appender& operator+=(const value_type* s) { return *this += std::basic_string_view<value_type>(s); }
    Appender& operator+=(value_type ch) {
        static_cast<Appender&>(*this).push_back(ch);
        return static_cast<Appender&>(*this);
    }
};

template<typename CharT>
class basic_unlimbuf_appender : public basic_appender_mixin<CharT, basic_unlimbuf_appender<CharT>> {
 public:
    explicit basic_unlimbuf_appender(CharT* dst) : curr_(dst) {}
    basic_unlimbuf_appender(const basic_unlimbuf_appender&) = delete;
    basic_unlimbuf_appender& operator=(const basic_unlimbuf_appender&) = delete;
    CharT& back() { return *(curr_ - 1); }
    CharT* curr() { return curr_; }
    basic_unlimbuf_appender& advance(unsigned len) {
        curr_ += len;
        return *this;
    }

    template<typename InputIt, typename = std::enable_if_t<is_random_access_iterator<InputIt>::value>>
    basic_unlimbuf_appender& append(InputIt first, InputIt last) {
        curr_ = std::copy(first, last, curr_);
        return *this;
    }
    basic_unlimbuf_appender& append(size_t count, CharT ch) {
        curr_ = std::fill_n(curr_, count, ch);
        return *this;
    }
    void push_back(CharT ch) { *curr_++ = ch; }

 private:
    CharT* curr_;
};

using unlimbuf_appender = basic_unlimbuf_appender<char>;
using wunlimbuf_appender = basic_unlimbuf_appender<wchar_t>;

template<typename CharT>
class basic_limbuf_appender : public basic_appender_mixin<CharT, basic_limbuf_appender<CharT>> {
 public:
    basic_limbuf_appender(CharT* dst, size_t n) : curr_(dst), last_(dst + n) {}
    basic_limbuf_appender(const basic_limbuf_appender&) = delete;
    basic_limbuf_appender& operator=(const basic_limbuf_appender&) = delete;
    CharT& back() { return *(curr_ - 1); }
    CharT* curr() { return curr_; }
    basic_limbuf_appender& advance(unsigned len) {
        assert(curr_ + len <= last_);
        curr_ += len;
        return *this;
    }

    template<typename InputIt, typename = std::enable_if_t<is_random_access_iterator<InputIt>::value>>
    basic_limbuf_appender& append(InputIt first, InputIt last) {
        curr_ = std::copy_n(first, std::min<size_t>(last - first, last_ - curr_), curr_);
        return *this;
    }
    basic_limbuf_appender& append(size_t count, CharT ch) {
        curr_ = std::fill_n(curr_, std::min<size_t>(count, last_ - curr_), ch);
        return *this;
    }
    void push_back(CharT ch) {
        if (curr_ != last_) { *curr_++ = ch; }
    }

 private:
    CharT *curr_, *last_;
};

using limbuf_appender = basic_limbuf_appender<char>;
using wlimbuf_appender = basic_limbuf_appender<wchar_t>;

template<typename Ty, typename Alloc = std::allocator<Ty>>
class basic_dynbuffer : protected std::allocator_traits<Alloc>::template rebind_alloc<Ty>,
                        public basic_appender_mixin<Ty, basic_dynbuffer<Ty>> {
 private:
    static_assert(std::is_trivially_copyable<Ty>::value && std::is_trivially_destructible<Ty>::value,
                  "uxs::basic_dynbuffer<> must have trivially copyable and destructible value type");

    using alloc_type = typename std::allocator_traits<Alloc>::template rebind_alloc<Ty>;

 public:
    using value_type = Ty;

    basic_dynbuffer(Ty* first, Ty* last, bool is_inline)
        : alloc_type(), curr_(first), first_(first), last_(last), is_inline_(is_inline) {}
    basic_dynbuffer(Ty* first, Ty* last, bool is_inline, const Alloc& al)
        : alloc_type(al), curr_(first), first_(first), last_(last), is_inline_(is_inline) {}
    ~basic_dynbuffer() {
        if (!is_inline_) { this->deallocate(first_, last_ - first_); }
    }
    basic_dynbuffer(const basic_dynbuffer&) = delete;
    basic_dynbuffer& operator=(const basic_dynbuffer&) = delete;

    bool empty() const { return first_ == curr_; }
    size_t size() const { return curr_ - first_; }
    size_t avail() const { return last_ - curr_; }
    const Ty* data() const { return first_; }
    Ty& back() {
        assert(first_ < curr_);
        return *(curr_ - 1);
    }
    Ty* first() { return first_; }
    Ty* curr() { return curr_; }
    Ty** p_curr() { return &curr_; }
    basic_dynbuffer& advance(unsigned len) {
        assert(curr_ + len <= last_);
        curr_ += len;
        return *this;
    }
    Ty* last() { return last_; }
    void clear() { curr_ = first_; }

    Ty* reserve_at_curr(size_t count) {
        if (static_cast<size_t>(last_ - curr_) < count) { grow(count); }
        return curr_;
    }

    template<typename InputIt, typename = std::enable_if_t<is_random_access_iterator<InputIt>::value>>
    basic_dynbuffer& append(InputIt first, InputIt last) {
        if (static_cast<size_t>(last_ - curr_) < static_cast<size_t>(last - first)) { grow(last - first); }
        curr_ = std::uninitialized_copy(first, last, curr_);
        return *this;
    }
    basic_dynbuffer& append(size_t count, Ty val) {
        if (static_cast<size_t>(last_ - curr_) < count) { grow(count); }
        curr_ = std::uninitialized_fill_n(curr_, count, val);
        return *this;
    }
    void push_back(Ty val) {
        if (curr_ == last_) { grow(1); }
        new (curr_) Ty(val);
        ++curr_;
    }
    template<typename... Args>
    void emplace_back(Args&&... args) {
        if (curr_ == last_) { grow(1); }
        new (curr_) Ty(std::forward<Args>(args)...);
        ++curr_;
    }
    void pop_back() {
        assert(first_ < curr_);
        --curr_;
    }

 private:
    Ty *curr_, *first_, *last_;
    bool is_inline_;

    void grow(size_t extra);
};

template<typename Ty, typename Alloc>
void basic_dynbuffer<Ty, Alloc>::grow(size_t extra) {
    size_t sz = curr_ - first_, delta_sz = std::max(extra, sz >> 1);
    const size_t max_avail = std::allocator_traits<alloc_type>::max_size(*this) - sz;
    if (delta_sz > max_avail) {
        if (extra > max_avail) { throw std::length_error("too much to reserve"); }
        delta_sz = std::max(extra, max_avail >> 1);
    }
    sz += delta_sz;
    Ty* first = this->allocate(sz);
    curr_ = std::uninitialized_copy(first_, curr_, first);
    if (!is_inline_) { this->deallocate(first_, last_ - first_); }
    first_ = first, last_ = first + sz, is_inline_ = false;
}

using dynbuffer = basic_dynbuffer<char>;
using wdynbuffer = basic_dynbuffer<wchar_t>;

template<typename Ty, size_t InlineBufSize = 0, typename Alloc = std::allocator<Ty>>
class basic_inline_dynbuffer : public basic_dynbuffer<Ty, Alloc> {
 public:
    basic_inline_dynbuffer()
        : basic_dynbuffer<Ty>(reinterpret_cast<Ty*>(&buf_), reinterpret_cast<Ty*>(&buf_[kInlineBufSize]), true) {}
    explicit basic_inline_dynbuffer(const Alloc& al)
        : basic_dynbuffer<Ty>(reinterpret_cast<Ty*>(&buf_), reinterpret_cast<Ty*>(&buf_[kInlineBufSize]), true, al) {}
    basic_dynbuffer<Ty>& base() { return *this; }

 private:
    enum : unsigned {
#if defined(NDEBUG) || !defined(_DEBUG_REDUCED_BUFFERS)
        kInlineBufSize = InlineBufSize != 0 ? InlineBufSize : 256 / sizeof(Ty)
#else   // defined(NDEBUG) || !defined(_DEBUG_REDUCED_BUFFERS)
        kInlineBufSize = 7
#endif  // defined(NDEBUG) || !defined(_DEBUG_REDUCED_BUFFERS)
    };
    typename std::aligned_storage<sizeof(Ty), std::alignment_of<Ty>::value>::type buf_[kInlineBufSize];
};

using inline_dynbuffer = basic_inline_dynbuffer<char>;
using inline_wdynbuffer = basic_inline_dynbuffer<wchar_t>;

// --------------------------

template<typename StrTy, typename Func>
StrTy& append_adjusted(StrTy& s, Func fn, unsigned len, const fmt_opts& fmt) {
    unsigned left = fmt.width - len, right = left;
    if (!(fmt.flags & fmt_flags::kLeadingZeroes)) {
        switch (fmt.flags & fmt_flags::kAdjustField) {
            case fmt_flags::kRight: right = 0; break;
            case fmt_flags::kInternal: left >>= 1, right -= left; break;
            case fmt_flags::kLeft:
            default: left = 0; break;
        }
    } else {
        right = 0;
    }
    s.append(left, fmt.fill);
    return fn(s).append(right, fmt.fill);
}

// --------------------------

namespace scvt {

template<typename Ty>
struct fp_traits;

template<>
struct fp_traits<double> {
    static_assert(sizeof(double) == sizeof(uint64_t), "type size mismatch");
    enum : unsigned { kTotalBits = 64, kBitsPerMantissa = 52 };
    enum : uint64_t { kMantissaMask = (1ull << kBitsPerMantissa) - 1 };
    enum : int { kExpMax = (1 << (kTotalBits - kBitsPerMantissa - 1)) - 1 };
    static uint64_t to_u64(const double& f) { return *reinterpret_cast<const uint64_t*>(&f); }
    static double from_u64(const uint64_t& u64) { return *reinterpret_cast<const double*>(&u64); }
};

template<>
struct fp_traits<float> {
    static_assert(sizeof(float) == sizeof(uint32_t), "type size mismatch");
    enum : unsigned { kTotalBits = 32, kBitsPerMantissa = 23 };
    enum : uint64_t { kMantissaMask = (1ull << kBitsPerMantissa) - 1 };
    enum : int { kExpMax = (1 << (kTotalBits - kBitsPerMantissa - 1)) - 1 };
    static uint64_t to_u64(const float& f) { return *reinterpret_cast<const uint32_t*>(&f); }
    static float from_u64(const uint64_t& u64) { return *reinterpret_cast<const float*>(&u64); }
};

// --------------------------

template<typename Ty>
struct type_substitution {
    using type = Ty;
};
template<>
struct type_substitution<unsigned char> {
    using type = uint32_t;
};
template<>
struct type_substitution<unsigned short> {
    using type = uint32_t;
};
template<>
struct type_substitution<unsigned> {
    using type = uint32_t;
};
template<>
struct type_substitution<unsigned long> {
    using type = std::conditional_t<sizeof(long) == sizeof(int32_t), uint32_t, uint64_t>;
};
template<>
struct type_substitution<unsigned long long> {
    using type = uint64_t;
};
template<>
struct type_substitution<signed char> {
    using type = int32_t;
};
template<>
struct type_substitution<signed short> {
    using type = int32_t;
};
template<>
struct type_substitution<signed> {
    using type = int32_t;
};
template<>
struct type_substitution<signed long> {
    using type = std::conditional_t<sizeof(long) == sizeof(int32_t), int32_t, int64_t>;
};
template<>
struct type_substitution<signed long long> {
    using type = int64_t;
};
template<>
struct type_substitution<long double> {
    using type = double;
};

// --------------------------

template<typename Ty, typename CharT>
UXS_EXPORT Ty to_integer_limited(const CharT* p, const CharT* end, const CharT*& last, Ty pos_limit) NOEXCEPT;

template<typename Ty, typename CharT>
Ty to_integer(const CharT* p, const CharT* end, const CharT*& last) NOEXCEPT {
    using UTy = typename std::make_unsigned<Ty>::type;
    return to_integer_limited<typename type_substitution<UTy>::type>(p, end, last, std::numeric_limits<UTy>::max());
}

template<typename Ty, typename CharT>
Ty to_char(const CharT* p, const CharT* end, const CharT*& last) NOEXCEPT {
    last = p;
    if (p == end) { return '\0'; }
    ++last;
    return *p;
}

template<typename CharT>
UXS_EXPORT uint64_t to_float_common(const CharT* p, const CharT* end, const CharT*& last, const unsigned bpm,
                                    const int exp_max) NOEXCEPT;

template<typename Ty, typename CharT>
Ty to_float(const CharT* p, const CharT* end, const CharT*& last) NOEXCEPT {
    using SubstTy = typename type_substitution<Ty>::type;
    return fp_traits<SubstTy>::from_u64(
        to_float_common(p, end, last, fp_traits<SubstTy>::kBitsPerMantissa, fp_traits<SubstTy>::kExpMax));
}

template<typename Ty, typename CharT>
UXS_EXPORT Ty to_bool(const CharT* p, const CharT* end, const CharT*& last) NOEXCEPT;

// --------------------------

template<typename StrTy, typename Ty>
UXS_EXPORT StrTy& fmt_integral(StrTy& s, Ty val, const fmt_opts& fmt);

template<typename StrTy, typename Ty>
UXS_EXPORT StrTy& fmt_char(StrTy& s, Ty val, const fmt_opts& fmt);

template<typename StrTy>
UXS_EXPORT StrTy& fmt_float_common(StrTy& s, uint64_t u64, const fmt_opts& fmt, const unsigned bpm, const int exp_max);

template<typename StrTy, typename Ty>
StrTy& fmt_float(StrTy& s, Ty val, const fmt_opts& fmt) {
    return fmt_float_common(s, fp_traits<Ty>::to_u64(val), fmt, fp_traits<Ty>::kBitsPerMantissa, fp_traits<Ty>::kExpMax);
}

template<typename StrTy, typename Ty>
UXS_EXPORT StrTy& fmt_bool(StrTy& s, Ty val, const fmt_opts& fmt);

}  // namespace scvt

// --------------------------

template<typename Ty, typename CharT>
struct string_parser;

template<typename Ty, typename CharT>
struct formatter;

namespace detail {
template<typename Ty, typename CharT>
struct has_string_parser {
    template<typename U>
    static auto test(const U* first, const U* last, Ty& v)
        -> std::is_same<decltype(string_parser<Ty, U>::from_chars(first, last, v)), const U*>;
    template<typename U>
    static auto test(...) -> std::false_type;
    using type = decltype(test<CharT>(nullptr, nullptr, std::declval<Ty&>()));
};
template<typename Ty, typename StrTy>
struct has_formatter {
    template<typename U>
    static auto test(U& s, const Ty& v)
        -> std::is_same<decltype(formatter<Ty, typename U::value_type>::format(s, v, fmt_opts{})), U&>;
    template<typename U>
    static auto test(...) -> std::false_type;
    using type = decltype(test<StrTy>(std::declval<StrTy&>(), std::declval<Ty>()));
};
}  // namespace detail

template<typename Ty, typename CharT>
struct has_string_parser : detail::has_string_parser<Ty, CharT>::type {};

template<typename Ty, typename StrTy>
struct has_formatter : detail::has_formatter<Ty, StrTy>::type {};

#define UXS_SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER(ty, from_chars_func, fmt_func) \
    template<typename CharT> \
    struct string_parser<ty, CharT> { \
        static ty default_value() NOEXCEPT { return {}; } \
        static const CharT* from_chars(const CharT* first, const CharT* last, ty& val) NOEXCEPT { \
            auto t = scvt::from_chars_func<ty>(first, last, last); \
            if (last != first) { val = static_cast<ty>(t); } \
            return last; \
        } \
    }; \
    template<typename CharT> \
    struct formatter<ty, CharT> { \
        template<typename StrTy> \
        static StrTy& format(StrTy& s, ty val, const fmt_opts& fmt) { \
            using Ty = scvt::type_substitution<ty>::type; \
            return scvt::fmt_func<StrTy, Ty>(s, static_cast<Ty>(val), fmt); \
        } \
    };
UXS_SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER(unsigned char, to_integer, fmt_integral)
UXS_SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER(unsigned short, to_integer, fmt_integral)
UXS_SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER(unsigned, to_integer, fmt_integral)
UXS_SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER(unsigned long, to_integer, fmt_integral)
UXS_SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER(unsigned long long, to_integer, fmt_integral)
UXS_SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER(signed char, to_integer, fmt_integral)
UXS_SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER(signed short, to_integer, fmt_integral)
UXS_SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER(signed, to_integer, fmt_integral)
UXS_SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER(signed long, to_integer, fmt_integral)
UXS_SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER(signed long long, to_integer, fmt_integral)
UXS_SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER(char, to_char, fmt_char)
UXS_SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER(wchar_t, to_char, fmt_char)
UXS_SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER(bool, to_bool, fmt_bool)
UXS_SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER(float, to_float, fmt_float)
UXS_SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER(double, to_float, fmt_float)
UXS_SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER(long double, to_float, fmt_float)
#undef SCVT_IMPLEMENT_STANDARD_STRING_CONVERTER

template<>
struct string_parser<char, wchar_t>;  // = delete

template<>
struct formatter<wchar_t, char>;  // = delete

template<typename Ty>
const char* from_chars(const char* first, const char* last, Ty& v) {
    return string_parser<Ty, char>::from_chars(first, last);
}

template<typename Ty>
const wchar_t* from_wchars(const wchar_t* first, const wchar_t* last, Ty& v) {
    return string_parser<Ty, wchar_t>::from_chars(first, last);
}

template<typename Ty, typename CharT, typename Traits = std::char_traits<CharT>>
size_t basic_stoval(std::basic_string_view<CharT, Traits> s, Ty& v) {
    return string_parser<Ty, CharT>::from_chars(s.data(), s.data() + s.size(), v) - s.data();
}

template<typename Ty>
size_t stoval(std::string_view s, Ty& v) {
    return basic_stoval(s, v);
}

template<typename Ty>
size_t wstoval(std::wstring_view s, Ty& v) {
    return basic_stoval(s, v);
}

template<typename Ty, typename CharT, typename Traits = std::char_traits<CharT>>
NODISCARD Ty from_basic_string(std::basic_string_view<CharT, Traits> s) {
    Ty result(string_parser<Ty, CharT>::default_value());
    basic_stoval(s, result);
    return result;
}

template<typename Ty>
NODISCARD Ty from_string(std::string_view s) {
    return from_basic_string<Ty>(s);
}

template<typename Ty>
NODISCARD Ty from_wstring(std::wstring_view s) {
    return from_basic_string<Ty>(s);
}

template<typename StrTy, typename Ty>
StrTy& to_basic_string(StrTy& s, const Ty& val, const fmt_opts& fmt = fmt_opts{}) {
    return formatter<Ty, typename StrTy::value_type>::format(s, val, fmt);
}

template<typename Ty, typename... Args>
NODISCARD std::string to_string(const Ty& val, Args&&... args) {
    inline_dynbuffer buf;
    to_basic_string(buf.base(), val, fmt_opts{std::forward<Args>(args)...});
    return std::string(buf.data(), buf.size());
}

template<typename Ty, typename... Args>
NODISCARD std::wstring to_wstring(const Ty& val, Args&&... args) {
    inline_wdynbuffer buf;
    to_basic_string(buf.base(), val, fmt_opts{std::forward<Args>(args)...});
    return std::wstring(buf.data(), buf.size());
}

template<typename Ty, typename... Args>
char* to_chars(char* buf, const Ty& val, Args&&... args) {
    unlimbuf_appender appender(buf);
    return to_basic_string(appender, val, fmt_opts{std::forward<Args>(args)...}).curr();
}

template<typename Ty, typename... Args>
wchar_t* to_wchars(wchar_t* buf, const Ty& val, Args&&... args) {
    wunlimbuf_appender appender(buf);
    return to_basic_string(appender, val, fmt_opts{std::forward<Args>(args)...}).curr();
}

template<typename Ty, typename... Args>
char* to_chars_n(char* buf, size_t n, const Ty& val, Args&&... args) {
    limbuf_appender appender(buf, n);
    return to_basic_string(appender, val, fmt_opts{std::forward<Args>(args)...}).curr();
}

template<typename Ty, typename... Args>
wchar_t* to_wchars_n(wchar_t* buf, size_t n, const Ty& val, Args&&... args) {
    wlimbuf_appender appender(buf, n);
    return to_basic_string(appender, val, fmt_opts{std::forward<Args>(args)...}).curr();
}

}  // namespace uxs
