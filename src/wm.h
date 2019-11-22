#ifndef __TTWWAM_WM__
#define __TTWWAM_WM__

#include <windows.h>

#include <map>
#include <memory>
#include <string>

#include "log.h"
#include "ui.h"

struct drect_t {
    double left;
    double top;
    double right;
    double bottom;
};

struct window_t {
    HWND hwnd;
    drect_t rect;
    std::wstring tostr() const;
};

struct monitor_t {
    HMONITOR hmon;
    HDC hdc;
    bool valid;
    MONITORINFOEX info;

    drect_t get_relative_window_rect(HWND hwnd) const;
    RECT get_absolute_window_rect(const window_t& window) const;
    std::wstring tostr() const;
};
typedef std::map<HWND, window_t> window_map_t;

struct container_t {
    std::wstring name;
    window_map_t wmap;

    container_t(std::wstring name);
    std::wstring tostr() const;
};

typedef std::map<std::wstring, std::shared_ptr<container_t>> container_map_t;

typedef std::map<HMONITOR, std::weak_ptr<container_t>> monitor_map_t;

std::shared_ptr<container_t> current_container();
void scan_current_desktops();

class WindowManager {
    public:
        WindowManager(sp_ui_t, sp_log_t);
        void show_all_containers() const;
};

typedef std::shared_ptr<WindowManager> sp_wm_t;

sp_wm_t init_wm(sp_ui_t ui, sp_log_t log);

#endif // __TTWWAM_WM__
