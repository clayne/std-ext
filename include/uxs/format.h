#pragma once

#include "alignment.h"
#include "io/iobuf.h"
#include "stringcvt.h"

namespace uxs {

template<typename StrTy>
class basic_format_context;

template<typename CharT>
class basic_format_parse_context;

using format_context = basic_format_context<membuffer>;
using wformat_context = basic_format_context<wmembuffer>;
using format_parse_context = basic_format_parse_context<char>;
using wformat_parse_context = basic_format_parse_context<wchar_t>;

namespace detail {
template<typename Ty, typename FmtCtx>
struct has_formatter {
    template<typename F, typename U>
    static auto test(U& ctx, const Ty& v)
        -> always_true<decltype(formatter<Ty, typename U::char_type>().format(ctx, v))>;
    template<typename U>
    static auto test(...) -> std::false_type;
    using type = decltype(test<FmtCtx>(std::declval<FmtCtx&>(), std::declval<const Ty&>()));
};
template<typename Ty, typename ParseCtx>
struct has_format_parser {
    template<typename U>
    static auto test(U& ctx)
        -> always_true<decltype(ctx.advance_to(std::declval<formatter<Ty, typename U::char_type>&>().parse(ctx)))>;
    template<typename U>
    static auto test(...) -> std::false_type;
    using type = decltype(test<ParseCtx>(std::declval<ParseCtx&>()));
};
}  // namespace detail

template<typename Ty, typename FmtCtx = format_context>
struct formattable : std::bool_constant<detail::has_formatter<Ty, FmtCtx>::type::value &&
                                        detail::has_format_parser<Ty, typename FmtCtx::parse_context>::type::value> {};

template<typename CharT>
struct formatter<bool, CharT> {
    fmt_opts specs;
    std::size_t width_arg_id = dynamic_extent;
    template<typename ParseCtx>
    UXS_CONSTEXPR typename ParseCtx::iterator parse(ParseCtx& ctx) {
        auto it = ctx.begin();
        if (it == ctx.end() || *it != ':') { return it; }
        std::size_t dummy_id = dynamic_extent;
        it = ParseCtx::parse_standard(ctx, it + 1, specs, width_arg_id, dummy_id);
        auto type = it != ctx.end() && *it != '}' ? ParseCtx::classify_standard_type(*it, specs) :
                                                    ParseCtx::type_spec::none;
        if (specs.prec >= 0) { ParseCtx::unexpected_prec_error(); }
        if (type == ParseCtx::type_spec::none || type == ParseCtx::type_spec::string) {
            if (!!(specs.flags & fmt_flags::sign_field)) { ParseCtx::unexpected_sign_error(); }
            if (!!(specs.flags & fmt_flags::leading_zeroes)) { ParseCtx::unexpected_leading_zeroes_error(); }
        } else if (type != ParseCtx::type_spec::integer) {
            ParseCtx::type_error();
        }
        return type == ParseCtx::type_spec::none ? it : it + 1;
    }
    template<typename FmtCtx>
    void format(FmtCtx& ctx, bool val) const {
        fmt_opts fmt = specs;
        if (width_arg_id != dynamic_extent) {
            fmt.width = ctx.arg(width_arg_id).get_unsigned(std::numeric_limits<decltype(fmt.width)>::max());
        }
        scvt::fmt_boolean(ctx.out(), val, fmt, ctx.locale());
    }
};

template<typename CharT>
struct formatter<CharT, CharT> {
    fmt_opts specs;
    std::size_t width_arg_id = dynamic_extent;
    template<typename ParseCtx>
    UXS_CONSTEXPR typename ParseCtx::iterator parse(ParseCtx& ctx) {
        auto it = ctx.begin();
        if (it == ctx.end() || *it != ':') { return it; }
        std::size_t dummy_id = dynamic_extent;
        it = ParseCtx::parse_standard(ctx, it + 1, specs, width_arg_id, dummy_id);
        auto type = it != ctx.end() && *it != '}' ? ParseCtx::classify_standard_type(*it, specs) :
                                                    ParseCtx::type_spec::none;
        if (specs.prec >= 0) { ParseCtx::unexpected_prec_error(); }
        if (type == ParseCtx::type_spec::none || type == ParseCtx::type_spec::character) {
            if (!!(specs.flags & fmt_flags::sign_field)) { ParseCtx::unexpected_sign_error(); }
            if (!!(specs.flags & fmt_flags::leading_zeroes)) { ParseCtx::unexpected_leading_zeroes_error(); }
        } else if (type != ParseCtx::type_spec::integer) {
            ParseCtx::type_error();
        }
        return type == ParseCtx::type_spec::none ? it : it + 1;
    }
    template<typename FmtCtx>
    void format(FmtCtx& ctx, CharT val) const {
        fmt_opts fmt = specs;
        if (width_arg_id != dynamic_extent) {
            fmt.width = ctx.arg(width_arg_id).get_unsigned(std::numeric_limits<decltype(fmt.width)>::max());
        }
        scvt::fmt_character(ctx.out(), val, fmt, ctx.locale());
    }
};

#define UXS_FMT_IMPLEMENT_STANDARD_FORMATTER(ty) \
    template<typename CharT> \
    struct formatter<ty, CharT> { \
        fmt_opts specs; \
        std::size_t width_arg_id = dynamic_extent; \
        template<typename ParseCtx> \
        UXS_CONSTEXPR typename ParseCtx::iterator parse(ParseCtx& ctx) { \
            auto it = ctx.begin(); \
            if (it == ctx.end() || *it != ':') { return it; } \
            std::size_t dummy_id = dynamic_extent; \
            it = ParseCtx::parse_standard(ctx, it + 1, specs, width_arg_id, dummy_id); \
            auto type = it != ctx.end() && *it != '}' ? ParseCtx::classify_standard_type(*it, specs) : \
                                                        ParseCtx::type_spec::none; \
            if (specs.prec >= 0) { ParseCtx::unexpected_prec_error(); } \
            if (type == ParseCtx::type_spec::character) { \
                if (!!(specs.flags & fmt_flags::sign_field)) { ParseCtx::unexpected_sign_error(); } \
                if (!!(specs.flags & fmt_flags::leading_zeroes)) { ParseCtx::unexpected_leading_zeroes_error(); } \
                specs.flags |= fmt_flags::character; \
            } else if (type != ParseCtx::type_spec::none && type != ParseCtx::type_spec::integer) { \
                ParseCtx::type_error(); \
            } \
            return type == ParseCtx::type_spec::none ? it : it + 1; \
        } \
        template<typename FmtCtx> \
        void format(FmtCtx& ctx, ty val) const { \
            fmt_opts fmt = specs; \
            if (width_arg_id != dynamic_extent) { \
                fmt.width = ctx.arg(width_arg_id).get_unsigned(std::numeric_limits<decltype(fmt.width)>::max()); \
            } \
            scvt::fmt_integer(ctx.out(), val, fmt, ctx.locale()); \
        } \
    };
UXS_FMT_IMPLEMENT_STANDARD_FORMATTER(std::int32_t)
UXS_FMT_IMPLEMENT_STANDARD_FORMATTER(std::int64_t)
UXS_FMT_IMPLEMENT_STANDARD_FORMATTER(std::uint32_t)
UXS_FMT_IMPLEMENT_STANDARD_FORMATTER(std::uint64_t)
#undef UXS_FMT_IMPLEMENT_STANDARD_FORMATTER

#define UXS_FMT_IMPLEMENT_STANDARD_FORMATTER(ty) \
    template<typename CharT> \
    struct formatter<ty, CharT> { \
        fmt_opts specs; \
        std::size_t width_arg_id = dynamic_extent; \
        std::size_t prec_arg_id = dynamic_extent; \
        template<typename ParseCtx> \
        UXS_CONSTEXPR typename ParseCtx::iterator parse(ParseCtx& ctx) { \
            auto it = ctx.begin(); \
            if (it == ctx.end() || *it != ':') { return it; } \
            it = ParseCtx::parse_standard(ctx, it + 1, specs, width_arg_id, prec_arg_id); \
            auto type = it != ctx.end() && *it != '}' ? ParseCtx::classify_standard_type(*it, specs) : \
                                                        ParseCtx::type_spec::none; \
            if (type != ParseCtx::type_spec::none && type != ParseCtx::type_spec::floating_point) { \
                ParseCtx::type_error(); \
            } \
            return type == ParseCtx::type_spec::none ? it : it + 1; \
        } \
        template<typename FmtCtx> \
        void format(FmtCtx& ctx, ty val) const { \
            fmt_opts fmt = specs; \
            if (width_arg_id != dynamic_extent) { \
                fmt.width = ctx.arg(width_arg_id).get_unsigned(std::numeric_limits<decltype(fmt.width)>::max()); \
            } \
            if (prec_arg_id != dynamic_extent) { \
                fmt.prec = ctx.arg(prec_arg_id).get_unsigned(std::numeric_limits<decltype(fmt.prec)>::max()); \
            } \
            scvt::fmt_float(ctx.out(), val, fmt, ctx.locale()); \
        } \
    };
UXS_FMT_IMPLEMENT_STANDARD_FORMATTER(float)
UXS_FMT_IMPLEMENT_STANDARD_FORMATTER(double)
UXS_FMT_IMPLEMENT_STANDARD_FORMATTER(long double)
#undef UXS_FMT_IMPLEMENT_STANDARD_FORMATTER

template<typename CharT>
struct formatter<const void*, CharT> {
    fmt_opts specs;
    std::size_t width_arg_id = dynamic_extent;
    template<typename ParseCtx>
    UXS_CONSTEXPR typename ParseCtx::iterator parse(ParseCtx& ctx) {
        auto it = ctx.begin();
        if (it == ctx.end() || *it != ':') { return it; }
        std::size_t dummy_id = dynamic_extent;
        it = ParseCtx::parse_standard(ctx, it + 1, specs, width_arg_id, dummy_id);
        auto type = it != ctx.end() && *it != '}' ? ParseCtx::classify_standard_type(*it, specs) :
                                                    ParseCtx::type_spec::none;
        if (specs.prec >= 0) { ParseCtx::unexpected_prec_error(); }
        if (!!(specs.flags & fmt_flags::sign_field)) { ParseCtx::unexpected_sign_error(); }
        if (type != ParseCtx::type_spec::none && type != ParseCtx::type_spec::pointer) { ParseCtx::type_error(); }
        return type == ParseCtx::type_spec::none ? it : it + 1;
    }
    template<typename FmtCtx>
    void format(FmtCtx& ctx, const void* val) const {
        fmt_opts fmt = specs;
        if (width_arg_id != dynamic_extent) {
            fmt.width = ctx.arg(width_arg_id).get_unsigned(std::numeric_limits<decltype(fmt.width)>::max());
        }
        fmt.flags |= fmt_flags::hex | fmt_flags::alternate;
        scvt::fmt_integer(ctx.out(), reinterpret_cast<std::uintptr_t>(val), fmt, ctx.locale());
    }
};

#define UXS_FMT_IMPLEMENT_STANDARD_FORMATTER(ty) \
    template<typename CharT> \
    struct formatter<ty, CharT> { \
        fmt_opts specs; \
        std::size_t width_arg_id = dynamic_extent; \
        std::size_t prec_arg_id = dynamic_extent; \
        template<typename ParseCtx> \
        UXS_CONSTEXPR typename ParseCtx::iterator parse(ParseCtx& ctx) { \
            auto it = ctx.begin(); \
            if (it == ctx.end() || *it != ':') { return it; } \
            it = ParseCtx::parse_standard(ctx, it + 1, specs, width_arg_id, prec_arg_id); \
            auto type = it != ctx.end() && *it != '}' ? ParseCtx::classify_standard_type(*it, specs) : \
                                                        ParseCtx::type_spec::none; \
            if (!!(specs.flags & fmt_flags::sign_field)) { ParseCtx::unexpected_sign_error(); } \
            if (!!(specs.flags & fmt_flags::leading_zeroes)) { ParseCtx::unexpected_leading_zeroes_error(); } \
            if (type != ParseCtx::type_spec::none && type != ParseCtx::type_spec::string) { ParseCtx::type_error(); } \
            return type == ParseCtx::type_spec::none ? it : it + 1; \
        } \
        template<typename FmtCtx> \
        void format(FmtCtx& ctx, ty val) const { \
            fmt_opts fmt = specs; \
            if (width_arg_id != dynamic_extent) { \
                fmt.width = ctx.arg(width_arg_id).get_unsigned(std::numeric_limits<decltype(fmt.width)>::max()); \
            } \
            if (prec_arg_id != dynamic_extent) { \
                fmt.prec = ctx.arg(prec_arg_id).get_unsigned(std::numeric_limits<decltype(fmt.prec)>::max()); \
            } \
            scvt::fmt_string<CharT>(ctx.out(), val, fmt, ctx.locale()); \
        } \
    };
UXS_FMT_IMPLEMENT_STANDARD_FORMATTER(const CharT*)
UXS_FMT_IMPLEMENT_STANDARD_FORMATTER(std::basic_string_view<CharT>)
#undef UXS_FMT_IMPLEMENT_STANDARD_FORMATTER

namespace sfmt {

enum class type_index : std::uint8_t {
    boolean = 0,
    character,
    integer,
    long_integer,
    unsigned_integer,
    unsigned_long_integer,
    single_precision,
    double_precision,
    long_double_precision,
    pointer,
    z_string,
    string,
    custom
};

// --------------------------

namespace detail {
template<typename Ty, typename CharT, typename = void>
struct reduce_type {
    using type = std::remove_cv_t<Ty>;
};
template<typename Ty, typename CharT>
struct reduce_type<Ty, CharT,
                   std::enable_if_t<std::is_integral<Ty>::value && std::is_unsigned<Ty>::value &&
                                    !is_boolean<Ty>::value && !is_character<Ty>::value>> {
    using type = std::conditional_t<(sizeof(Ty) <= sizeof(std::uint32_t)), std::uint32_t, std::uint64_t>;
};
template<typename Ty, typename CharT>
struct reduce_type<Ty, CharT,
                   std::enable_if_t<std::is_integral<Ty>::value && std::is_signed<Ty>::value &&
                                    !is_boolean<Ty>::value && !is_character<Ty>::value>> {
    using type = std::conditional_t<(sizeof(Ty) <= sizeof(std::int32_t)), std::int32_t, std::int64_t>;
};
template<typename Ty, typename CharT>
struct reduce_type<Ty, CharT, std::enable_if_t<is_character<Ty>::value>> {
    using type = CharT;
};
template<typename CharT, typename Traits>
struct reduce_type<std::basic_string_view<CharT, Traits>, CharT> {
    using type = std::basic_string_view<CharT>;
};
template<typename CharT, typename Traits, typename Alloc>
struct reduce_type<std::basic_string<CharT, Traits, Alloc>, CharT> {
    using type = std::basic_string_view<CharT>;
};
template<typename Ty, typename CharT>
struct reduce_type<Ty, CharT, std::enable_if_t<std::is_array<Ty>::value>> {
    using type =
        std::conditional_t<is_character<typename std::remove_extent<Ty>::type>::value, const CharT*, const void*>;
};
template<typename Ty, typename CharT>
struct reduce_type<Ty*, CharT> {
    using type = std::conditional_t<is_character<Ty>::value, const CharT*, const void*>;
};
template<typename CharT>
struct reduce_type<std::nullptr_t, CharT> {
    using type = const void*;
};
}  // namespace detail

template<typename Ty, typename CharT>
using reduce_type_t = typename detail::reduce_type<Ty, CharT>::type;

// --------------------------

namespace detail {
template<typename Ty, typename CharT>
struct arg_type_index;
#define UXS_FMT_DECLARE_ARG_TYPE_INDEX(ty, index) \
    template<typename CharT> \
    struct arg_type_index<ty, CharT> : std::integral_constant<type_index, index> {};
UXS_FMT_DECLARE_ARG_TYPE_INDEX(bool, type_index::boolean)
UXS_FMT_DECLARE_ARG_TYPE_INDEX(CharT, type_index::character)
UXS_FMT_DECLARE_ARG_TYPE_INDEX(std::int32_t, type_index::integer)
UXS_FMT_DECLARE_ARG_TYPE_INDEX(std::int64_t, type_index::long_integer)
UXS_FMT_DECLARE_ARG_TYPE_INDEX(std::uint32_t, type_index::unsigned_integer)
UXS_FMT_DECLARE_ARG_TYPE_INDEX(std::uint64_t, type_index::unsigned_long_integer)
UXS_FMT_DECLARE_ARG_TYPE_INDEX(float, type_index::single_precision)
UXS_FMT_DECLARE_ARG_TYPE_INDEX(double, type_index::double_precision)
UXS_FMT_DECLARE_ARG_TYPE_INDEX(long double, type_index::long_double_precision)
UXS_FMT_DECLARE_ARG_TYPE_INDEX(const void*, type_index::pointer)
UXS_FMT_DECLARE_ARG_TYPE_INDEX(const CharT*, type_index::z_string)
UXS_FMT_DECLARE_ARG_TYPE_INDEX(std::basic_string_view<CharT>, type_index::string)
#undef UXS_FMT_DECLARE_ARG_TYPE_INDEX
}  // namespace detail

// --------------------------

template<typename FmtCtx>
class custom_arg_handle {
 public:
    using format_func_type = void (*)(FmtCtx&, typename FmtCtx::parse_context&, const void*);

