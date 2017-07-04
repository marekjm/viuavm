#include <bitset>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <vector>
using namespace std;

template<unsigned long I> auto ashl(bitset<I> bs, unsigned long a) -> bitset<I> {
    bool sign = bs.test(I - 1);
    auto shifted = (bs << a);
    if (sign) {
        shifted.set(I - 1);
    }
    return shifted;
}

template<unsigned long I> auto ashr(bitset<I> bs, unsigned long a) -> bitset<I> {
    bool sign = bs.test(I - 1);
    auto shifted = (bs >> a);
    if (sign) {
        shifted.set(I - 1);
    }
    return shifted;
}

class Bits {
    vector<bool> underlying_array;

  public:
    using size_type = decltype(underlying_array)::size_type;

    auto at(size_type) const -> bool;
    auto set(size_type, const bool = true) -> bool;
    auto flip(size_type) -> bool;

    auto clear() -> void;

    auto shl(size_type) -> Bits;
    auto shr(size_type) -> Bits;
    auto ashl(size_type) -> Bits;
    auto ashr(size_type) -> Bits;

    auto operator|(const Bits&) const -> Bits;

    operator string() const;

    Bits(size_type = 1);
};

Bits::Bits(size_type i) {
    underlying_array.reserve(i);
    for (; i; --i) {
        underlying_array.push_back(false);
    }
}

Bits::operator string() const {
    ostringstream oss;

    for (size_type i = underlying_array.size(); i; --i) {
        oss << underlying_array.at(i - 1);
        if (((i - 1) % 4 == 0) and i != 1) {
            oss << ':';
        }
    }

    return oss.str();
}

auto Bits::at(size_type i) const -> bool { return underlying_array.at(i); }

auto Bits::set(size_type i, const bool value) -> bool {
    bool was = at(i);
    underlying_array.at(i) = value;
    return was;
}

auto Bits::flip(size_type i) -> bool {
    set(i, not at(i));
    return at(i);
}

auto Bits::clear() -> void {
    for (size_type i = underlying_array.size(); i; --i) {
        set(i - 1, false);
    }
}

auto Bits::shl(size_type n) -> Bits {
    Bits shifted{n};

    if (n >= underlying_array.size()) {
        clear();
        return shifted;
    }

    for (size_type i = 0; i < underlying_array.size(); ++i) {
        auto index_to_set = underlying_array.size() - 1 - i;
        auto index_of_value = underlying_array.size() - 1 - i - n;
        auto index_to_set_in_shifted = (n - 1 - i);

        if (index_of_value < underlying_array.size()) {
            if (index_to_set_in_shifted < n) {
                shifted.set(index_to_set_in_shifted, at(index_to_set));
            }
            set(index_to_set, at(index_of_value));
            set(index_of_value, false);
        } else {
            if (index_to_set_in_shifted < n) {
                shifted.set(index_to_set_in_shifted, at(index_to_set));
            }
            set(index_to_set, false);
        }
    }

    return shifted;
}

auto Bits::shr(size_type n) -> Bits {
    Bits shifted{n};

    if (n >= underlying_array.size()) {
        clear();
        return shifted;
    }

    for (size_type i = 0; i < underlying_array.size(); ++i) {
        auto index_to_set = i;
        auto index_of_value = i + n;
        auto index_to_set_in_shifted = i;

        if (index_of_value < underlying_array.size()) {
            if (index_to_set_in_shifted < n) {
                shifted.set(index_to_set_in_shifted, at(index_to_set));
            }
            set(index_to_set, at(index_of_value));
            set(index_of_value, false);
        } else {
            if (index_to_set_in_shifted < n) {
                shifted.set(index_to_set_in_shifted, at(index_to_set));
            }
            set(index_to_set, false);
        }
    }

    return shifted;
}

auto Bits::ashl(size_type n) -> Bits {
    auto sign = at(underlying_array.size() - 1);
    Bits shifted = shl(n);
    set(underlying_array.size() - 1, sign);
    return shifted;
}

auto Bits::ashr(size_type n) -> Bits {
    auto sign = at(underlying_array.size() - 1);
    Bits shifted = shr(n);
    set(underlying_array.size() - 1, sign);
    return shifted;
}

auto Bits::operator|(const Bits& that) const -> Bits {
    Bits ored{underlying_array.size()};

    for (size_type i = 0; i < underlying_array.size(); ++i) {
        ored.set(i, (at(i) or that.at(i)));
    }

    return ored;
}

auto operator<<(ostream& os, const Bits& bits) -> ostream& {
    os << string(bits);
    return os;
}

int main() {
    bitset<8> bs;
    bs.set(0);
    bs.set(2);
    bs.set(4);
    bs.set(6);
    bs.set(7);
    cout << "bs  = " << bs << endl;

    bs <<= 5;
    bs.set(0);
    cout << "bs  = " << bs << endl;

    bs >>= 6;
    cout << "bs  = " << bs << endl;

    Bits bts{8};

    bts.set(0);
    bts.set(2);
    bts.set(4);
    bts.set(6);
    bts.set(7);
    cout << "bts = " << bts << endl;

    Bits s0 = bts.shl(5);
    bts.set(0);
    cout << "bts = " << bts << endl;
    cout << "(shifted) = " << s0 << endl;

    Bits s1 = bts.shr(6);
    cout << "bts = " << bts << endl;
    cout << "(shifted) = " << s1 << endl;

    cout << "----" << endl;

    bts.clear();
    bts.set(0);
    bts.set(2);
    bts.set(4);
    bts.set(6);
    bts.set(7);
    cout << "bts = " << bts << endl;

    Bits s2 = bts.ashl(4);
    cout << "bts = " << bts << endl;
    cout << "(shifted) = " << s2 << endl;

    Bits s3 = bts.ashr(4);
    cout << "bts = " << bts << endl;
    cout << "(shifted) = " << s3 << endl;

    cout << "----" << endl;

    Bits or_lhs{8};
    or_lhs.set(0);
    or_lhs.set(1);
    or_lhs.set(4);
    or_lhs.set(7);

    Bits or_rhs{8};
    or_rhs.set(0);
    or_rhs.set(2);
    or_rhs.set(7);

    Bits or_result = or_lhs | or_rhs;
    cout << or_lhs << " | " << or_rhs << " == " << or_result << endl;
}
