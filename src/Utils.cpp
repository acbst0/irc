#include "../include/libs.hpp"

// Yardımcı trim fonksiyonu
static inline std::string trim(const std::string& s) {
    size_t start = 0;
    size_t end = s.size();
    while (start < end && (s[start] == ' ' || s[start] == '\r' || s[start] == '\n'))
        start++;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\r' || s[end - 1] == '\n'))
        end--;
    return s.substr(start, end - start);
}

static std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        item = trim(item);
        if (!item.empty())
            elems.push_back(item);
    }
    return elems;
}

void parseIrc(const std::string& line, std::string& cmd, std::vector<std::string>& params, std::string& trailing) 
{
    std::string s = line;
    if (!s.empty() && s[0] == ':') {
        size_t sp = s.find(' ');
        s = (sp == std::string::npos) ? "" : s.substr(sp + 1);
    }
    size_t colon = s.find(" :");
    if (colon != std::string::npos) {
        trailing = trim(s.substr(colon + 2));
        s = s.substr(0, colon);
    } else {
        trailing.clear();
    }

    std::vector<std::string> p = split(s, ' ');
    cmd = p.empty() ? "" : trim(p[0]);
    for (size_t i = 1; i < p.size(); ++i) {
        std::string param = trim(p[i]);
        if (!param.empty())
            params.push_back(param);
    }
}

void enqueue(std::string &outbuf, const std::string& line)
{
    outbuf += line;
    if (outbuf.size() > 1<<20) outbuf.erase(0, outbuf.size() - (1<<20));
}