    template<typename Ty>
    UXS_CONSTEXPR custom_arg_handle(const Ty& v) noexcept : val_(&v), print_fn_(func<Ty>) {}

    void format(FmtCtx& ctx, typename FmtCtx::parse_context& parse_ctx) const { print_fn_(ctx, parse_ctx, val_); }

 private:
    const void* val_;            // value pointer
    format_func_type print_fn_;  // printing function pointer

    template<typename Ty>
    static void func(FmtCtx& ctx, typename FmtCtx::parse_context& parse_ctx, const void* val) {
        ctx.format_arg(parse_ctx, *static_cast<const Ty*>(val));
    }
};

// --------------------------

template<typename Ty, typename CharT, typename = void>
struct arg_type_index : std::integral_constant<type_index, type_index::custom> {};
template<typename Ty, typename CharT>
struct arg_type_index<Ty, CharT, std::void_t<typename detail::arg_type_index<reduce_type_t<Ty, CharT>, CharT>::type>>
    : std::integral_constant<type_index, detail::arg_type_index<reduce_type_t<Ty, CharT>, CharT>::value> {};

template<typename FmtCtx, typename Ty>
using arg_store_type = std::conditional<(arg_type_index<Ty, typename FmtCtx::char_type>::value) < type_index::custom,
                                        reduce_type_t<Ty, typename FmtCtx::char_type>, custom_arg_handle<FmtCtx>>;

template<typename FmtCtx, typename Ty>
using arg_size = uxs::size_of<typename arg_store_type<FmtCtx, Ty>::type>;

template<typename FmtCtx, typename Ty>
using arg_alignment = std::alignment_of<typename arg_store_type<FmtCtx, Ty>::type>;

template<typename FmtCtx, std::size_t, typename...>
struct arg_store_size_evaluator;
template<typename FmtCtx, std::size_t Size>
struct arg_store_size_evaluator<FmtCtx, Size> : std::integral_constant<std::size_t, Size> {};
template<typename FmtCtx, std::size_t Size, typename Ty, typename... Rest>
struct arg_store_size_evaluator<FmtCtx, Size, Ty, Rest...>
    : std::integral_constant<
          std::size_t,
          arg_store_size_evaluator<FmtCtx,
                                   uxs::align_up<arg_alignment<FmtCtx, Ty>::value>::template type<Size>::value +
                                       arg_size<FmtCtx, Ty>::value,
                                   Rest...>::value> {};

template<typename FmtCtx, typename...>
struct arg_store_alignment_evaluator;
template<typename FmtCtx, typename Ty>
struct arg_store_alignment_evaluator<FmtCtx, Ty> : arg_alignment<FmtCtx, Ty> {};
template<typename FmtCtx, typename Ty1, typename Ty2, typename... Rest>
struct arg_store_alignment_evaluator<FmtCtx, Ty1, Ty2, Rest...>
    : std::conditional<(arg_alignment<FmtCtx, Ty1>::value > arg_alignment<FmtCtx, Ty2>::value),
                       arg_store_alignment_evaluator<FmtCtx, Ty1, Rest...>,
                       arg_store_alignment_evaluator<FmtCtx, Ty2, Rest...>>::type {};

template<typename FmtCtx, typename... Args>
class arg_store {
 public:
    using char_type = typename FmtCtx::char_type;
    static const std::size_t arg_count = sizeof...(Args);

