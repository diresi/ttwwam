#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>
#include "lib/ttwwam.h"

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    return ttwwam_main(hInstance, hPrevInstance, lpszCmdLine, nCmdShow);
}
