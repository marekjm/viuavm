#include <sstream>
#include <string>

#include <viua/support/variant.h>
#include <viua/vm/types.h>


namespace viua::vm::types {
Cell_view::Cell_view(Cell& cell) : content{std::monostate{}}
{
    std::visit(
        [this](auto&& arg) -> void {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                // do nothing
            } else if constexpr (std::is_same_v<T, int64_t>) {
                content = std::reference_wrapper<int64_t>(arg);
            } else if constexpr (std::is_same_v<T, uint64_t>) {
                content = std::reference_wrapper<uint64_t>(arg);
            } else if constexpr (std::is_same_v<T, float>) {
                content = std::reference_wrapper<float>(arg);
            } else if constexpr (std::is_same_v<T, double>) {
                content = std::reference_wrapper<double>(arg);
            } else if constexpr (std::is_same_v<T, Cell::boxed_type>) {
                content = std::reference_wrapper<boxed_type>(*arg.get());
            } else {
                viua::support::non_exhaustive_visitor<T>();
            }
        },
        cell.content);
}
Cell_view::Cell_view(boxed_type& v) : content{v}
{}

Value::~Value()
{}

auto Value::reference_to() -> std::unique_ptr<Ref>
{
    return std::make_unique<Ref>(this);
}
}  // namespace viua::vm::types

namespace viua::vm::types {
auto Signed_integer::type_name() const -> std::string
{
    return "int";
}
auto Signed_integer::to_string() const -> std::string
{
    return std::to_string(value);
}

auto Unsigned_integer::type_name() const -> std::string
{
    return "uint";
}
auto Unsigned_integer::to_string() const -> std::string
{
    return std::to_string(value);
}

auto Float_single::type_name() const -> std::string
{
    return "float";
}
auto Float_single::to_string() const -> std::string
{
    return std::to_string(value);
}

auto Float_double::type_name() const -> std::string
{
    return "double";
}
auto Float_double::to_string() const -> std::string
{
    return std::to_string(value);
}
}  // namespace viua::vm::types

namespace viua::vm::types {
auto Ref::type_name() const -> std::string
{
    return ('*' + value->type_name());
}
auto Ref::to_string() const -> std::string
{
    return value->as_trait<traits::To_string, std::string>(
        [](traits::To_string const& ts) -> std::string {
            return ts.to_string();
        },
        type_name());
}

auto String::type_name() const -> std::string
{
    return "string";
}
auto String::to_string() const -> std::string
{
    return viua::vm::types::traits::To_string::quote_and_escape(content);
}
String::operator bool() const
{
    return (not content.empty());
}
auto String::operator()(traits::Plus::tag_type const, Cell const& c) const
    -> Cell
{
    if (not std::holds_alternative<Cell::boxed_type>(c.content)) {
        throw std::runtime_error{"cannot add unboxed value to String"};
    }

    auto const v = dynamic_cast<String*>(c.get<Cell::boxed_type>().get());
    if (not v) {
        throw std::runtime_error{"cannot add "
                                 + c.get<Cell::boxed_type>()->type_name()
                                 + " value to String"};
    }

    auto s     = std::make_unique<String>();
    s->content = (content + v->content);
    return Cell{std::move(s)};
}
auto String::operator() (traits::Cmp const&, Cell_view const& v) const -> std::strong_ordering
{
    if (not v.holds<Value>()) {
        throw std::runtime_error{"cannot compare unboxed value to String"};
    }

    auto const a = v.boxed_of<String>();
    if (not a) {
        throw std::runtime_error{"cannot compare "
                                 + v.boxed_of<Value>().value().get().type_name()
                                 + " value to String"};
    }

    return (a.value().get().content <=> content);
}

auto Atom::type_name() const -> std::string
{
    return "atom";
}
auto Atom::to_string() const -> std::string
{
    return viua::vm::types::traits::To_string::quote_and_escape(content);
}

auto Atom::operator() (traits::Eq const&, Cell_view const& v) const -> std::partial_ordering
{
    if (not v.holds<Value>()) {
        throw std::runtime_error{"cannot compare unboxed value to Atom"};
    }

    auto const a = v.boxed_of<Atom>();
    if (not a) {
        throw std::runtime_error{"cannot compare "
                                 + v.boxed_of<Value>().value().get().type_name()
                                 + " value to Atom"};
    }

    return (a.value().get().content <=> content);
}

auto stringify_cell(Cell const& vc) -> std::string
{
    if (std::holds_alternative<int64_t>(vc.content)) {
        return std::to_string(std::get<int64_t>(vc.content));
    } else if (std::holds_alternative<uint64_t>(vc.content)) {
        return std::to_string(std::get<uint64_t>(vc.content));
    } else if (std::holds_alternative<float>(vc.content)) {
        return std::to_string(std::get<float>(vc.content));
    } else if (std::holds_alternative<double>(vc.content)) {
        return std::to_string(std::get<double>(vc.content));
    } else {
        auto const* v = vc.get<Cell::boxed_type>().get();
        auto const* s =
            dynamic_cast<viua::vm::types::traits::To_string const*>(v);
        if (s) {
            return s->to_string();
        } else {
            return ("<value of " + v->type_name() + ">");
        }
    }
}

auto Struct::insert(key_type const key, value_type&& value) -> void
{
    values[key] = std::move(value);
}
auto Struct::at(key_type const key) -> value_type&
{
    return values.at(key);
}
auto Struct::remove(key_type const key) -> value_type
{
    auto v = std::move(values.at(key));
    values.erase(key);
    return v;
}
auto Struct::type_name() const -> std::string
{
    return "struct";
}
auto Struct::to_string() const -> std::string
{
    if (values.empty()) {
        return "{}";
    }

    auto out = std::ostringstream{};
    out << "{";

    auto first = true;
    for (auto const& [k, v] : values) {
        if (not first) {
            out << ", ";
        }
        first = false;

        out << k << " = " << stringify_cell(v);
    }

    out << "}";
    return out.str();
}

auto Buffer::type_name() const -> std::string
{
    return "buffer";
}
auto Buffer::to_string() const -> std::string
{
    if (values.empty()) {
        return "[]";
    }

    auto out = std::ostringstream{};
    out << "[";
    out << stringify_cell(values.front());
    for (auto i = size_t{1}; i < values.size(); ++i) {
        out << ", " << stringify_cell(values.at(i));
    }
    out << "]";
    return out.str();
}
auto Buffer::push(value_type&& v) -> void
{
    values.push_back(std::move(v));
}
auto Buffer::at(size_type const n) -> value_type&
{
    return values.at(n);
}
auto Buffer::pop(size_type const n) -> value_type
{
    auto v = std::move(values.at(n));
    values.erase(values.begin() + n);
    return v;
}
auto Buffer::size() const -> size_type
{
    return values.size();
}
}  // namespace viua::vm::types
