
#include <string>

#ifdef _UNICODE
namespace std {
    using tstring = wstring;
}
#else
namespace std {
    using tstring = string;
}
}
#endif // _UNICODE

#ifdef _UNICODE
std::wstring Ansi2Unicode(const std::string & ansiStr);
std::string Unicode2Ansi(const std::wstring & unicodeStr);
#else
std::string Ansi2Unicode(const std::string & str)
std::string Unicode2Ansi(const std::string & str);
#endif // _UNICODE
