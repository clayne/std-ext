#pragma once

#include "value.h"
#include "xml.h"

#include "uxs/stringalg.h"

namespace uxs {
namespace db {
namespace xml {

// --------------------------

template<typename CharT, typename Alloc>
basic_value<CharT, Alloc> reader::read(std::string_view root_element, const Alloc& al) {
    if (input_.peek() == ibuf::traits_type::eof()) { throw database_error("empty input"); }

    auto text_to_value = [&al](std::string_view sval) -> basic_value<CharT, Alloc> {
        switch (classify_string(sval)) {
            case string_class::null: return {nullptr, al};
            case string_class::true_value: return {true, al};
            case string_class::false_value: return {false, al};
            case string_class::integer_number: {
                std::uint64_t u64 = 0;
                if (stoval(sval, u64) != 0) {
                    if (u64 <= static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())) {
                        return {static_cast<std::int32_t>(u64), al};
                    } else if (u64 <= static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max())) {
                        return {static_cast<std::uint32_t>(u64), al};
                    } else if (u64 <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                        return {static_cast<std::int64_t>(u64), al};
                    }
                    return {u64, al};
                }
                // too big integer - treat as double
                return {from_string<double>(sval), al};
            } break;
            case string_class::negative_integer_number: {
                std::int64_t i64 = 0;
                if (stoval(sval, i64) != 0) {
                    if (i64 >= static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min())) {
                        return {static_cast<std::int32_t>(i64), al};
                    }
                    return {i64, al};
                }
                // too big integer - treat as double
                return {from_string<double>(sval), al};
            } break;
            case string_class::floating_point_number: return {from_string<double>(sval), al};
            case string_class::ws_with_nl: return make_record<CharT>(al);
            case string_class::other: return {utf_string_adapter<CharT>{}(sval), al};
            default: UXS_UNREACHABLE_CODE;
        }
    };

    inline_dynbuffer txt;
    basic_value<CharT, Alloc> result;
    std::vector<std::pair<basic_value<CharT, Alloc>*, std::string>> stack;
    std::pair<token_t, std::string_view> tk = read_next();

    stack.reserve(32);
    stack.emplace_back(&result, root_element);

    while (true) {
        auto* top = &stack.back();
        switch (tk.first) {
            case token_t::eof: throw database_error(format("{}: unexpected end of file", n_ln_));
            case token_t::preamble: throw database_error(format("{}: unexpected document preamble", n_ln_));
            case token_t::entity: throw database_error(format("{}: unknown entity name", n_ln_));
            case token_t::plain_text: {
                if (!top->first->is_record()) { txt += tk.second; }
            } break;
            case token_t::start_element: {
                txt.clear();
                auto result = top->first->emplace_unique(utf_string_adapter<CharT>{}(tk.second), al);
                stack.emplace_back(&result.first->second, tk.second);
                if (!result.second) {
                    result.first->second.convert(dtype::array);
                    stack.back().first = &result.first->second.emplace_back(al);
                }
            } break;
            case token_t::end_element: {
                if (top->second != tk.second) {
                    throw database_error(format("{}: unterminated element {}", n_ln_, top->second));
                }
                if (!top->first->is_record()) {
                    *(top->first) = text_to_value(std::string_view(txt.data(), txt.size()));
                }
                stack.pop_back();
                if (stack.empty()) { return result; }
            } break;
            default: UXS_UNREACHABLE_CODE;
        }
        tk = read_next();
    }
}

// --------------------------

template<typename CharT, typename Alloc>
struct writer_stack_item_t {
    using value_t = basic_value<CharT, Alloc>;
    using string_type = std::conditional_t<std::is_same<CharT, char>::value, std::string_view, std::string>;
    writer_stack_item_t() {}
    writer_stack_item_t(const value_t* p, std::string_view el, const value_t* it) : v(p), element(el), array_it(it) {}
    writer_stack_item_t(const value_t* p, std::string_view el, typename value_t::const_record_iterator it)
        : v(p), element(el), record_it(it) {}
    const value_t* v;
    string_type element;
    union {
        const value_t* array_it;
        typename value_t::const_record_iterator record_it;
    };
};