    arg_store& operator=(const arg_store&) = delete;

    UXS_CONSTEXPR explicit arg_store(const Args&... args) noexcept {
        store_values(0, arg_count * sizeof(unsigned), args...);
    }
    UXS_CONSTEXPR const void* data() const noexcept { return data_; }

 private:
    static const std::size_t storage_size =
        arg_store_size_evaluator<FmtCtx, arg_count * sizeof(unsigned), Args...>::value;
    static const std::size_t storage_alignment = arg_store_alignment_evaluator<FmtCtx, unsigned, Args...>::value;
    alignas(storage_alignment) std::uint8_t data_[storage_size];

    template<typename Ty>
    UXS_CONSTEXPR static void store_value(
        const Ty& v,
        std::enable_if_t<(arg_type_index<Ty, char_type>::value < type_index::pointer), void*> data) noexcept {
        static_assert(!is_character<Ty>::value || sizeof(Ty) <= sizeof(char_type),
                      "inconsistent character argument type");
        ::new (data) reduce_type_t<Ty, char_type>(v);
    }

    template<typename Ty>
    UXS_CONSTEXPR static void store_value(
        const Ty& v,
        std::enable_if_t<(arg_type_index<Ty, char_type>::value == type_index::custom), void*> data) noexcept {
        using ReducedTy = reduce_type_t<Ty, char_type>;
        static_assert(formattable<ReducedTy, FmtCtx>::value, "value of this type cannot be formatted");
        ::new (data) custom_arg_handle<FmtCtx>(static_cast<const ReducedTy&>(v));
    }

    template<typename Ty>
    UXS_CONSTEXPR static void store_value(Ty* v, void* data) noexcept {
        static_assert(!is_character<Ty>::value || std::is_same<std::remove_cv_t<Ty>, char_type>::value,
                      "inconsistent string argument type");
        ::new (data) const void*(v);
    }

    UXS_CONSTEXPR static void store_value(std::nullptr_t, void* data) noexcept { ::new (data) const void*(nullptr); }

    template<typename CharT, typename Traits>
    UXS_CONSTEXPR static void store_value(const std::basic_string_view<CharT, Traits>& s, void* data) noexcept {
        using Ty = std::basic_string_view<char_type>;
        static_assert(std::is_same<CharT, char_type>::value, "inconsistent string argument type");
        static_assert(std::is_trivially_copyable<Ty>::value && std::is_trivially_destructible<Ty>::value,
                      "std::basic_string_view<> is assumed to be trivially copyable and destructible");
        ::new (data) Ty(s.data(), s.size());
    }

    template<typename CharT, typename Traits, typename Alloc>
    UXS_CONSTEXPR static void store_value(const std::basic_string<CharT, Traits, Alloc>& s, void* data) noexcept {
        using Ty = std::basic_string_view<char_type>;
        static_assert(std::is_same<CharT, char_type>::value, "inconsistent string argument type");
        static_assert(std::is_trivially_copyable<Ty>::value && std::is_trivially_destructible<Ty>::value,
                      "std::basic_string_view<> is assumed to be trivially copyable and destructible");
        ::new (data) Ty(s.data(), s.size());
    }

