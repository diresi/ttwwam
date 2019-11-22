#define WIN32_LEAN_AND_MEAN
#define UNICODE

#include <memory>
#include "commands.h"
#include "wm.h"

using std::shared_ptr;

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

bool init_cmds() {
    for(const auto& c : _commands) {
        const cmd_spec_t& cmd = c.second;
        if (!cmd.hotkey.first) {
            continue;
        }
        const hotkey_t& hk = cmd.hotkey.second;
        if (!RegisterHotKey(hwndMain, 1, hk.modifier, hk.keycode)) {
            wstring err = L"Failed to register hotkey for ";
            err += c.first + L": " + get_last_error_message();
            log_fatal_error(err);
            return false;
        }
    }
    return true;
}
