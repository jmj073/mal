#include <iostream>
#include <string>

#include "readline.h"
#include "reader.h"
#include "printer.h"

using namespace std;

MalType READ(const string& in);
MalType EVAL(const MalType& ast);
string PRINT(const MalType& ast);
string rep(const string& in);

static ReadLine read_line("~/.mal-history");

int main() {
    const string prompt = "user> ";
    string input;

    while (read_line.get(prompt, input)) {
        try {
            cout << rep(input) << endl;
        } catch (MalSyntaxError& e) {
            cerr << e.what() << endl;
        } catch(MalInvalidHashmapKey& e) {
            cerr << e.what() << endl;
        } catch (MalNoToken& e) {
        }
    }
}

MalType READ(const string& in) {
    return read_str(in);
}

MalType EVAL(const MalType& ast) {
    return ast;
}

string PRINT(const MalType& ast) {
    return pr_str(ast, true);
}

string rep(const string& in) {
    return PRINT(EVAL(READ(in)));
}
