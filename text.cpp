#include <cstdint>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
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
    vector<Character> text;

    public:

    using size_type = decltype(text)::size_type;

    auto size() const -> size_type;
    auto at(size_type) const -> Character;

    auto str() const -> string;

    auto operator [] (size_type) const -> Character;

    static auto parse(string) -> decltype(text);

    Text(string);
};

auto operator << (ostream& out, const Text& text) -> ostream& {
    out << text.str();
    return out;
}


Text::Text(string s): text(std::move(Text::parse(s))) {
}

static auto is_continuation_byte(uint8_t b) -> bool {
    return ((UTF8_FILLING_NORMALISER & b) == UTF8_FILLING);
}

auto Text::size() const -> size_type {
    return text.size();
}

auto Text::parse(string s) -> decltype(text) {
    vector<Character> parsed_text;

    char ss[5];
    ss[4] = '\0';

    for (decltype(s)::size_type i = 0; i < s.size(); ++i) {
        const auto each { s.at(i) };
        ss[0] = each;
        ss[1] = '\0';
        ss[2] = '\0';
        ss[3] = '\0';

        if ((UTF8_1ST_ROW_NORMALISER & each) == UTF8_1ST_ROW) {
            // do nothing
        } else if ((UTF8_2ND_ROW_NORMALISER & each) == UTF8_2ND_ROW) {
            ss[1] = s.at(i+1);
        } else if ((UTF8_3RD_ROW_NORMALISER & each) == UTF8_3RD_ROW) {
            ss[1] = s.at(i+1);
            ss[2] = s.at(i+2);
        } else if ((UTF8_4TH_ROW_NORMALISER & each) == UTF8_4TH_ROW) {
            ss[1] = s.at(i+1);
            ss[2] = s.at(i+2);
            ss[3] = s.at(i+3);
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
            throw std::domain_error(s);
        }

        parsed_text.emplace_back(ss);
    }

    return parsed_text;
}

auto Text::at(size_type target) const -> Character {
    return text.at(target);
}

auto Text::str() const -> string {
    ostringstream oss;
    for (const auto& each : text) {
        oss << each;
    }
    return oss.str();
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

    Text text { s };

    for (Text::size_type i = 0; i < text.size(); ++i) {
        cout << text.at(i) << endl;
    }

    cout << text << endl;

    return 0;
}
