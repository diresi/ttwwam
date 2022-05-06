#ifndef _LIBTTWWAM_H_
#define _LIBTTWWAM_H_

#ifdef _LIBTTWWAM_H_SHARED_
#include "libttwwam_export.h"
#else
#define LIBTTWWAM_EXPORT
#endif // _LIBTTWWAM_H_SHARED_

LIBTTWWAM_EXPORT int CALLBACK ttwwam_main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow);

#endif // _LIBTTWWAM_H_
