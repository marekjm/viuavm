#include <cstdint>
#include <string>
#include <iostream>
#include <vector>
using namespace std;


const uint8_t UTF8_1ST_ROW = 0b00000000;
const uint8_t UTF8_2ND_ROW = 0b11000000;
const uint8_t UTF8_3RD_ROW = 0b11100000;
const uint8_t UTF8_4TH_ROW = 0b11110000;
const uint8_t UTF8_FILLING = 0b10000000;

const uint8_t UTF8_1ST_ROW_NORMALISER = 0b10000000;
const uint8_t UTF8_2ND_ROW_NORMALISER = 0b11100000;
const uint8_t UTF8_3RD_ROW_NORMALISER = 0b11110000;
const uint8_t UTF8_4TH_ROW_NORMALISER = 0b11111000;
const uint8_t UTF8_FILLING_NORMALISER = 0b11000000;


using Character = string;

class Text {
    string text;

    mutable decltype(text)::size_type current_size { 0 };

    public:

    using size_type = decltype(text)::size_type;

    auto size() const -> size_type;
    auto at(size_type) const -> Character;

    auto str() const -> string;

    auto operator [] (size_type) const -> Character;

    Text(string);
};

auto operator << (ostream& out, const Text& text) -> ostream& {
    out << text.str();
    return out;
}


Text::Text(string s): text(std::move(s)) {
}

static auto is_continuation_byte(uint8_t b) -> bool {
    return ((UTF8_FILLING_NORMALISER & b) == UTF8_FILLING);
}

auto Text::size() const -> size_type {
    if (current_size) {
        return current_size;
    }

    char ss[5];
    ss[4] = '\0';

    ssize_t sz = 0;

    for (std::remove_reference<decltype(text)>::type::size_type i = 0; i < text.size(); ++i, ++sz) {
        const auto each { text.at(i) };
        ss[0] = each;
        ss[1] = '\0';
        ss[2] = '\0';
        ss[3] = '\0';

        if ((UTF8_1ST_ROW_NORMALISER & each) == UTF8_1ST_ROW) {
            // do nothing
        } else if ((UTF8_2ND_ROW_NORMALISER & each) == UTF8_2ND_ROW) {
            ss[1] = text.at(i+1);
        } else if ((UTF8_3RD_ROW_NORMALISER & each) == UTF8_3RD_ROW) {
            ss[1] = text.at(i+1);
            ss[2] = text.at(i+2);
        } else if ((UTF8_4TH_ROW_NORMALISER & each) == UTF8_4TH_ROW) {
            ss[1] = text.at(i+1);
            ss[2] = text.at(i+2);
            ss[3] = text.at(i+3);
        }

        if ((UTF8_1ST_ROW_NORMALISER & ss[0]) == UTF8_1ST_ROW) {
            // do nothing
        } else if ((UTF8_2ND_ROW_NORMALISER & ss[0]) == UTF8_2ND_ROW and is_continuation_byte(ss[1])) {
            ++i;
        } else if ((UTF8_3RD_ROW_NORMALISER & ss[0]) == UTF8_3RD_ROW and is_continuation_byte(ss[1]) and is_continuation_byte(ss[2])) {
            ++i;
            ++i;
        } else if ((UTF8_4TH_ROW_NORMALISER & ss[0]) == UTF8_4TH_ROW and is_continuation_byte(ss[1]) and is_continuation_byte(ss[2]) and is_continuation_byte(ss[3])) {
            ++i;
            ++i;
            ++i;
        } else {
            cerr << "UTF-8 decoding error\n";
        }
    }

    current_size = sz;

    return sz;
}

auto Text::at(size_type target) const -> Character {
    char ss[5];
    ss[4] = '\0';

    size_t sz = 0;

    for (std::remove_reference<decltype(text)>::type::size_type i = 0; sz <= target and i < text.size(); ++i, ++sz) {
        const auto each { text.at(i) };
        ss[0] = each;
        ss[1] = '\0';
        ss[2] = '\0';
        ss[3] = '\0';

        if ((UTF8_1ST_ROW_NORMALISER & each) == UTF8_1ST_ROW) {
            // do nothing
        } else if ((UTF8_2ND_ROW_NORMALISER & each) == UTF8_2ND_ROW) {
            ss[1] = text.at(i+1);
        } else if ((UTF8_3RD_ROW_NORMALISER & each) == UTF8_3RD_ROW) {
            ss[1] = text.at(i+1);
            ss[2] = text.at(i+2);
        } else if ((UTF8_4TH_ROW_NORMALISER & each) == UTF8_4TH_ROW) {
            ss[1] = text.at(i+1);
            ss[2] = text.at(i+2);
            ss[3] = text.at(i+3);
        }

        if ((UTF8_1ST_ROW_NORMALISER & ss[0]) == UTF8_1ST_ROW) {
            // do nothing
        } else if ((UTF8_2ND_ROW_NORMALISER & ss[0]) == UTF8_2ND_ROW and is_continuation_byte(ss[1])) {
            ++i;
        } else if ((UTF8_3RD_ROW_NORMALISER & ss[0]) == UTF8_3RD_ROW and is_continuation_byte(ss[1]) and is_continuation_byte(ss[2])) {
            ++i;
            ++i;
        } else if ((UTF8_4TH_ROW_NORMALISER & ss[0]) == UTF8_4TH_ROW and is_continuation_byte(ss[1]) and is_continuation_byte(ss[2]) and is_continuation_byte(ss[3])) {
            ++i;
            ++i;
            ++i;
        } else {
            cerr << "UTF-8 decoding error\n";
        }
    }

    return string { ss };
}

auto Text::str() const -> string {
    return text;
}

auto Text::operator [] (size_type i) const -> Character {
    return at(i);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "error: no string given\n";
        return 1;
    }
    const string s { argv[1] };
    cout << s.size() << endl;

    Text text { s };
    cout << text.size() << endl;
    cout << text.size() << endl;

    cout << text.at(6) << endl;
    cout << text[6] << endl;
    cout << text.at(7) << endl;

    cout << text << endl;

    return 0;
}
