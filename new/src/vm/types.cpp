#include <string>

#include <viua/vm/types.h>

namespace viua::vm::types {
Value::~Value()
{}

auto String::type_name() const -> std::string
{
    return "string";
}
auto String::to_string() const -> std::string
{
    return viua::vm::types::traits::To_string::quote_and_escape(content);
}
String::operator bool () const
{
    return (not content.empty());
}
auto String::operator()(traits::Plus::tag_type const, Register_cell const& c) const -> Register_cell
{
    if (not std::holds_alternative<std::unique_ptr<Value>>(c)) {
        throw std::runtime_error{"cannot add unboxed value to String"};
    }

    auto const v = dynamic_cast<String*>(std::get<std::unique_ptr<Value>>(c).get());
    if (not v) {
        throw std::runtime_error{"cannot add "
            + std::get<std::unique_ptr<Value>>(c)->type_name() + " value to String"};
    }

    auto s = std::make_unique<String>();
    s->content = (content + v->content);
    return s;
}
auto String::operator()(traits::Eq::tag_type const, Register_cell const& c) const -> Register_cell
{
    if (not std::holds_alternative<std::unique_ptr<Value>>(c)) {
        throw std::runtime_error{"cannot compare unboxed value to String"};
    }

    auto const v = dynamic_cast<String*>(std::get<std::unique_ptr<Value>>(c).get());
    if (not v) {
        throw std::runtime_error{"cannot compare "
            + std::get<std::unique_ptr<Value>>(c)->type_name() + " value to String"};
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
auto Atom::operator()(traits::Eq::tag_type const, Register_cell const& c) const -> Register_cell
{
    if (not std::holds_alternative<std::unique_ptr<Value>>(c)) {
        throw std::runtime_error{"cannot compare unboxed value to Atom"};
    }

    auto const v = dynamic_cast<Atom*>(std::get<std::unique_ptr<Value>>(c).get());
    if (not v) {
        throw std::runtime_error{"cannot compare "
            + std::get<std::unique_ptr<Value>>(c)->type_name() + " value to Atom"};
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

auto Buffer::type_name() const -> std::string
{
    return "buffer";
}
auto Buffer::to_string() const -> std::string
{
    return "[]";
}
}  // namespace viua::vm::types
