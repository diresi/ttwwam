#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>

#include <algorithm>
#include <cctype>
#include <codecvt>
#include <functional>
#include <locale>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <sstream>
#include <vector>

using std::function;
using std::make_shared;
using std::map;
using std::pair;
using std::shared_ptr;
using std::string;
using std::stringstream;
using std::weak_ptr;
using std::wstring;
using std::wstringstream;
using std::vector;

const LPCWSTR WC_NAME = L"ttwwam-main-cls";
const LPCWSTR W_NAME = L"ttwwam-main";

static HWND hwndMain;
static HWND hwndInput;
static WNDPROC defaultInputEditProc;
static HWND hwndPreview;
static HWND hwndLog;

const DWORD ID_EDITPREVIEW = 100;
const DWORD ID_EDITLOG = 101;
const DWORD ID_EDITINPUT = 102;
const DWORD INPUTHEIGHT = 25;
const DWORD EN_USER_BASE = 0x8000;
const DWORD EN_USER_CONFIRM = EN_USER_BASE + 1;
const DWORD EN_USER_ABORT = EN_USER_BASE + 2;

// live previews:
// https://www.victorhurdugaci.com/fancy-windows-previewer

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

void clear_preview()
{
    int index = GetWindowTextLength(hwndPreview);
    SendMessage(hwndPreview, EM_SETSEL, 0, index);
    SendMessage(hwndPreview, EM_REPLACESEL, 0, (LPARAM) 0);
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

struct drect_t {
    double left;
    double top;
    double right;
    double bottom;
};

// quick declaration ... implementation follows
struct window_t {
    HWND hwnd;
    drect_t rect;
    wstring tostr() const;
};

struct monitor_t {
    HMONITOR hmon;
    HDC hdc;
    bool valid;
    MONITORINFOEX info;
    DWORD width;
    DWORD height;

    drect_t get_relative_window_rect(HWND hwnd)
    {
        RECT r;
        GetWindowRect(hwnd, &r);
        const RECT& rm = info.rcMonitor;
        DWORD w = rm.right - rm.left;
        DWORD h = rm.bottom - rm.top;

        r.left -= rm.left;
        r.right -= rm.left;
        r.top -= rm.top;
        r.bottom -= rm.top;

        drect_t dr;
        dr.left = static_cast<double>(r.left) / w;
        dr.right = static_cast<double>(r.right) / w;
        dr.top = static_cast<double>(r.top) / h;
        dr.bottom = static_cast<double>(r.bottom) / h;
        return dr;
    }

    RECT get_absolute_window_rect(const window_t& window)
    {
        const RECT& rm = info.rcMonitor;
        DWORD w = rm.right - rm.left;
        DWORD h = rm.bottom - rm.top;

        const drect_t& in = window.rect;
        RECT out;
        out.left = static_cast<LONG>(in.left * w + rm.left);
        out.right = static_cast<LONG>(in.right * w + rm.left);
        out.top = static_cast<LONG>(in.top * h + rm.top);
        out.bottom = static_cast<LONG>(in.bottom * h + rm.top);
        return out;
    }

    wstring tostr() const {
        wstring txt;
        txt.append(_w(info.rcMonitor));
        txt.append(L" HMONITOR=");
        txt.append(_w(hmon));
        txt.append(L", HDC=");
        txt.append(_w(hdc));
        txt.append(L", Name=");
        txt.append(info.szDevice);
        return txt;
    }
};

typedef map<HMONITOR, monitor_t> monitor_map_t;
static monitor_map_t _monitors;

typedef map<HMONITOR, HWND> tracker_t;
static tracker_t _trackers;


wstring get_window_title(HWND hwnd)
{
    // win32 insists to add null terminator :/
    int l = GetWindowTextLength(hwnd) + sizeof(TCHAR);
    vector<TCHAR> v;
    v.resize(l);
    GetWindowText(hwnd, &v[0], l);
    wstring title(v.begin(), v.end());
    title.erase(title.find(L'\0'));
    return title;
}

wstring get_edit_text(HWND hwnd)
{
    return get_window_title(hwnd);
}

template<>
wstring _w<drect_t>(drect_t r)
{
    stringstream ss;
    ss << "("
        << r.left << ","
        << r.top << ")-("
        << r.right << ","
        << r.bottom << ")";
    return s2w(ss.str());
}

monitor_t get_monitor_info(HMONITOR hmon)
{
    monitor_t mon = {hmon};
    mon.info.cbSize = sizeof(MONITORINFOEX);
    mon.valid = false;
    if (GetMonitorInfo(hmon, &mon.info)) {
        mon.valid = true;
        mon.width = mon.rcMonitor.right - mon.rcMonitor.left;
        mon.height = mon.rcMonitor.bottom - mon.rcMonitor.top;
    }
    return mon;
}

HMONITOR current_monitor_handle()
{
    POINT pt;
    GetCursorPos(&pt);
    return MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);
}