    UXS_CONSTEXPR void store_values(std::size_t i, std::size_t offset) noexcept {}

    template<typename Ty, typename... Ts>
    UXS_CONSTEXPR void store_values(std::size_t i, std::size_t offset, const Ty& v, const Ts&... other) noexcept {
        offset = uxs::align_up<arg_alignment<FmtCtx, Ty>::value>::value(offset);
        ::new (reinterpret_cast<unsigned*>(&data_) + i) unsigned(
            static_cast<unsigned>(offset << 8) | static_cast<unsigned>(arg_type_index<Ty, char_type>::value));
        store_value(v, &data_[offset]);
        store_values(i + 1, offset + arg_size<FmtCtx, Ty>::value, other...);
    }
};

template<typename FmtCtx>
class arg_store<FmtCtx> {
 public:
    using char_type = typename FmtCtx::char_type;
    static const std::size_t arg_count = 0;
    arg_store& operator=(const arg_store&) = delete;
    UXS_CONSTEXPR arg_store() noexcept = default;
    UXS_CONSTEXPR const void* data() const noexcept { return nullptr; }
};

// --------------------------

struct parse_context_utils {
    template<typename Iter, typename Ty>
    static UXS_CONSTEXPR Iter parse_number(Iter first, Iter last, Ty& num) {
        for (unsigned dig = 0; first != last && (dig = dig_v(*first)) < 10; ++first) {
            Ty num0 = num;
            num = 10 * num + dig;
            if (num < num0) { throw format_error("integer overflow"); }
        }
        return first;
    }

    template<typename ParseCtx>
    static UXS_CONSTEXPR typename ParseCtx::iterator parse_standard(ParseCtx& ctx, typename ParseCtx::iterator it,
                                                                    fmt_opts& specs, std::size_t& width_arg_id,
                                                                    std::size_t& prec_arg_id) {
        if (it == ctx.end()) { return it; }

        enum class state_t { adjustment = 0, sign, alternate, leading_zeroes, width, precision, locale, finish };

        state_t state = state_t::adjustment;

        if (it + 1 != ctx.end()) {
            switch (*(it + 1)) {  // adjustment with fill character
                case '<': {
                    specs.fill = *it, specs.flags |= fmt_flags::left;
                    it += 2, state = state_t::sign;
                } break;
                case '^': {
                    specs.fill = *it, specs.flags |= fmt_flags::internal;
                    it += 2, state = state_t::sign;
                } break;
                case '>': {
                    specs.fill = *it, specs.flags |= fmt_flags::right;
                    it += 2, state = state_t::sign;
                } break;
                default: break;
            }
        }

        for (; it != ctx.end(); ++it) {
            switch (*it) {
#define UXS_FMT_SPECIFIER_CASE(next_state, action) \
    if (state < next_state) { \
        state = next_state; \
        action; \
        break; \
    } \
    return it;
                // adjustment
                case '<': UXS_FMT_SPECIFIER_CASE(state_t::sign, { specs.flags |= fmt_flags::left; });
                case '^': UXS_FMT_SPECIFIER_CASE(state_t::sign, { specs.flags |= fmt_flags::internal; });
                case '>': UXS_FMT_SPECIFIER_CASE(state_t::sign, { specs.flags |= fmt_flags::right; });

                // sign specifiers
                case '-': UXS_FMT_SPECIFIER_CASE(state_t::alternate, { specs.flags |= fmt_flags::sign_neg; });
                case '+': UXS_FMT_SPECIFIER_CASE(state_t::alternate, { specs.flags |= fmt_flags::sign_pos; });
                case ' ': UXS_FMT_SPECIFIER_CASE(state_t::alternate, { specs.flags |= fmt_flags::sign_align; });

                // alternate
                case '#': UXS_FMT_SPECIFIER_CASE(state_t::leading_zeroes, { specs.flags |= fmt_flags::alternate; });

                // leading zeroes
                case '0': UXS_FMT_SPECIFIER_CASE(state_t::width, { specs.flags |= fmt_flags::leading_zeroes; });

                // locale
                case 'L': UXS_FMT_SPECIFIER_CASE(state_t::finish, { specs.flags |= fmt_flags::localize; });

                // width
                case '{':
                    UXS_FMT_SPECIFIER_CASE(state_t::precision, {
                        if (++it == ctx.end()) { ParseCtx::syntax_error(); }
                        specs.width = 1;  // width is specified
                        // obtain argument number for width
                        if (*it == '}') {
                            width_arg_id = ctx.next_arg_id();
                        } else if ((width_arg_id = dig_v(*it)) < 10) {
                            it = ParseCtx::parse_number(++it, ctx.end(), width_arg_id);
                            ctx.check_arg_id(width_arg_id);
                            if (it == ctx.end() || *it != '}') { ParseCtx::syntax_error(); }
                        } else {
                            ParseCtx::syntax_error();
                        }
                        ctx.check_arg_integral(width_arg_id);
                        break;
                    });
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    UXS_FMT_SPECIFIER_CASE(state_t::precision, {
                        specs.width = static_cast<unsigned>(*it - '0');
                        it = ParseCtx::parse_number(++it, ctx.end(), specs.width) - 1;
                    });

                // precision
                case '.':
                    UXS_FMT_SPECIFIER_CASE(state_t::locale, {
                        if (++it == ctx.end()) { syntax_error(); }
                        unsigned dig = dig_v(*it);
                        if (dig < 10) {
                            specs.prec = dig;
                            it = ParseCtx::parse_number(++it, ctx.end(), specs.prec) - 1;
                        } else if (*it == '{' && ++it != ctx.end()) {
                            specs.prec = 0;  // precision is specified
                            // obtain argument number for precision
                            if (*it == '}') {
                                prec_arg_id = ctx.next_arg_id();
                            } else if ((prec_arg_id = dig_v(*it)) < 10) {
                                it = ParseCtx::parse_number(++it, ctx.end(), prec_arg_id);
                                ctx.check_arg_id(prec_arg_id);
                                if (it == ctx.end() || *it != '}') { ParseCtx::syntax_error(); }
                            } else {
                                ParseCtx::syntax_error();
                            }
                            ctx.check_arg_integral(prec_arg_id);
                        } else {
                            ParseCtx::syntax_error();
                        }
                        break;
                    });
#undef UXS_FMT_SPECIFIER_CASE

                default: return it;
            }
        }

        return it;
    }

    enum class type_spec : unsigned { none = 0, integer, floating_point, pointer, character, string };

    template<typename CharT>
    static UXS_CONSTEXPR type_spec classify_standard_type(CharT ch, fmt_opts& specs) {
        switch (ch) {
            case 'd': specs.flags |= fmt_flags::dec; return type_spec::integer;
            case 'B': specs.flags |= fmt_flags::bin | fmt_flags::uppercase; return type_spec::integer;
            case 'b': specs.flags |= fmt_flags::bin; return type_spec::integer;
            case 'o': specs.flags |= fmt_flags::oct; return type_spec::integer;
            case 'X': specs.flags |= fmt_flags::hex | fmt_flags::uppercase; return type_spec::integer;
            case 'x': specs.flags |= fmt_flags::hex; return type_spec::integer;
            case 'F': specs.flags |= fmt_flags::fixed | fmt_flags::uppercase; return type_spec::floating_point;
            case 'f': specs.flags |= fmt_flags::fixed; return type_spec::floating_point;
            case 'E': specs.flags |= fmt_flags::scientific | fmt_flags::uppercase; return type_spec::floating_point;
            case 'e': specs.flags |= fmt_flags::scientific; return type_spec::floating_point;
            case 'G': specs.flags |= fmt_flags::general | fmt_flags::uppercase; return type_spec::floating_point;
            case 'g': specs.flags |= fmt_flags::general; return type_spec::floating_point;
            case 'A': specs.flags |= fmt_flags::hex | fmt_flags::uppercase; return type_spec::floating_point;
            case 'a': specs.flags |= fmt_flags::hex; return type_spec::floating_point;
            case 'P': specs.flags |= fmt_flags::uppercase; return type_spec::pointer;
            case 'p': return type_spec::pointer;
            case 'c': return type_spec::character;
            case 's': return type_spec::string;
            default: return type_spec::none;
        }
    }

