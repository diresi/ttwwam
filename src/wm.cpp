#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>

#include "wm.h"
#include "ui.h"

using std::wstring;

static container_map_t _containers;
static monitor_map_t _monitors;

drect_t monitor_t::get_relative_window_rect(HWND hwnd) const
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

RECT monitor_t::get_absolute_window_rect(const window_t& window) const
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

wstring monitor_t::tostr() const {
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
    mon.valid = true;
    if (!GetMonitorInfo(hmon, &mon.info)) {
        mon.valid = false;
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

container_t::container_t(wstring name)
    : name(name)
{}

wstring container_t::tostr() const {
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

shared_ptr<container_t> current_container()
{
    HMONITOR hmon = current_monitor_handle();
    return _monitors[hmon].lock();
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

map<HMONITOR, HWND> _trackers;

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
            0, 0,
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

    const auto tit = _trackers.find(hmon);
    if (tit == _trackers.end()) {
        _trackers[hmon] = create_tracking_window(mon);
    }

    monitor_map_t &mm = *reinterpret_cast<monitor_map_t*>(lpar);
    shared_ptr<container_t> cont = mm[hmon].lock();
    if (!cont) {
        cont = new_container();
        if (!cont) {
            return TRUE;
        }
        mm[hmon] = cont;
    }

    drect_t r = mon.get_relative_window_rect(hwnd);
    window_t w = {hwnd, r};
    cont->wmap[hwnd] = w;
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

void scan_monitors()
{
    // trying to associate monitors after changes to the display configuration
    // were made. probably doesn't handle resolution-only changes.
    for(auto it = _trackers.cbegin(); it != _trackers.end();) {
        HMONITOR hmon = MonitorFromWindow(it->second, MONITOR_DEFAULTTONULL);
        if (hmon != it->first) {
            // monitor was removed or re-enumerated, let's check if we have a
            // container to hide

            auto mit = _monitors.find(it->first);
            if (mit != _monitors.end()) {
                if (hmon) {
                    // since we had a container displayed there, we move
                    // it to the new monitor (handle)
                    _monitors[hmon] = mit->second;
                } else {
                    // the monitor is gone, hide the container
                    show_hide_container(mit->second.lock(), false);
                }
            }

            if (!hmon) {
                // monitor was removed, destroy the tracker window
                DestroyWindow(it->second);
            }
            // remove monitor and tracker entries
            _monitors.erase(it->first);
            _trackers.erase(it++);
        } else {
            ++it;
        }
    }

    // should implicitly be covered by above loop
    // for(auto it = _monitors.cbegin(); it != _monitors.cend();) {
    //     if (!get_monitor_info(it->first).valid) {
    //         _monitors.erase(it++);
    //     } else {
    //         ++it;
    //     }
    // }
}

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

sp_wm_t init_wm(sp_ui_t ui, sp_log_t log);
{
    return std::make_shared<WindowManager>(ui, log);
}

WindowManager::WindowManager(sp_ui_t ui, sp_log_t log)
    : _ui(ui)
      , _log(log)
{
}

void WindowManager::show_all_containers() const
{
    for(const auto& c : _containers) {
        show_hide_container(c.second, true);
    }
}
