#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>

#include "cmd.h"
#include "log.h"
#include "ui.h"
#include "wm.h"


// live previews:
// https://www.victorhurdugaci.com/fancy-windows-previewer

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    auto ui = init_ui(hInstance);
    if (!ui) {
        log_fatal_error(L"Failed to initialize my fantastic UI");
        return -1;
    }

    auto log = init_log(ui);
    if (!log) {
        log_fatal_error(L"Failed to initialize my wonderful loggers");
        return -2;
    }

    auto wm = init_wm(ui, log);
    if (!wm) {
        log_fatal_error(L"Failed to initialize my superb window manager");
        return -3;
    }

    auto cmd = init_cmd(wm, ui, log);
    if (!cmd) {
        log_fatal_error(L"Failed to initialize my glorious command interpreter");
        return -4;
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

    wm->show_all_containers();
    return static_cast<int>(msg.wParam);
}
