#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include "string.h"
using namespace std;


namespace str {
    bool startswith(const std::string& s, const std::string& w) {
        /*  Returns true if s stars with w.
         */
        return (s.compare(0, w.length(), w) == 0);
    }

    bool startswithchunk(const std::string& s, const std::string& w) {
        /*  Returns true if s stars with chunk w.
         */
        return (chunk(s) == w);
    }

    bool endswith(const std::string& s, const std::string& w) {
        /*  Returns true if s ends with w.
         */
        return (s.compare(s.length()-w.length(), s.length(), w) == 0);
    }


    bool isnum(const std::string& s, bool negatives) {
        /*  Returns true if s contains only numerical characters.
         *  Regex equivalent: `^[0-9]+$`
         */
        bool num = false;
        unsigned start = 0;
        if (s[0] == '-' and negatives) {
            // must handle negative numbers
            start = 1;
        }
        for (unsigned i = start; i < s.size(); ++i) {
            switch (s[i]) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    num = true;
                    break;
                default:
                    num = false;
            }
            if (!num) break;
        }
        return num;
    }

    bool isfloat(const std::string& s, bool negatives) {
        /*  Returns true if s contains only numerical characters.
         *  Regex equivalent: `^[0-9]+\.[0-9]+$`
         */
        bool is = false;
        unsigned start = 0;
        if (s[0] == '-' and negatives) {
            // to handle negative numbers
            start = 1;
        }
        int dot = -1;
        for (unsigned i = start; i < s.size(); ++i) {
            if (s[i] == '.') {
                dot = (int)i;
                break;
            }
        }
        is = isnum(sub(s, 0, dot), negatives) and isnum(sub(s, dot+1));
        return is;
    }


    string sub(const string& s, int b, int e) {
        /*  Returns substring of s.
         *  If only s is passed, returns copy of s.
         */
        if (b == 0 and e == -1) return string(s);

        ostringstream part;
        part.str("");

        unsigned end;
        if (e < 0) { end = unsigned(s.size() + e + 1); }
        else { end = unsigned(e); }

        for (unsigned i = unsigned(b); i < s.size() and i < end; ++i) {
            part << s[i];
        }

        return part.str();
    }


    string chunk(const string& s, bool ignore_leading_ws) {
        /*  Returns part of the string until first whitespace from left side.
         */
        ostringstream chnk;

        string str = (ignore_leading_ws ? lstrip(s) : s);

        for (unsigned i = 0; i < str.size(); ++i) {
            if (str[i] == *" " or str[i] == *"\t" or str[i] == *"\v" or str[i] == *"\n") break;
            chnk << str[i];
        }
        return chnk.str();
    }

    vector<string> chunks(const string& s) {
        /*  Returns chunks of string.
         */
        vector<string> chnks;
        string tmp(lstrip(s));
        string chnk;
        while (tmp.size()) {
            chnk = chunk(tmp);
            tmp = lstrip(sub(tmp, chnk.size()));
            chnks.push_back(chnk);
        }
        return chnks;
    }


    string extract(const string& s) {
        /*  Extracts *enquoted chunk*.
         *  It is particularly useful if you have a string encoded in another string.
         *
         *  Consider this string: `"Hello 'Beautiful' World!" some other (42) things;`.
         *  str::extract(<that string>) will return `"Hello 'Beautiful' World!"`.
         */
        ostringstream chnk;

        char quote = s[0];
        int backs = 0;

        for (unsigned i = 0; i < s.size(); ++i) {
            chnk << s[i];
            if (s[i] == quote and i > 0 and (backs % 2 == 0)) { break; }
            if (s[i] == '\\') { ++backs; }
            if (s[i] == quote) { backs = 0; }
        }

        return chnk.str();
    }


    string lstrip(const string& s) {
        /*  Removes whitespace from left side of the string.
         */
        unsigned i = 0;
        while (i < s.size()) {
            if (s[i] == *" " or s[i] == *"\t" or s[i] == *"\v" or s[i] == *"\n") {
                ++i;
            } else {
                break;
            }
        }
        return sub(s, i);
    }


    unsigned lshare(const string& s, const string& w) {
        unsigned share = 0;
        for (unsigned i = 0; i < s.size() and i < w.size(); ++i) {
            if (s[i] == w[i]) {
                ++share;
            } else {
                break;
            }
        }
        return share;
    }


    string enquote(const string& s) {
        /** Enquote the string.
         */
        ostringstream encoded;
        char closing = '"';

        encoded << closing;
        for (unsigned i = 0; i < s.size(); ++i) {
            if (s[i] == closing) { encoded << "\\"; }
            encoded << s[i];
        }
        encoded << closing;

        return encoded.str();
    }
}