monitor_t current_monitor()
{
    return get_monitor_info(current_monitor_handle());
}

wstring window_t::tostr() const {
    wstring txt;
    txt.append(_w(rect));
    txt.append(L" ");
    txt.append(_w(current_monitor().get_absolute_window_rect(*this)));
    txt.append(L" ");
    txt.append(L" HWND=");
    txt.append(_w(hwnd));
    txt.append(L" ");
    txt.append(L" Title=");
    txt.append(get_window_title(hwnd));
    return txt;
}

// bool window_title_is(HWND hwnd, const wstring& title)
// {
//     if(GetWindowTextLength(hwnd) != title.size()) {
//         return false;
//     }
//     return get_window_title(hwnd) == title;
// }

typedef map<HWND, window_t> window_map_t;

struct container_t {
    wstring name;
    // weak_ptr<monitor_t> mon;
    window_map_t wmap;

    container_t(wstring name)
        : name(name)
    {}

    wstring tostr() const {
        wstring txt;
        txt.append(name);
        txt.append(NL);
        for(const auto& it: wmap)
        {
            txt.append(it.second.tostr());
            txt.append(NL);
        }
        return txt;
    }
};

typedef map<wstring, shared_ptr<container_t>> container_map_t;
static container_map_t _containers;

// typedef map<HMONITOR, weak_ptr<container_t>> monitor_map_t;
// static monitor_map_t _monitors;

shared_ptr<container_t> current_container()
{
    HMONITOR hmon = current_monitor_handle();
    // return _monitors[hmon].lock();
}

void show_hide_window(HWND hwnd, bool show)
{
    ShowWindow(hwnd, show ? SW_SHOW : SW_HIDE);
}

// bool is_program_manager(HWND hwnd)
// {
//     static const wstring title = L"Program Manager";
//     return window_title_is(hwnd, title);
// }

bool ignore_window(HWND hwnd)
{
    if (GetShellWindow() == hwnd) {
        return true;
    }
    if (hwnd == hwndMain) {
        return true;
    }
    if (!IsWindowVisible(hwnd)) {
        return true;
    }
    if (!GetWindowTextLength(hwnd)) {
        return true;
    }
    // if (is_program_manager(hwnd)) {
    //     return true;
    // }
    return false;
}

wstring new_container_name() {
    int i = 0;
    wstring name;
    if (_containers.empty()) {
        return L"main";
    }
    do {
        ++i;
        name =  L"cont " + _w(i);
    } while(_containers.find(name) != _containers.end());
    return name;
}

shared_ptr<container_t> new_container(wstring name=L"")
{
    if (name.empty()) {
        name = new_container_name();
    }
    if (_containers.find(name) != _containers.end()) {
        return shared_ptr<container_t>();
    }
    shared_ptr<container_t> c = make_shared<container_t>(name);
    _containers[name] = c;
    return c;
}

bool delete_container(shared_ptr<container_t> c)
{
    if (!c) {
        return false;
    }

    _containers.erase(c->name);
    return true;
}

