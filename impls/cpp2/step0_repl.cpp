#include <iostream>
#include <string>

#include "readline.h"

using namespace std;

string READ(const string& in);
string EVAL(const string& expr);
string PRINT(const string& value);
string rep(const string& in);

static ReadLine read_line("~/.mal-history");

int main() {
    const string prompt = "user> ";
    string input;

    while (read_line.get(prompt, input)) {
        cout << rep(input) << endl;
    }
}

string READ(const string& in) {
    return in;
}

string EVAL(const string& expr) {
    return expr;
}

string PRINT(const string& value) {
    return value;
}

string rep(const string& in) {
    return PRINT(EVAL(READ(in)));
}
