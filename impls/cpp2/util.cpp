#include "util.h"

std::string regex_replace_callback(
    const std::string& input,
    const std::regex& re,
    const std::function<std::string(const std::smatch&)>& callback
) {
    std::string result;
    std::size_t lastPos = 0;

    auto begin = std::sregex_iterator(input.begin(), input.end(), re);
    auto end   = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        const std::smatch& match = *it;

        // 매치 전 텍스트 추가
        result.append(input, lastPos, match.position() - lastPos);

        // 콜백으로 변환된 결과 추가
        result.append(callback(match));

        // 마지막 처리 위치 갱신
        lastPos = match.position() + match.length();
    }

    // 남은 뒷부분 추가
    result.append(input, lastPos);

    return result;
}