HWND create_tracking_window(const monitor_t& mon)
{
    DWORD dwStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP;
    DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_TOPMOST;
    HWND hwnd = CreateWindowEx(
            dwExStyle,
            L"EDIT",
            NULL,
            dwStyle,
            mon.info.rcMonitor.left,
            mon.info.rcMonitor.top,
            mon.width,
            mon.height,
            NULL, NULL, NULL, NULL);
    wstring txt(L"tracking HMONITOR=");
    txt.append(_w(mon.hmon));
    txt.append(L" with HWND=");
    txt.append(_w(hwnd));
    log_debug(txt);
    return hwnd;
}

BOOL __stdcall EnumWindowProc(HWND hwnd, LPARAM lpar)
{
    if (ignore_window(hwnd)) {
        return TRUE;
    }

    HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
    if (!hmon) {
        return TRUE;
    }

    monitor_t mon = get_monitor_info(hmon);

    // const auto tit = _trackers.find(hmon);
    // if (tit == _trackers.end()) {
    //     _trackers[hmon] = create_tracking_window(mon);
    // }

    // monitor_map_t &mm = *reinterpret_cast<monitor_map_t*>(lpar);
    // shared_ptr<container_t> cont = mm[hmon].lock();
    // if (!cont) {
    //     cont = new_container();
    //     if (!cont) {
    //         return TRUE;
    //     }
    //     mm[hmon] = cont;
    // }

    // drect_t r = mon.get_relative_window_rect(hwnd);
    // window_t w = {hwnd, r};
    // cont->wmap[hwnd] = w;
    return TRUE;
}

bool show_hide_container(shared_ptr<container_t> c, bool show)
{
    if (!c) {
        return false;
    }

    for(const auto& we : c->wmap) {
        show_hide_window(we.second.hwnd, show);
    }
    return true;
}

void show_hide_current_container(bool show)
{
    show_hide_container(current_container(), show);
}

void update_trackers(tracker_t& trackers, monitor_map_t& monitors)
{
    // trying to associate monitors after changes to the display configuration
    // were made. probably doesn't handle resolution-only changes.
    for(auto it = trackers.cbegin(); it != trackers.end();) {
        HMONITOR hmon = MonitorFromWindow(it->second, MONITOR_DEFAULTTONULL);
        if (hmon != it->first) {
            // monitor was removed or re-enumerated, let's check if we have a
            // container to hide

            const auto mit = monitors.find(it->first);
            if (mit != monitors.end()) {
                if (hmon) {
                    // since we had a container displayed there, we move
                    // it to the new monitor (handle)
                    // _monitors[hmon] = mit->second;
                } else {
                    // the monitor is gone, hide the container
                    // show_hide_container(mit->second.lock(), false);
                }
            }

            if (!hmon) {
                // monitor was removed, destroy the tracker window
                DestroyWindow(it->second);
                trackers.erase(it++);
            }
        } else {
            ++it;
        }
    }

    // create trackers for new monitors
    for(auto it = monitors.cbegin(); it != monitors.end(); ++it) {
        if (trackers.find(it->first) == trackers.end()) {
            trackers[it->first] = create_tracking_window(it->second);
        }
    }
}

BOOL __stdcall MonitorEnumProc(HMONITOR hmon, HDC, LPRECT pR, LPARAM lpar)
{
    monitor_map_t& mm = *reinterpret_cast<tracker_t*>(lpar);
    monitor_t m = get_monitor_info(hmon);
    if (m.valid) {
        mm[hmon] = m;
    }
    return TRUE;
}

void scan_monitors()
{
    monitor_map_t tmp;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&tmp));
    update_trackers(_trackers, tmp);
    _monitors.swap(tmp);
}

// void scan_monitorsX()
// {
//     // trying to associate monitors after changes to the display configuration
//     // were made. probably doesn't handle resolution-only changes.
//     for(auto it = _trackers.cbegin(); it != _trackers.end();) {
//         HMONITOR hmon = MonitorFromWindow(it->second, MONITOR_DEFAULTTONULL);
//         if (hmon != it->first) {
//             // monitor was removed or re-enumerated, let's check if we have a
//             // container to hide

