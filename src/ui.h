#ifndef __TTWWAM_UI__
#define __TTWWAM_UI__

#include <windows.h>

#include <memory>

class _UserInterface;

class UserInterface {
    private:
        std::unique_ptr<_UserInterface> _p;

    public:
        UserInterface(HINSTANCE hInstance);
        bool register_window_class();
        bool create_main_window();
};

typedef std::shared_ptr<UserInterface> sp_ui_t;

sp_ui_t init_ui(HINSTANCE hInstance);

#endif // __TTWWAM_UI__
