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
}  // namespace viua::vm::types
