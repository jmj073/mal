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
