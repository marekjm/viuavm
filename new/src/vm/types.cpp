#include <sstream>
#include <string>

#include <viua/vm/types.h>


namespace viua::vm::types {
Value::~Value()
{}
}

namespace viua::vm::types {
auto Signed_integer::type_name() const -> std::string
{
    return "int";
}
auto Unsigned_integer::type_name() const -> std::string
{
    return "uint";
}
auto Float_single::type_name() const -> std::string
{
    return "float";
}
auto Float_double::type_name() const -> std::string
{
    return "double";
}
}

namespace viua::vm::types {
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
auto String::operator()(traits::Plus::tag_type const,
                        Register_cell const& c) const -> Register_cell
{
    if (not std::holds_alternative<std::unique_ptr<Value>>(c)) {
        throw std::runtime_error{"cannot add unboxed value to String"};
    }

    auto const v =
        dynamic_cast<String*>(std::get<std::unique_ptr<Value>>(c).get());
    if (not v) {
        throw std::runtime_error{
            "cannot add " + std::get<std::unique_ptr<Value>>(c)->type_name()
            + " value to String"};
    }

    auto s     = std::make_unique<String>();
    s->content = (content + v->content);
    return s;
}
auto String::operator()(traits::Eq::tag_type const,
                        Register_cell const& c) const -> Register_cell
{
    if (not std::holds_alternative<std::unique_ptr<Value>>(c)) {
        throw std::runtime_error{"cannot compare unboxed value to String"};
    }

    auto const v =
        dynamic_cast<String*>(std::get<std::unique_ptr<Value>>(c).get());
    if (not v) {
        throw std::runtime_error{
            "cannot compare " + std::get<std::unique_ptr<Value>>(c)->type_name()
            + " value to String"};
    }

    return static_cast<uint64_t>(v->content == content);
}

auto Atom::type_name() const -> std::string
{
    return "atom";
}
auto Atom::to_string() const -> std::string
{
    return viua::vm::types::traits::To_string::quote_and_escape(content);
}
auto Atom::operator()(traits::Eq::tag_type const, Register_cell const& c) const
    -> Register_cell
{
    if (not std::holds_alternative<std::unique_ptr<Value>>(c)) {
        throw std::runtime_error{"cannot compare unboxed value to Atom"};
    }

    auto const v =
        dynamic_cast<Atom*>(std::get<std::unique_ptr<Value>>(c).get());
    if (not v) {
        throw std::runtime_error{
            "cannot compare " + std::get<std::unique_ptr<Value>>(c)->type_name()
            + " value to Atom"};
    }

    return static_cast<uint64_t>(v->content == content);
}

auto Struct::type_name() const -> std::string
{
    return "struct";
}
auto Struct::to_string() const -> std::string
{
    return "{}";
}

auto stringify_cell(Value_cell const& vc) -> std::string
{
    if (std::holds_alternative<int64_t>(vc)) {
        return std::to_string(std::get<int64_t>(vc));
    } else if (std::holds_alternative<uint64_t>(vc)) {
        return std::to_string(std::get<uint64_t>(vc));
    } else if (std::holds_alternative<float>(vc)) {
        return std::to_string(std::get<float>(vc));
    } else if (std::holds_alternative<double>(vc)) {
        return std::to_string(std::get<double>(vc));
    } else {
        auto const* v = std::get<std::unique_ptr<viua::vm::types::Value>>(vc).get();
        auto const* s = dynamic_cast<viua::vm::types::traits::To_string const*>(v);
        if (s) {
            return s->to_string();
        } else {
            return ("<value of " + v->type_name() + ">");
        }
    }
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
auto Buffer::push(Value_cell&& v) -> void
{
    values.push_back(std::move(v));
}
auto Buffer::pop(size_t const n) -> Value_cell
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
