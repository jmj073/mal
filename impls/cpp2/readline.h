#ifndef _MY_READLINE_H_
#define _MY_READLINE_H_

#include <string>
#include <filesystem>

class ReadLine {
public:
    ReadLine(const std::filesystem::path& history_path);
    ~ReadLine();

    bool get(const std::string& prompt, std::string& out);

private:
    std::filesystem::path m_history_path;
};

#endif // _MY_READLINE_H_
