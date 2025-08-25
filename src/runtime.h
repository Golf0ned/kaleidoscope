#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

extern "C" DLLEXPORT double print(double val);
extern "C" DLLEXPORT double printStar();
extern "C" DLLEXPORT double printSpace();
extern "C" DLLEXPORT double printNewLine();
