#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>

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

void show_hide_window(HWND hwnd, bool show)
{
    ShowWindow(hwnd, show ? SW_SHOW : SW_HIDE);
}

bool init_ui(HINSTANCE hInstance) {
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
        return false;
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
        return false;
    }

    log_debug(L"main window created");
    return true;
}

sp_ui_t init_ui(HINSTANCE hInstance)
{
    return std::make_shared<UI>(hInstance);
}

UserInterface::UserInterface(HINSTANCE hInstance)
{
}