//             auto mit = _monitors.find(it->first);
//             if (mit != _monitors.end()) {
//                 if (hmon) {
//                     // since we had a container displayed there, we move
//                     // it to the new monitor (handle)
//                     _monitors[hmon] = mit->second;
//                 } else {
//                     // the monitor is gone, hide the container
//                     show_hide_container(mit->second.lock(), false);
//                 }
//             }

//             if (!hmon) {
//                 // monitor was removed, destroy the tracker window
//                 DestroyWindow(it->second);
//             }
//             // remove monitor and tracker entries
//             _monitors.erase(it->first);
//             _trackers.erase(it++);
//         } else {
//             ++it;
//         }
//     }

//     // should implicitly be covered by above loop
//     // for(auto it = _monitors.cbegin(); it != _monitors.cend();) {
//     //     if (!get_monitor_info(it->first).valid) {
//     //         _monitors.erase(it++);
//     //     } else {
//     //         ++it;
//     //     }
//     // }
// }

void scan_current_desktops()
{
    scan_monitors();
    EnumWindows(EnumWindowProc, reinterpret_cast<LPARAM>(&_monitors));
}

bool move_to_monitor(shared_ptr<container_t> c, HMONITOR hmon)
{
    if (!c || !hmon) {
        return false;
    }

    // drop container from another monitor where it's currently visible?
    for(auto it = _monitors.cbegin(); it != _monitors.end();) {
        shared_ptr<container_t> mc = it->second.lock();
        if (c == mc) {
            _monitors.erase(it++);
        } else {
            ++it;
        }
    }

    monitor_t mon = get_monitor_info(hmon);

    for(const auto& e: c->wmap) {
        const window_t& window = e.second;
        RECT r = mon.get_absolute_window_rect(window);
        MoveWindow(
                window.hwnd,
                static_cast<int>(r.left),
                static_cast<int>(r.top),
                static_cast<int>(r.right-r.left),
                static_cast<int>(r.bottom-r.top),
                TRUE);
    }

    _monitors[hmon] = c;
    return true;
}