    template<typename ParseCtx, typename Ty>
    static UXS_CONSTEXPR void parse_arg(ParseCtx& ctx) {
        formatter<Ty, typename ParseCtx::char_type> f;
        ctx.advance_to(f.parse(ctx));
    }

    static void syntax_error() { throw format_error("invalid specifier syntax"); }
    static void unexpected_prec_error() { throw format_error("unexpected precision specifier"); };
    static void unexpected_sign_error() { throw format_error("unexpected sign specifier"); };
    static void unexpected_leading_zeroes_error() { throw format_error("unexpected leading zeroes specifier"); };
    static void type_error() { throw format_error("invalid type specifier"); };
};

template<typename ParseCtx, typename AppendTextFn, typename AppendArgFn>
UXS_CONSTEXPR void parse_format(ParseCtx& ctx, const AppendTextFn& append_text_fn, const AppendArgFn& append_arg_fn) {
    auto it0 = ctx.begin();
    for (auto it = it0; it != ctx.end(); ++it) {
        if (*it == '{' || *it == '}') {
            append_text_fn(it0, it++);
            it0 = it;
            if (it != ctx.end() && *(it - 1) == '{' && *it != '{') {
                std::size_t arg_id = 0;
                if ((arg_id = dig_v(*it)) < 10) {
                    it = ParseCtx::parse_number(++it, ctx.end(), arg_id);
                    ctx.check_arg_id(arg_id);
                } else {
                    arg_id = ctx.next_arg_id();
                }
                ctx.advance_to(it);
                append_arg_fn(ctx, arg_id);
                if ((it = ctx.begin()) == ctx.end() || *it != '}') { ParseCtx::syntax_error(); }
                it0 = it + 1;
            } else if (it == ctx.end() || *(it - 1) != *it) {
                ParseCtx::syntax_error();
            }
        }
    }
    append_text_fn(it0, ctx.end());
}

template<typename FmtCtx>
UXS_EXPORT void vformat(FmtCtx, typename FmtCtx::parse_context);

}  // namespace sfmt

template<typename FmtCtx, typename Ty>
struct format_arg_type_index : sfmt::detail::arg_type_index<Ty, typename FmtCtx::char_type> {};
template<typename FmtCtx>
struct format_arg_type_index<FmtCtx, sfmt::custom_arg_handle<FmtCtx>>
    : std::integral_constant<sfmt::type_index, sfmt::type_index::custom> {};

template<typename FmtCtx>
class basic_format_arg {
 public:
    using char_type = typename FmtCtx::char_type;
    using handle = sfmt::custom_arg_handle<FmtCtx>;

    basic_format_arg(sfmt::type_index index, const void* data) noexcept : index_(index), data_(data) {}

    sfmt::type_index index() const noexcept { return index_; }

    template<typename Ty>
    const Ty& as() const {
        if (index_ != format_arg_type_index<FmtCtx, Ty>::value) { throw format_error("invalid value type"); }
        return *static_cast<const Ty*>(data_);
    }

    unsigned get_unsigned(unsigned limit) const {
        switch (index_) {
#define UXS_FMT_ARG_SIGNED_INTEGER_VALUE_CASE(ty) \
    case format_arg_type_index<FmtCtx, ty>::value: { \
        ty val = *static_cast<const ty*>(data_); \
        if (val < 0) { throw format_error("negative argument specified"); } \
        if (static_cast<std::make_unsigned<ty>::type>(val) > limit) { throw format_error("too large integer"); } \
        return static_cast<unsigned>(val); \
    } break;
#define UXS_FMT_ARG_UNSIGNED_INTEGER_VALUE_CASE(ty) \
    case format_arg_type_index<FmtCtx, ty>::value: { \
        ty val = *static_cast<const ty*>(data_); \
        if (val > limit) { throw format_error("too large integer"); } \
        return static_cast<unsigned>(val); \
    } break;
            UXS_FMT_ARG_SIGNED_INTEGER_VALUE_CASE(std::int32_t)
            UXS_FMT_ARG_SIGNED_INTEGER_VALUE_CASE(std::int64_t)
            UXS_FMT_ARG_UNSIGNED_INTEGER_VALUE_CASE(std::uint32_t)
            UXS_FMT_ARG_UNSIGNED_INTEGER_VALUE_CASE(std::uint64_t)
#undef UXS_FMT_ARG_SIGNED_INTEGER_VALUE_CASE
#undef UXS_FMT_ARG_UNSIGNED_INTEGER_VALUE_CASE
            default: throw format_error("argument is not an integer");
        }
    }

    template<typename Func>
    void visit(const Func& func) {
        switch (index_) {
#define UXS_FMT_FORMAT_ARG_VALUE(ty) \
    case format_arg_type_index<FmtCtx, ty>::value: { \
        func(*static_cast<ty const*>(data_)); \
    } break;
            UXS_FMT_FORMAT_ARG_VALUE(bool)
            UXS_FMT_FORMAT_ARG_VALUE(char_type)
            UXS_FMT_FORMAT_ARG_VALUE(std::int32_t)
            UXS_FMT_FORMAT_ARG_VALUE(std::int64_t)
            UXS_FMT_FORMAT_ARG_VALUE(std::uint32_t)
            UXS_FMT_FORMAT_ARG_VALUE(std::uint64_t)
            UXS_FMT_FORMAT_ARG_VALUE(float)
            UXS_FMT_FORMAT_ARG_VALUE(double)
            UXS_FMT_FORMAT_ARG_VALUE(long double)
            UXS_FMT_FORMAT_ARG_VALUE(const void*)
            UXS_FMT_FORMAT_ARG_VALUE(const char_type*)
            UXS_FMT_FORMAT_ARG_VALUE(std::basic_string_view<char_type>)
            UXS_FMT_FORMAT_ARG_VALUE(typename basic_format_arg<FmtCtx>::handle)
#undef UXS_FMT_FORMAT_ARG_VALUE
        }
    }

 private:
    sfmt::type_index index_;
    const void* data_;
};

template<typename FmtCtx>
class basic_format_args {
 public:
    template<typename... Args>
    basic_format_args(const sfmt::arg_store<FmtCtx, Args...>& store) noexcept
        : data_(store.data()), size_(sfmt::arg_store<FmtCtx, Args...>::arg_count) {}

    basic_format_arg<FmtCtx> get(std::size_t id) const {
        if (id >= size_) { throw format_error("out of argument list"); }
        const unsigned meta = static_cast<const unsigned*>(data_)[id];
        return basic_format_arg<FmtCtx>(static_cast<sfmt::type_index>(meta & 0xff),
                                        static_cast<const std::uint8_t*>(data_) + (meta >> 8));
    }

 private:
    const void* data_;
    std::size_t size_;
};

template<typename CharT>
class basic_format_parse_context : public sfmt::parse_context_utils {
 public:
    using char_type = CharT;
    using iterator = typename std::basic_string_view<char_type>::const_iterator;
    using const_iterator = typename std::basic_string_view<char_type>::const_iterator;

    UXS_CONSTEXPR explicit basic_format_parse_context(std::basic_string_view<char_type> fmt) noexcept : fmt_(fmt) {}

#if __cplusplus >= 201703L
    basic_format_parse_context(const basic_format_parse_context&) = delete;
#endif  // __cplusplus >= 201703L
    basic_format_parse_context& operator=(const basic_format_parse_context&) = delete;

