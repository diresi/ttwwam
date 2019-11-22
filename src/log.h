#ifndef __TTWWAM_LOG__
#define __TTWWAM_LOG__

#include <memory>
#include <string>

#include "ui.h"


class Logger {
};

typedef std::shared_ptr<Logger> sp_log_t;

sp_log_t init_log(sp_ui_t ui);

void log_fatal_error(const std::wstring& msg);

#endif // __TTWWAM_LOG__
