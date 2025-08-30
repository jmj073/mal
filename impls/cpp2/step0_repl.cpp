#include <iostream>
#include <string>

using namespace std;

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

int main() {
    cin.tie(0)->sync_with_stdio(0);

    const string prompt = "user> ";

    while (true) {
        cout << prompt << flush;
        
        string in;
        getline(cin, in);

        if (!cin) break;

        cout << rep(in) << endl;
    }
}
