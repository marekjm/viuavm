#include <viua/support/env.h>
using namespace std;

namespace support {
    namespace env {
        vector<string> getpaths(const string& var) {
            const char* VAR = getenv(var.c_str());
            vector<string> paths;

            if (VAR == 0) {
                // return empty vector as environment does not
                // set requested variable
                return paths;
            }

            string PATH = string(VAR);

            string path;
            unsigned i = 0;
            while (i < PATH.size()) {
                if (PATH[i] == ':') {
                    if (path.size()) {
                        paths.push_back(path);
                        path = "";
                        ++i;
                    }
                }
                path += PATH[i];
                ++i;
            }
            if (path.size()) {
                paths.push_back(path);
            }

            return paths;
        }

        bool isfile(const string& path) {
            struct stat sf;

            // not a file if stat returned error
            if (stat(path.c_str(), &sf) == -1) return false;
            // not a file if S_ISREG() macro returned false
            if (not S_ISREG(sf.st_mode)) return false;

            // file otherwise
            return true;
        }
    }
}
