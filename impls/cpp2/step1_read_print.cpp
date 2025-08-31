#include <iostream>
#include <string>

#include "readline.h"
#include "reader.h"
#include "printer.h"

using namespace std;

unique_ptr<MalType> READ(const string& in);
unique_ptr<MalType> EVAL(unique_ptr<MalType> ast);
string PRINT(unique_ptr<MalType> ast);
string rep(const string& in);

static ReadLine read_line("~/.mal-history");

int main() {
    const string prompt = "user> ";
    string input;

    while (read_line.get(prompt, input)) {
        cout << rep(input) << endl;
    }
}

unique_ptr<MalType> READ(const string& in) {
    return read_str(in);
}

unique_ptr<MalType> EVAL(unique_ptr<MalType> ast) {
    return ast;
}

string PRINT(unique_ptr<MalType> ast) {
    return pr_str(ast.get());
}

string rep(const string& in) {
    return PRINT(EVAL(READ(in)));
}
