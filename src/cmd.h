#ifndef __TTWWAM_CMD__
#define __TTWWAM_CMD__

#include <windows.h>

#include <functional>
#include <memory>
#include <sstream>
#include <vector>

#include "log.h"
#include "ui.h"
#include "wm.h"

struct hotkey_t {
    SHORT modifier;
    SHORT keycode;
};

struct cmd_t {
    std::wstring cmd;
    std::vector<std::wstring> args;
};

struct cmd_spec_t {
    std::pair<bool, hotkey_t> hotkey;
    std::function<bool(HWND, cmd_t)> func;
};

class CommandProcessor {
    public:
        CommandProcessor(sp_wm_t, sp_ui_t, sp_log_t);
};

typedef std::shared_ptr<CommandProcessor> sp_cmd_t;

sp_cmd_t init_cmd(sp_wm_t, sp_ui_t, sp_log_t);

#endif // __TTWWAM_CMD__
