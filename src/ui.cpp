#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>

#include <memory>

#include "log.h"
#include "ui.h"

// fwd declaration(s)
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK InputEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

sp_ui_t init_ui(HINSTANCE hInstance)
{
    auto ui = std::make_shared<UserInterface>(hInstance);
    if(!ui->register_window_class()) {
        log_fatal_error(L"failed to register window class");
        return sp_ui_t();
    }
    if(!ui->create_main_window()) {
        log_fatal_error(L"failed create main window");
        return sp_ui_t();
    }
}

void show_hide_window(HWND hwnd, bool show)
{
    ShowWindow(hwnd, show ? SW_SHOW : SW_HIDE);
}

class _UserInterface
    : public std::enable_shared_from_this<_UserInterface>
{
    private:
        HINSTANCE _hInstance;
        WNDCLASSEX _wce;
        ATOM _wc;
        HWND _hwndMain;
        HWND _hwndInput;
        HWND _hwndPreview;
        HWND _hwndLog;
        WNDPROC _defaultInputEditProc;

    public:
        const LPCWSTR WC_NAME = L"ttwwam-main-cls";
        const LPCWSTR W_NAME = L"ttwwam-main";
        const DWORD ID_EDITPREVIEW = 100;
        const DWORD ID_EDITLOG = 101;
        const DWORD ID_EDITINPUT = 102;
        const DWORD INPUTHEIGHT = 25;
        const DWORD EN_USER_BASE = 0x8000;
        const DWORD EN_USER_CONFIRM = EN_USER_BASE + 1;
        const DWORD EN_USER_ABORT = EN_USER_BASE + 2;


    public:
        _UserInterface(HINSTANCE hInstance);

        bool register_window_class();
        bool create_main_window();
        bool create_user_interface(HWND);

        LRESULT onInputEditMessage(HWND, UINT, WPARAM, LPARAM) const;

        void show_main_window(bool) const;
};

typedef std::shared_ptr<_UserInterface> sp__ui_t;

_UserInterface::_UserInterface(HINSTANCE hInstance)
    : _hInstance(hInstance)
{}

bool _UserInterface::register_window_class()
{
    _wce = {
        sizeof(_wce),
        0,
        WindowProc,
        0,
        0,
        _hInstance,
        LoadIcon(NULL, IDI_APPLICATION),
        LoadCursor(NULL, IDC_ARROW),
        reinterpret_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH)),
        NULL,
        WC_NAME,
        NULL,
    };

    _wc = RegisterClassEx(&_wce);
    return !!_wc;
}

bool _UserInterface::create_main_window()
{
    DWORD dwStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP;
    DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_TOPMOST;
    _hwndMain = CreateWindowEx(
            dwExStyle,
            MAKEINTATOM(_wc),
            W_NAME,
            dwStyle,
            0, 0, 800, 600,
            NULL, NULL, NULL, NULL);

    SetWindowLongPtr(_hwndMain, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&shared_from_this()));

    return !!_hwndMain;
}

bool _UserInterface::create_user_interface(HWND hwnd)
{
    _hwndInput = CreateWindowEx(
            0, L"EDIT", NULL,
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOVSCROLL,
            0, 0, 0, 0,
            hwnd,
            reinterpret_cast<HMENU>(ID_EDITINPUT),
            _hInstance,
            NULL);

    _defaultInputEditProc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtr(
                _hwndInput,
                GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(InputEditProc)));

    _hwndPreview = CreateWindowEx(
            0, L"EDIT", NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
            0, 0, 0, 0,
            hwnd,
            reinterpret_cast<HMENU>(ID_EDITPREVIEW),
            _hInstance,
            NULL);
    SendMessage(_hwndPreview, EM_SETREADONLY, TRUE, 0);

    _hwndLog = CreateWindowEx(
            0, L"EDIT", NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
            0, 0, 0, 0,
            hwnd,
            reinterpret_cast<HMENU>(ID_EDITLOG),
            _hInstance,
            NULL);
    SendMessage(_hwndLog, EM_SETREADONLY, TRUE, 0);

}

LRESULT _UserInterface::onInputEditMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) const
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
                            _hwndMain,
                            WM_COMMAND,
                            MAKEWPARAM(ID_EDITINPUT, cmd),
                            reinterpret_cast<LPARAM>(hwnd));
                    return 0;
                }
            }

        default:
            return CallWindowProc(_defaultInputEditProc, hwnd, msg, wParam, lParam);
    }
    return 0;
}


void _UserInterface::show_main_window(bool show) const
{
    show_hide_window(_hwndMain, show);
    if (!show) {
        return false;
    }

    // scan_current_desktops();

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


////////////////////////////////////////////////////////////////////////////////
UserInterface::UserInterface(HINSTANCE hInstance)
    : _p(std::make_unique<_UserInterface>(hInstance))
{}

bool UserInterface::register_window_class()
{
    return _p->register_window_class();
}

bool UserInterface::create_main_window()
{
    return _p->create_main_window();
}




bool handle_hotkey(HWND hwnd, SHORT modifiers, SHORT keycode) {
    // for(const auto& cmd : _commands) {
    //     if (!cmd.second.hotkey.first) {
    //         continue;
    //     }
    //     const hotkey_t& hk = cmd.second.hotkey.second;
    //     SHORT mod = hk.modifier & ~MOD_NOREPEAT;
    //     if ((mod == modifiers) && (hk.keycode == keycode)) {
    //         return run_command(hwnd, cmd.first);
    //     }
    // }
    return false;
}

void clear_preview()
{
    int index = GetWindowTextLength(hwndPreview);
    SendMessage(hwndPreview, EM_SETSEL, 0, index);
    SendMessage(hwndPreview, EM_REPLACESEL, 0, (LPARAM) 0);
}

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

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    sp__ui_t ui = dynamic_cast<sp__ui_t>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (!ui) {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    switch(uMsg) {
        case WM_CREATE:
            ui->create_user_interface(hwnd);
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

LRESULT CALLBACK InputEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    sp__ui_t ui = dynamic_cast<sp__ui_t>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (!ui) {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return ui->onInputEditMessage(hwnd, msg, wParam, lParam);
}