template<typename CharT>
basic_membuffer<CharT>& print_xml_text(basic_membuffer<CharT>& out, std::basic_string_view<CharT> text) {
    auto it0 = text.begin();
    for (auto it = it0; it != text.end(); ++it) {
        std::string_view esc;
        switch (*it) {
            case '&': esc = std::string_view("&amp;", 5); break;
            case '<': esc = std::string_view("&lt;", 4); break;
            case '>': esc = std::string_view("&gt;", 4); break;
            case '\'': esc = std::string_view("&apos;", 6); break;
            case '\"': esc = std::string_view("&quot;", 6); break;
            default: continue;
        }
        out.append(it0, it).append(esc);
        it0 = it + 1;
    }
    out.append(it0, text.end());
    return out;
}

template<typename CharT, typename Alloc>
void writer::write(const basic_value<CharT, Alloc>& v, std::string_view root_element, unsigned indent) {
    std::vector<writer_stack_item_t<CharT, Alloc>> stack;
    typename writer_stack_item_t<CharT, Alloc>::string_type element(root_element);
    stack.reserve(32);

    auto write_value = [this, &stack, &element, &indent](const basic_value<CharT, Alloc>& v) {
        switch (v.type_) {
            case dtype::null: output_.append("null", 4); break;
            case dtype::boolean: {
                output_.append(v.value_.b ? std::string_view("true", 4) : std::string_view("false", 5));
            } break;
            case dtype::integer: {
                to_basic_string(output_, v.value_.i);
            } break;
            case dtype::unsigned_integer: {
                to_basic_string(output_, v.value_.u);
            } break;
            case dtype::long_integer: {
                to_basic_string(output_, v.value_.i64);
            } break;
            case dtype::unsigned_long_integer: {
                to_basic_string(output_, v.value_.u64);
            } break;
            case dtype::double_precision: {
                to_basic_string(output_, v.value_.dbl, fmt_opts{fmt_flags::json_compat});
            } break;
            case dtype::string: {
                print_xml_text<char>(output_, utf8_string_adapter{}(v.str_view()));
            } break;
            case dtype::array: {
                stack.emplace_back(&v, element, v.as_array().data());
                return true;
            } break;
            case dtype::record: {
                indent += indent_size_;
                stack.emplace_back(&v, element, v.as_record().begin());
                return true;
            } break;
        }
        return false;
    };

    output_.push_back('<');
    output_.append(element).push_back('>');
    if (!write_value(v)) {
        output_.append("</", 2).append(element).push_back('>');
        return;
    }

loop:
    auto* top = &stack.back();
    if (top->v->is_array()) {
        auto range = top->v->as_array();
        const auto* el = top->array_it;
        const auto* el_end = range.data() + range.size();
        while (true) {
            if (el != range.data() && !(el - 1)->is_array()) { output_.append("</", 2).append(element).push_back('>'); }
            if (el == el_end) { break; }
            if (!el->is_array()) {
                output_.push_back('\n');
                output_.append(indent, indent_char_).push_back('<');
                output_.append(element).push_back('>');
            }
            if (write_value(*el++)) {
                std::prev(stack.end(), 2)->array_it = el;
                goto loop;
            }
        }
    } else {
        auto range = top->v->as_record();
        auto el = top->record_it;
        while (true) {
            if (el != range.begin() && !std::prev(el)->second.is_array()) {
                output_.append("</", 2).append(element).push_back('>');
            }
            if (el == range.end()) { break; }
            element = utf8_string_adapter{}(el->first);
            if (!el->second.is_array()) {
                output_.push_back('\n');
                output_.append(indent, indent_char_).push_back('<');
                output_.append(element).push_back('>');
            }
            if (write_value((el++)->second)) {
                std::prev(stack.end(), 2)->record_it = el;
                goto loop;
            }
        }
        indent -= indent_size_;
        output_.push_back('\n');
        output_.append(indent, indent_char_);
    }

    element = top->element;
    stack.pop_back();
    if (!stack.empty()) { goto loop; }
    output_.append("</", 2).append(element).push_back('>');
}

}  // namespace xml
}  // namespace db
}  // namespace uxs