    UXS_CONSTEXPR iterator begin() const noexcept { return fmt_.begin(); }
    UXS_CONSTEXPR iterator end() const noexcept { return fmt_.end(); }
    UXS_CONSTEXPR void advance_to(iterator it) { fmt_.remove_prefix(static_cast<std::size_t>(it - fmt_.begin())); }

    UXS_NODISCARD UXS_CONSTEXPR std::size_t next_arg_id() {
        if (next_arg_id_ == dynamic_extent) { throw format_error("automatic argument indexing error"); }
        return next_arg_id_++;
    }
    UXS_CONSTEXPR void check_arg_id(std::size_t id) {
        if (next_arg_id_ != dynamic_extent && next_arg_id_ > 0) {
            throw format_error("manual argument indexing error");
        }
        next_arg_id_ = dynamic_extent;
    }

    template<typename... Ts>
    UXS_CONSTEXPR void check_arg_type(std::size_t id) {}
    UXS_CONSTEXPR void check_arg_integral(std::size_t id) const {}

 private:
    std::basic_string_view<char_type> fmt_;
    std::size_t next_arg_id_ = 0;
};

template<typename CharT>
class compile_parse_context : public basic_format_parse_context<CharT> {
 public:
    using char_type = CharT;

    UXS_CONSTEXPR compile_parse_context(std::basic_string_view<char_type> fmt,
                                        uxs::span<const sfmt::type_index> arg_types) noexcept
        : basic_format_parse_context<CharT>(fmt), arg_types_(arg_types) {}

    UXS_NODISCARD UXS_CONSTEXPR std::size_t next_arg_id() {
        std::size_t id = basic_format_parse_context<CharT>::next_arg_id();
        if (id >= arg_types_.size()) { throw format_error("out of argument list"); }
        return id;
    }
    UXS_CONSTEXPR void check_arg_id(std::size_t id) {
        basic_format_parse_context<CharT>::check_arg_id(id);
        if (id >= arg_types_.size()) { throw format_error("out of argument list"); }
    }

    template<typename... Ts>
    UXS_CONSTEXPR void check_arg_type(std::size_t id) {
        const std::array<sfmt::type_index, sizeof...(Ts)> types{sfmt::detail::arg_type_index<Ts, char_type>::value...};
        if (std::find(types.begin(), types.end(), arg_types_[id]) == types.end()) {
            throw format_error("argument is not of valid type");
        }
    }

    UXS_CONSTEXPR void check_arg_integral(std::size_t id) const {
        const auto index = arg_types_[id];
        if (index < sfmt::type_index::integer || index > sfmt::type_index::unsigned_long_integer) {
            throw format_error("argument is not an integer");
        }
    }

 private:
    uxs::span<const sfmt::type_index> arg_types_;
};

template<typename StrTy>
class basic_format_context {
 public:
    using output_type = StrTy;
    using char_type = typename StrTy::value_type;
    template<typename Ty>
    using formatter_type = formatter<Ty, char_type>;
    using parse_context = basic_format_parse_context<char_type>;
    using format_args_type = basic_format_args<basic_format_context>;
    using format_arg_type = basic_format_arg<basic_format_context>;

    basic_format_context(StrTy& s, format_args_type args) : s_(s), args_(args) {}
    basic_format_context(StrTy& s, const std::locale& loc, format_args_type args) : s_(s), loc_(loc), args_(args) {}
    basic_format_context& operator=(const basic_format_context&) = delete;
    StrTy& out() { return s_; }
    locale_ref locale() const { return loc_; }
    format_arg_type arg(std::size_t id) const { return args_.get(id); }

    template<typename Ty>
    void format_arg(parse_context& parse_ctx, const Ty& val) {
        formatter_type<Ty> f;
        parse_ctx.advance_to(f.parse(parse_ctx));
        f.format(*this, val);
    }

    void format_arg(parse_context& parse_ctx, const typename format_arg_type::handle& h) { h.format(*this, parse_ctx); }

 private:
    StrTy& s_;
    locale_ref loc_;
    basic_format_args<basic_format_context> args_;
};

using format_context = basic_format_context<membuffer>;
using wformat_context = basic_format_context<wmembuffer>;
using format_args = basic_format_args<format_context>;
using wformat_args = basic_format_args<wformat_context>;

template<typename FmtCtx = format_context, typename... Args>
UXS_CONSTEXPR sfmt::arg_store<FmtCtx, Args...> make_format_args(const Args&... args) noexcept {
    return sfmt::arg_store<FmtCtx, Args...>{args...};
}

template<typename... Args>
UXS_CONSTEXPR sfmt::arg_store<wformat_context, Args...> make_wformat_args(const Args&... args) noexcept {
    return sfmt::arg_store<wformat_context, Args...>{args...};
}

template<typename CharT, typename... Args>
class basic_format_string {
 public:
    using char_type = CharT;
    template<typename Ty,
             typename = std::enable_if_t<std::is_convertible<const Ty&, std::basic_string_view<char_type>>::value>>
    UXS_CONSTEVAL basic_format_string(const Ty& fmt) noexcept : fmt_(fmt) {
#if defined(UXS_HAS_CONSTEVAL)
        using parse_context = compile_parse_context<char_type>;
        constexpr std::array<sfmt::type_index, sizeof...(Args)> arg_types{
            sfmt::arg_type_index<Args, char_type>::value...};
        constexpr std::array<void (*)(parse_context&), sizeof...(Args)> parsers{
            sfmt::parse_context_utils::parse_arg<parse_context, sfmt::reduce_type_t<Args, char_type>>...};
        parse_context ctx{fmt_, arg_types};
        sfmt::parse_format(
            ctx, [](auto&&...) {}, [&parsers](auto& ctx, std::size_t id) { parsers[id](ctx); });
#endif  // defined(UXS_HAS_CONSTEVAL)
    }
    UXS_CONSTEXPR std::basic_string_view<char_type> get() const noexcept { return fmt_; }

