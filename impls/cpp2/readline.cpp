#include <cstdlib>
#include <readline/readline.h>
#include <readline/history.h>
#include <readline/tilde.h>

#include "readline.h"

using namespace std;
using namespace filesystem;

string copyNfree(char* str) {
    string ret = str;
    free(str);
    return ret;
}

ReadLine::ReadLine(const path& history_path)
    : m_history_path(copyNfree(tilde_expand(history_path.c_str())))
{
    read_history(m_history_path.c_str());
}

ReadLine::~ReadLine() {
}

bool ReadLine::get(const string& prompt, string& out) {
    char* line = readline(prompt.c_str());
    if (line == NULL) {
        return false;
    }

    add_history(line);
    append_history(1, m_history_path.c_str());

    out = line;
    free(line);

    return true;
}