bool move_to_current_monitor(shared_ptr<container_t> c)
{
    return move_to_monitor(c, current_monitor_handle());
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

bool show_main_window(HWND hwnd, bool show)
{
    show_hide_window(hwnd, show);
    if (!show) {
        return false;
    }

    scan_current_desktops();

    monitor_t m = current_monitor();
    log_debug(m.tostr());
    MoveWindow(
            hwnd,
            m.info.rcMonitor.left,
            m.info.rcMonitor.top,
            m.info.rcMonitor.right - m.info.rcMonitor.left,
            m.info.rcMonitor.bottom - m.info.rcMonitor.top,
            FALSE);
    SetWindowText(hwndInput, L"");
    SetFocus(hwndInput);
    SetForegroundWindow(hwnd);

    return true;
}

struct hotkey_t {
    SHORT modifier;
    SHORT keycode;
};

struct cmd_t {
    wstring cmd;
    vector<wstring> args;
};

struct cmd_spec_t {
    pair<bool, hotkey_t> hotkey;
    function<bool(HWND, cmd_t)> func;
};

bool cmd_quit_program(HWND hwnd, const cmd_t& cmd)
{
    PostQuitMessage(0);
    return true;
}


bool cmd_new_desktop(HWND hwnd, const cmd_t& cmd)
{
    scan_current_desktops();
    shared_ptr<container_t> c = current_container();
    show_hide_container(c, false);
    if (!c || !c->wmap.empty()) {
        c = new_container();
        _monitors[current_monitor_handle()] = c;
    }
    show_main_window(hwnd, false);
    return true;
}

bool switch_to_desktop(HWND hwnd, const wstring& name)
{
    log_debug(wstring(L"Switching to container ") + name);
    shared_ptr<container_t> current = current_container();
    if (current && (name == current->name)) {
        log_debug(L"oh, we're already on the correct container");
        return true;
    }

    shared_ptr<container_t> next;
    auto it = _containers.find(name);
    if (it == _containers.end()) {
        log_debug(L"container not found, creating a new one");
        next = new_container(name);
    } else {
        next = it->second;
    }

    if (!next) {
        log_debug(wstring(L"container not found: ") + name);
        return false;
    }

    scan_current_desktops();
    show_hide_container(current, false);
    move_to_current_monitor(next);

    if (current && current->wmap.empty()) {
        delete_container(current);
    }
    show_hide_container(next, true);
    return true;
}

bool cmd_switch_to_desktop(HWND hwnd, const cmd_t& cmd)
{
    wstring name = join_strings(cmd.args);
    return switch_to_desktop(hwnd, name);
}

bool cmd_show_main_window(HWND hwnd, const cmd_t& cmd)
{
    show_main_window(hwnd, true);
    // return false to prevent immediately getting closed again ;)
    shared_ptr<container_t> c = current_container();
    if (c) {
        log_debug(wstring(L"current container = ") + c->name + L" with " + _w(c->wmap.size()) + L" windows");
    }
    return false;
}

bool cmd_rename_current_container(HWND hwnd, const cmd_t& cmd)
{
    wstring name = join_strings(cmd.args);
    if (name.empty()) {
        return false;
    }
    shared_ptr<container_t> c = current_container();
    if (!c) {
        return false;
    }
    if (_containers.find(name) != _containers.end()) {
        return false;
    }
    _containers.erase(c->name);
    c->name = name;
    _containers[name] = c;
    return true;
}

bool cmd_scan_desktops(HWND hwnd, const cmd_t cmd)
{
    scan_current_desktops();
    return false;
}

bool cmd_kill_windows(HWND hwnd, const cmd_t cmd)
{
    shared_ptr<container_t> c = current_container();
    if (!c) {
        return false;
    }
    for(const auto& w: c->wmap) {
        PostMessage(w.second.hwnd, WM_CLOSE, 0, 0);
    }

    return true;
}

bool cmd_delete_desktop(HWND hwnd, const cmd_t cmd)
{
    shared_ptr<container_t> c = current_container();
    show_hide_container(c, true);
    return delete_container(c);
}

bool cmd_info(HWND hwnd, const cmd_t cmd)
{
    for(const auto& it: _trackers) {
        wstring txt(L"tracking HWND=");
        txt.append(_w(it.second));
        txt.append(L" was on HMONITOR=");
        txt.append(_w(it.first));
        txt.append(L" is now on HMONITOR=");
        txt.append(_w(MonitorFromWindow(it.second, MONITOR_DEFAULTTONULL)));
        log_debug(txt);
    }
    for(const auto& it: _monitors)
    {
        monitor_t m = get_monitor_info(it.first);
        log_debug(m.tostr());
    }
    for(const auto& it: _containers)
    {
        log_debug(it.second->tostr());
    }
    return false;
}

const map<wstring, cmd_spec_t> _commands = {
    {L":show_main_window", {{true, {MOD_CONTROL | MOD_NOREPEAT, VK_UP}}, cmd_show_main_window}},
    {L":quit", {{false, {}}, cmd_quit_program}},
    {L":new", {{false, {}}, cmd_new_desktop}},
    {L":switch", {{false, {}}, cmd_switch_to_desktop}},
    {L":rename", {{false, {}}, cmd_rename_current_container}},
    {L":scan", {{false, {}}, cmd_scan_desktops}},
    {L":kill", {{false, {}}, cmd_kill_windows}},
    {L":release", {{false, {}}, cmd_delete_desktop}},
    {L":info", {{false, {}}, cmd_info}},
};

cmd_t split_command(wstring scmd)
{
    trim(scmd);
    cmd_t cmd;
    if (scmd.empty()) {
        return cmd;
    }
    std::wregex regex{LR"([\s,]+)"}; // split on space and comma
    std::wsregex_token_iterator it{scmd.begin(), scmd.end(), regex, -1};
    cmd.cmd = *it;
    cmd.args = {++it, {}};
    return cmd;
}

bool update_preview(HWND hwnd, wstring scmd)
{
    cmd_t cmd = split_command(scmd);
    clear_preview();
    for(const auto& e: _containers) {
        if (e.first.find(cmd.cmd) != wstring::npos) {
            log_info(e.first);
        } else {
            for(const auto& w: e.second->wmap) {
                wstring t = get_window_title(w.second.hwnd);
                if (t.find(cmd.cmd) != wstring::npos) {
                    log_info(e.first + L" - " + t);
                }
            }
        }
    }
    for(const auto& e: _commands) {
        if (e.first.find(cmd.cmd) != wstring::npos) {
            log_info(e.first);
        }
    }
    return false;
}

bool run_command(HWND hwnd, wstring scmd)
{
    cmd_t cmd = split_command(scmd);
    if (cmd.cmd.empty()) {
        show_main_window(hwnd, false);
        return true;
    }
    log_debug(scmd);
    auto it = _commands.find(cmd.cmd);

    bool hide = false;
    if (it == _commands.end()) {
        log_debug(L"command not found");
        if (!scmd.empty() && (scmd[0] != L':')) {
            hide = switch_to_desktop(hwnd, scmd);
        }
    } else {
        hide = it->second.func(hwnd, cmd);
    }
    if (hide) {
        show_main_window(hwnd, false);
        return true;
    }
    return false;
}

bool handle_hotkey(HWND hwnd, SHORT modifiers, SHORT keycode) {
    for(const auto& cmd : _commands) {
        if (!cmd.second.hotkey.first) {
            continue;
        }
        const hotkey_t& hk = cmd.second.hotkey.second;
        SHORT mod = hk.modifier & ~MOD_NOREPEAT;
        if ((mod == modifiers) && (hk.keycode == keycode)) {
            return run_command(hwnd, cmd.first);
        }
    }
    return false;
}

LRESULT CALLBACK myInputEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CHAR:
            {
                if (wParam == 1) {
                    // CTRL+A
                    SendMessage(hwnd, EM_SETSEL, 0, -1);
                    return 0;
                }

                DWORD cmd = 0;
                if (wParam == VK_ESCAPE) {
                    cmd = EN_USER_ABORT;
                } else if (wParam == VK_RETURN) {
                    cmd = EN_USER_CONFIRM;
                }
                if (cmd) {
                    SendMessage(
                            hwndMain,
                            WM_COMMAND,
                            MAKEWPARAM(ID_EDITINPUT, cmd),
                            reinterpret_cast<LPARAM>(hwnd));
                    return 0;
                }
            }

        default:
            return CallWindowProc(defaultInputEditProc, hwnd, msg, wParam, lParam);
    }
    return 0;
}

