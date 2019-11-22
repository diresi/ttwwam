#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>


const LPWSTR NL = L"\r\n";

string w2s(const wstring &var)
{
   static std::locale loc("");
   auto &facet = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(loc);
   return std::wstring_convert<std::remove_reference<decltype(facet)>::type, wchar_t>(&facet).to_bytes(var);
}

wstring s2w(const string &var)
{
   static std::locale loc("");
   auto &facet = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(loc);
   return std::wstring_convert<std::remove_reference<decltype(facet)>::type, wchar_t>(&facet).from_bytes(var);
}

// trim from start (in place)
static inline void ltrim(wstring &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(wstring &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(wstring &s) {
    ltrim(s);
    rtrim(s);
}

wstring join_strings(const vector<wstring>& parts, wstring delim=L" ")
{
    wstringstream ss;
    bool first = true;
    for(auto it = parts.begin(), eit=parts.end(); it != eit; ++it) {
        if (first) {
            first = false;
        } else {
            ss << delim;
        }
        ss << *it;
    }
    return ss.str();
}

wstring get_last_error_message()
{
    DWORD err = GetLastError();
    LPWSTR messageBuffer = nullptr;
    size_t size = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&messageBuffer,
            0,
            NULL);

    wstring msg(messageBuffer, size);
    LocalFree(messageBuffer);
    return msg;
}


void log_error(LPCWSTR txt)
{
    wstring err = get_last_error_message();
    wstring msg = txt;
    msg += L": ";
    msg += err;
    log(msg, hwndLog);
}

void append_log(const wstring& txt, HWND hwnd)
{
    int index = GetWindowTextLength(hwnd);
    SendMessage(hwnd, EM_SETSEL, index, index);
    SendMessage(hwnd, EM_REPLACESEL, 0, (LPARAM) txt.c_str());
}

void log(const wstring& msg, HWND hwnd)
{
    append_log(msg, hwnd);
    append_log(NL, hwnd);
}

void log_debug(LPCWSTR txt)
{
    log(txt, hwndLog);
}
void log_debug(const wstring& txt)
{
    log(txt, hwndLog);
}

void log_info(const wstring& txt)
{
    log(txt, hwndPreview);
}

template<typename T>
wstring _w(T in)
{
    stringstream ss;
    ss << in;
    return s2w(ss.str());
}
template<>
wstring _w<RECT>(RECT r)
{
    stringstream ss;
    ss << "("
        << r.left << ","
        << r.top << ")-("
        << r.right << ","
        << r.bottom << ")";
    return s2w(ss.str());
}

void log_fatal_error(const wstring& msg) {
    // MessageBox(hwndMain, err.c_str(), L"", MB_OK);
    Logger::fatal_error(msg);
}

sp_log_t init_log(sp_ui_t ui) {
    return std::make_shared<Logger>(ui);
}

Logger::Logger(sp_ui_t ui)
    : _ui(ui)
{
}
