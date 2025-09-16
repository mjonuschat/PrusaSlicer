#include "Slic3r/Domain/Slicing.hpp"
#include <span>


namespace Slic3r::Domain::Slicing {

std::string join(const std::vector<std::string>& texts, const std::string& delimiter)
{
    if (texts.empty()) {
        return {};
    }

    std::string result{texts.front()};
    for (const std::string& text : std::span{texts}.subspan(1)) {
        result += delimiter;
        result += text;
    }
    return result;
}

std::ostream& operator<<(std::ostream& output, const Error& error) {
    output << "(";
    output << int(error.code);
    output << ", [";
    output << join(error.item_keys, ", ");
    output << "])";
    return output;
}

std::ostream& operator<<(std::ostream& output, const Progress& progress) {
    output << "(";
    output << progress.progress.value;
    output << "%, ";
    output << int(progress.progress_info);
    output << ")";
    return output;
}

}