void create_gui(HWND hwnd)
{
    hwndInput = CreateWindowEx(
            0, L"EDIT", NULL,
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOVSCROLL,
            0, 0, 0, 0,
            hwnd,
            reinterpret_cast<HMENU>(ID_EDITINPUT),
            reinterpret_cast<HINSTANCE>(GetWindowLong(hwnd, GWL_HINSTANCE)),
            NULL);
    defaultInputEditProc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtr(
                hwndInput,
                GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(myInputEditProc)));

    hwndPreview = CreateWindowEx(
            0, L"EDIT", NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
            0, 0, 0, 0,
            hwnd,
            reinterpret_cast<HMENU>(ID_EDITPREVIEW),
            reinterpret_cast<HINSTANCE>(GetWindowLong(hwnd, GWL_HINSTANCE)),
            NULL);
    SendMessage(hwndPreview, EM_SETREADONLY, TRUE, 0);

    hwndLog = CreateWindowEx(
            0, L"EDIT", NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
            0, 0, 0, 0,
            hwnd,
            reinterpret_cast<HMENU>(ID_EDITLOG),
            reinterpret_cast<HINSTANCE>(GetWindowLong(hwnd, GWL_HINSTANCE)),
            NULL);
    SendMessage(hwndLog, EM_SETREADONLY, TRUE, 0);

}

