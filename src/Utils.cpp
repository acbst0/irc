#include "../include/libs.hpp"

static std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

void parseIrc(const std::string& line, std::string& cmd, std::vector<std::string>& params, std::string& trailing) 
{
    std::string s = line;
    
    // bu kısım olmadan if içerisine girmiyor komut. düzeltildi
    size_t end = s.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
        s = s.substr(0, end + 1);
    } else {
        s.clear();
    }
    
    if (!s.empty() && s[0]==':') {
        size_t sp = s.find(' ');
        s = (sp==std::string::npos) ? "" : s.substr(sp+1);
    }
    size_t colon = s.find(" :");
    if (colon != std::string::npos) {
        trailing = s.substr(colon+2);
        s = s.substr(0, colon);
    } else trailing.clear();
    std::vector<std::string> p = split(s, ' ');
    cmd = p.empty() ? "" : p[0];
    for (size_t i=1;i<p.size();++i) if (!p[i].empty()) params.push_back(p[i]);
}

void enqueue(std::string &outbuf, const std::string& line)
{
    outbuf += line;
    if (outbuf.size() > 1<<20) outbuf.erase(0, outbuf.size() - (1<<20));
}