 private:
    std::basic_string_view<char_type> fmt_;
};

template<typename... Args>
using format_string = basic_format_string<char, type_identity_t<Args>...>;
template<typename... Args>
using wformat_string = basic_format_string<wchar_t, type_identity_t<Args>...>;

// ---- basic_vformat

template<typename StrTy>
StrTy& basic_vformat(StrTy& s, std::basic_string_view<typename StrTy::value_type> fmt,
                     basic_format_args<basic_format_context<StrTy>> args) {
    sfmt::vformat(basic_format_context<StrTy>{s, args}, basic_format_parse_context<typename StrTy::value_type>{fmt});
    return s;
}

template<typename StrTy>
StrTy& basic_vformat(StrTy& s, const std::locale& loc, std::basic_string_view<typename StrTy::value_type> fmt,
                     basic_format_args<basic_format_context<StrTy>> args) {
    sfmt::vformat(basic_format_context<StrTy>{s, loc, args},
                  basic_format_parse_context<typename StrTy::value_type>{fmt});
    return s;
}

// ---- basic_format

template<typename StrTy, typename... Args>
StrTy& basic_format(StrTy& s, basic_format_string<typename StrTy::value_type, Args...> fmt, const Args&... args) {
    return basic_vformat(s, fmt.get(), make_format_args<basic_format_context<StrTy>>(args...));
}

template<typename StrTy, typename... Args>
StrTy& basic_format(StrTy& s, const std::locale& loc, basic_format_string<typename StrTy::value_type, Args...> fmt,
                    const Args&... args) {
    return basic_vformat(s, loc, fmt.get(), make_format_args<basic_format_context<StrTy>>(args...));
}

// ---- vformat

inline std::string vformat(std::string_view fmt, format_args args) {
    inline_dynbuffer buf;
    basic_vformat<membuffer>(buf, fmt, args);
    return std::string(buf.data(), buf.size());
}

inline std::wstring vformat(std::wstring_view fmt, wformat_args args) {
    inline_wdynbuffer buf;
    basic_vformat<wmembuffer>(buf, fmt, args);
    return std::wstring(buf.data(), buf.size());
}

inline std::string vformat(const std::locale& loc, std::string_view fmt, format_args args) {
    inline_dynbuffer buf;
    basic_vformat<membuffer>(buf, loc, fmt, args);
    return std::string(buf.data(), buf.size());
}

inline std::wstring vformat(const std::locale& loc, std::wstring_view fmt, wformat_args args) {
    inline_wdynbuffer buf;
    basic_vformat<wmembuffer>(buf, loc, fmt, args);
    return std::wstring(buf.data(), buf.size());
}

// ---- format

template<typename... Args>
std::string format(format_string<Args...> fmt, const Args&... args) {
    return vformat(fmt.get(), make_format_args(args...));
}

template<typename... Args>
std::wstring format(wformat_string<Args...> fmt, const Args&... args) {
    return vformat(fmt.get(), make_wformat_args(args...));
}

template<typename... Args>
std::string format(const std::locale& loc, format_string<Args...> fmt, const Args&... args) {
    return vformat(loc, fmt.get(), make_format_args(args...));
}

template<typename... Args>
std::wstring format(const std::locale& loc, wformat_string<Args...> fmt, const Args&... args) {
    return vformat(loc, fmt.get(), make_wformat_args(args...));
}

// ---- vformat_to

inline char* vformat_to(char* p, std::string_view fmt, format_args args) {
    membuffer buf(p);
    return basic_vformat<membuffer>(buf, fmt, args).curr();
}

template<typename OutputIt, typename = std::enable_if_t<is_output_iterator<OutputIt, const char&>::value>>
OutputIt vformat_to(OutputIt out, std::string_view fmt, format_args args) {
    inline_dynbuffer buf;
    basic_vformat<membuffer>(buf, fmt, args);
    return std::copy_n(buf.data(), buf.size(), std::move(out));
}

inline wchar_t* vformat_to(wchar_t* p, std::wstring_view fmt, wformat_args args) {
    wmembuffer buf(p);
    return basic_vformat<wmembuffer>(buf, fmt, args).curr();
}

template<typename OutputIt, typename = std::enable_if_t<is_output_iterator<OutputIt, const wchar_t&>::value>>
OutputIt vformat_to(OutputIt out, std::wstring_view fmt, wformat_args args) {
    inline_wdynbuffer buf;
    basic_vformat<wmembuffer>(buf, fmt, args);
    return std::copy_n(buf.data(), buf.size(), std::move(out));
}

inline char* vformat_to(char* p, const std::locale& loc, std::string_view fmt, format_args args) {
    membuffer buf(p);
    return basic_vformat<membuffer>(buf, loc, fmt, args).curr();
}

template<typename OutputIt, typename = std::enable_if_t<is_output_iterator<OutputIt, const char&>::value>>
OutputIt vformat_to(OutputIt out, const std::locale& loc, std::string_view fmt, format_args args) {
    inline_dynbuffer buf;
    basic_vformat<membuffer>(buf, loc, fmt, args);
    return std::copy_n(buf.data(), buf.size(), std::move(out));
}

inline wchar_t* vformat_to(wchar_t* p, const std::locale& loc, std::wstring_view fmt, wformat_args args) {
    wmembuffer buf(p);
    return basic_vformat<wmembuffer>(buf, loc, fmt, args).curr();
}

template<typename OutputIt, typename = std::enable_if_t<is_output_iterator<OutputIt, const wchar_t&>::value>>
OutputIt vformat_to(OutputIt out, const std::locale& loc, std::wstring_view fmt, wformat_args args) {
    inline_wdynbuffer buf;
    basic_vformat<wmembuffer>(buf, loc, fmt, args);
    return std::copy_n(buf.data(), buf.size(), std::move(out));
}

// ---- format_to

template<typename OutputIt, typename... Args,
         typename = std::enable_if_t<is_output_iterator<OutputIt, const char&>::value>>
OutputIt format_to(OutputIt out, format_string<Args...> fmt, const Args&... args) {
    return vformat_to(std::move(out), fmt.get(), make_format_args(args...));
}

template<typename OutputIt, typename... Args,
         typename = std::enable_if_t<is_output_iterator<OutputIt, const wchar_t&>::value>>
OutputIt format_to(OutputIt out, wformat_string<Args...> fmt, const Args&... args) {
    return vformat_to(std::move(out), fmt.get(), make_wformat_args(args...));
}

template<typename OutputIt, typename... Args,
         typename = std::enable_if_t<is_output_iterator<OutputIt, const char&>::value>>
OutputIt format_to(OutputIt out, const std::locale& loc, format_string<Args...> fmt, const Args&... args) {
    return vformat_to(std::move(out), loc, fmt.get(), make_format_args(args...));
}

template<typename OutputIt, typename... Args,
         typename = std::enable_if_t<is_output_iterator<OutputIt, const wchar_t&>::value>>
OutputIt format_to(OutputIt out, const std::locale& loc, wformat_string<Args...> fmt, const Args&... args) {
    return vformat_to(std::move(out), loc, fmt.get(), make_wformat_args(args...));
}

// ---- vformat_to_n

template<typename OutputIt>
struct format_to_n_result {
#if __cplusplus < 201703L
    format_to_n_result(OutputIt o, std::size_t s) : out(o), size(s) {}
#endif  // __cplusplus < 201703L
    OutputIt out;
    std::size_t size;
};

template<typename Ty>
class basic_membuffer_with_size_counter final : public basic_membuffer<Ty> {
 public:
    basic_membuffer_with_size_counter(Ty* first, Ty* last) noexcept
        : basic_membuffer<Ty>(first, last), size_(static_cast<std::size_t>(last - first)) {}
    std::size_t size() const { return this->avail() ? size_ - this->avail() : size_; }

 private:
    std::size_t size_ = 0;