LRESULT CALLBACK WindowProc(
  _In_ HWND   hwnd,
  _In_ UINT   uMsg,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam
)
{
    switch(uMsg) {
        case WM_CREATE:
            create_gui(hwnd);
            return 0;

        case WM_SIZE:
            {
                WORD w = LOWORD(lParam);
                WORD whalf = w/2;
                WORD wrest = w - whalf - 1;
                WORD hrest = HIWORD(lParam) - INPUTHEIGHT;
                MoveWindow(hwndInput, 0, 0, w, INPUTHEIGHT, TRUE);
                MoveWindow(hwndPreview, 0, INPUTHEIGHT, whalf, hrest, TRUE);
                MoveWindow(hwndLog, whalf+1, INPUTHEIGHT, wrest, hrest, TRUE);
            }
            return 0;

        case WM_HOTKEY:
            if (handle_hotkey(hwnd, LOWORD(lParam), HIWORD(lParam))) {
                return 0;
            }
            break;

        // case WM_DISPLAYCHANGE:
        //     {
        //         DWORD depth = wParam;
        //         WORD w = LOWORD(lParam);
        //         WORD h = HIWORD(lParam);
        //         log_debug(wstring(L"WM_DISPLAYCHANGE ") + _w(depth) + L"(" + _w(w) + L"," + _w(h) + L")");
        //     }
        //     break;

        case WM_COMMAND:
            if (LOWORD(wParam) != ID_EDITINPUT) {
                break;
            }
            switch HIWORD(wParam) {
                case EN_CHANGE:
                    update_preview(hwnd, get_edit_text(reinterpret_cast<HWND>(lParam)));
                    return 0;

                case EN_USER_ABORT:
                    show_main_window(hwnd, false);
                    return 0;

                case EN_USER_CONFIRM:
                    run_command(hwnd, get_edit_text(reinterpret_cast<HWND>(lParam)));
                    return 0;
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    WNDCLASSEX wce = {
        sizeof(wce),
        0,
        WindowProc,
        0,
        0,
        hInstance,
        LoadIcon(NULL, IDI_APPLICATION),
        LoadCursor(NULL, IDC_ARROW),
        reinterpret_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH)),
        NULL,
        WC_NAME,
        NULL,
    };

    ATOM wc = RegisterClassEx(&wce);
    if (!wc) {
        log_error(L"failed to register window class");
        return -1;
    }

    DWORD dwStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP;
    DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_TOPMOST;
    hwndMain = CreateWindowEx(
            dwExStyle,
            MAKEINTATOM(wc),
            W_NAME,
            dwStyle,
            0, 0, 800, 600,
            NULL, NULL, NULL, NULL);
    if (!hwndMain) {
        MessageBox(NULL, get_last_error_message().c_str(), L"", MB_OK);
        return -2;
    }
    log_debug(L"main window created");


    for(const auto& c : _commands) {
        const cmd_spec_t& cmd = c.second;
        if (!cmd.hotkey.first) {
            continue;
        }
        const hotkey_t& hk = cmd.hotkey.second;
        if (!RegisterHotKey(hwndMain, 1, hk.modifier, hk.keycode)) {
            wstring err = L"Failed to register hotkey for ";
            err += c.first + L": " + get_last_error_message();
            MessageBox(hwndMain, err.c_str(), L"", MB_OK);
            return -3;
        }
    }

    BOOL ok;
    MSG msg = {0};
    while((ok = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (ok == -1) {
            // idk, maybe exit?
            // log_error(L"exit?");
            break;
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    for(const auto& c : _containers) {
        show_hide_container(c.second, true);
    }

    return msg.wParam;
}
