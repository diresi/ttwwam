#ifndef __TTWWAM_UI__
#define __TTWWAM_UI__

#include <windows.h>

class UserInterface {
    UserInterface(HINSTANCE hInstance);
};

typedef std::shared_ptr<UserInterface> sp_ui_t;

sp_ui_t init_ui(HINSTANCE hInstance);

#endif // __TTWWAM_UI__