    std::size_t try_grow(std::size_t extra) override {
        size_ += extra;
        return 0;
    }
};

inline format_to_n_result<char*> vformat_to_n(char* p, std::size_t n, std::string_view fmt, format_args args) {
    basic_membuffer_with_size_counter<char> buf(p, p + n);
    return {basic_vformat<membuffer>(buf, fmt, args).curr(), buf.size()};
}

template<typename OutputIt, typename = std::enable_if_t<is_output_iterator<OutputIt, const char&>::value>>
format_to_n_result<OutputIt> vformat_to_n(OutputIt out, std::size_t n, std::string_view fmt, format_args args) {
    inline_dynbuffer buf;
    basic_vformat<membuffer>(buf, fmt, args);
    return {std::copy_n(buf.data(), std::min(buf.size(), n), std::move(out)), buf.size()};
}

inline format_to_n_result<wchar_t*> vformat_to_n(wchar_t* p, std::size_t n, std::wstring_view fmt, wformat_args args) {
    basic_membuffer_with_size_counter<wchar_t> buf(p, p + n);
    return {basic_vformat<wmembuffer>(buf, fmt, args).curr(), buf.size()};
}

template<typename OutputIt, typename = std::enable_if_t<is_output_iterator<OutputIt, const wchar_t&>::value>>
format_to_n_result<OutputIt> vformat_to_n(OutputIt out, std::size_t n, std::wstring_view fmt, wformat_args args) {
    inline_wdynbuffer buf;
    basic_vformat<wmembuffer>(buf, fmt, args);
    return {std::copy_n(buf.data(), std::min(buf.size(), n), std::move(out)), buf.size()};
}

inline format_to_n_result<char*> vformat_to_n(char* p, std::size_t n, const std::locale& loc, std::string_view fmt,
                                              format_args args) {
    basic_membuffer_with_size_counter<char> buf(p, p + n);
    return {basic_vformat<membuffer>(buf, loc, fmt, args).curr(), buf.size()};
}

template<typename OutputIt, typename = std::enable_if_t<is_output_iterator<OutputIt, const char&>::value>>
format_to_n_result<OutputIt> vformat_to_n(OutputIt out, std::size_t n, const std::locale& loc, std::string_view fmt,
                                          format_args args) {
    inline_dynbuffer buf;
    basic_vformat<membuffer>(buf, loc, fmt, args);
    return {std::copy_n(buf.data(), std::min(buf.size(), n), std::move(out)), buf.size()};
}

inline format_to_n_result<wchar_t*> vformat_to_n(wchar_t* p, std::size_t n, const std::locale& loc,
                                                 std::wstring_view fmt, wformat_args args) {
    basic_membuffer_with_size_counter<wchar_t> buf(p, p + n);
    return {basic_vformat<wmembuffer>(buf, loc, fmt, args).curr(), buf.size()};
}

template<typename OutputIt, typename = std::enable_if_t<is_output_iterator<OutputIt, const wchar_t&>::value>>
format_to_n_result<OutputIt> vformat_to_n(OutputIt out, std::size_t n, const std::locale& loc, std::wstring_view fmt,
                                          wformat_args args) {
    inline_wdynbuffer buf;
    basic_vformat<wmembuffer>(buf, loc, fmt, args);
    return {std::copy_n(buf.data(), std::min(buf.size(), n), std::move(out)), buf.size()};
}

// ---- format_to_n

template<typename OutputIt, typename... Args,
         typename = std::enable_if_t<is_output_iterator<OutputIt, const char&>::value>>
format_to_n_result<OutputIt> format_to_n(OutputIt out, std::size_t n, format_string<Args...> fmt, const Args&... args) {
    return vformat_to_n(std::move(out), n, fmt.get(), make_format_args(args...));
}

template<typename OutputIt, typename... Args,
         typename = std::enable_if_t<is_output_iterator<OutputIt, const wchar_t&>::value>>
format_to_n_result<OutputIt> format_to_n(OutputIt out, std::size_t n, wformat_string<Args...> fmt, const Args&... args) {
    return vformat_to_n(std::move(out), n, fmt.get(), make_wformat_args(args...));
}

template<typename OutputIt, typename... Args,
         typename = std::enable_if_t<is_output_iterator<OutputIt, const char&>::value>>
format_to_n_result<OutputIt> format_to_n(OutputIt out, std::size_t n, const std::locale& loc,
                                         format_string<Args...> fmt, const Args&... args) {
    return vformat_to_n(std::move(out), n, loc, fmt.get(), make_format_args(args...));
}

template<typename OutputIt, typename... Args,
         typename = std::enable_if_t<is_output_iterator<OutputIt, const wchar_t&>::value>>
format_to_n_result<OutputIt> format_to_n(OutputIt out, std::size_t n, const std::locale& loc,
                                         wformat_string<Args...> fmt, const Args&... args) {
    return vformat_to_n(std::move(out), n, loc, fmt.get(), make_wformat_args(args...));
}

// ---- vprint

template<typename CharT, typename Traits = std::char_traits<CharT>>
basic_iobuf<CharT>& print_quoted_text(basic_iobuf<CharT>& out, std::basic_string_view<CharT, Traits> text) {
    auto it0 = text.data();
    out.put('\"');
    for (auto it = it0; it != text.end(); ++it) {
        char esc = '\0';
        switch (*it) {
            case '\"': esc = '\"'; break;
            case '\\': esc = '\\'; break;
            case '\a': esc = 'a'; break;
            case '\b': esc = 'b'; break;
            case '\f': esc = 'f'; break;
            case '\n': esc = 'n'; break;
            case '\r': esc = 'r'; break;
            case '\t': esc = 't'; break;
            case '\v': esc = 'v'; break;
            default: continue;
        }
        out.write(uxs::as_span(it0, it - it0));
        out.put('\\'), out.put(esc);
        it0 = it + 1;
    }
    out.write(uxs::as_span(it0, text.end() - it0));
    out.put('\"');
    return out;
}

template<typename Ty>
class basic_membuffer_for_iobuf final : public basic_membuffer<Ty> {
 public:
    explicit basic_membuffer_for_iobuf(basic_iobuf<Ty>& out) noexcept
        : basic_membuffer<Ty>(out.first_avail(), out.last_avail()), out_(out) {}
    ~basic_membuffer_for_iobuf() override { out_.advance(this->curr() - out_.first_avail()); }

 private:
    basic_iobuf<Ty>& out_;

    std::size_t try_grow(std::size_t extra) override {
        out_.advance(this->curr() - out_.first_avail());
        if (!out_.reserve().good()) { return 0; }
        this->set(out_.first_avail(), out_.last_avail());
        return this->avail();
    }
};

inline iobuf& vprint(iobuf& out, std::string_view fmt, format_args args) {
    basic_membuffer_for_iobuf<char> buf(out);
    basic_vformat<membuffer>(buf, fmt, args);
    return out;
}

inline wiobuf& vprint(wiobuf& out, std::wstring_view fmt, wformat_args args) {
    basic_membuffer_for_iobuf<wchar_t> buf(out);
    basic_vformat<wmembuffer>(buf, fmt, args);
    return out;
}

inline iobuf& vprint(iobuf& out, const std::locale& loc, std::string_view fmt, format_args args) {
    basic_membuffer_for_iobuf<char> buf(out);
    basic_vformat<membuffer>(buf, loc, fmt, args);
    return out;
}

inline wiobuf& vprint(wiobuf& out, const std::locale& loc, std::wstring_view fmt, wformat_args args) {
    basic_membuffer_for_iobuf<wchar_t> buf(out);
    basic_vformat<wmembuffer>(buf, loc, fmt, args);
    return out;
}

// ---- print

template<typename... Args>
iobuf& print(iobuf& out, format_string<Args...> fmt, const Args&... args) {
    return vprint(out, fmt.get(), make_format_args(args...));
}

template<typename... Args>
wiobuf& print(wiobuf& out, wformat_string<Args...> fmt, const Args&... args) {
    return vprint(out, fmt.get(), make_wformat_args(args...));
}

template<typename... Args>
iobuf& print(format_string<Args...> fmt, const Args&... args) {
    return vprint(stdbuf::out, fmt.get(), make_format_args(args...));
}

template<typename... Args>
iobuf& print(iobuf& out, const std::locale& loc, format_string<Args...> fmt, const Args&... args) {
    return vprint(out, loc, fmt.get(), make_format_args(args...));
}

template<typename... Args>
wiobuf& print(wiobuf& out, const std::locale& loc, wformat_string<Args...> fmt, const Args&... args) {
    return vprint(out, loc, fmt.get(), make_wformat_args(args...));
}

template<typename... Args>
iobuf& print(const std::locale& loc, format_string<Args...> fmt, const Args&... args) {
    return vprint(stdbuf::out, loc, fmt.get(), make_format_args(args...));
}

// ---- println

template<typename... Args>
iobuf& println(iobuf& out, format_string<Args...> fmt, const Args&... args) {
    return vprint(out, fmt.get(), make_format_args(args...)).endl();
}

template<typename... Args>
wiobuf& println(wiobuf& out, wformat_string<Args...> fmt, const Args&... args) {
    return vprint(out, fmt.get(), make_wformat_args(args...)).endl();
}

template<typename... Args>
iobuf& println(format_string<Args...> fmt, const Args&... args) {
    return vprint(stdbuf::out, fmt.get(), make_format_args(args...)).endl();
}

template<typename... Args>
iobuf& println(iobuf& out, const std::locale& loc, format_string<Args...> fmt, const Args&... args) {
    return vprint(out, loc, fmt.get(), make_format_args(args...)).endl();
}

template<typename... Args>
wiobuf& println(wiobuf& out, const std::locale& loc, wformat_string<Args...> fmt, const Args&... args) {
    return vprint(out, loc, fmt.get(), make_wformat_args(args...)).endl();
}

template<typename... Args>
iobuf& println(const std::locale& loc, format_string<Args...> fmt, const Args&... args) {
    return vprint(stdbuf::out, loc, fmt.get(), make_format_args(args...)).endl();
}

}  // namespace uxs
