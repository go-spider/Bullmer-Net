#ifdef XNET_EXPORTS
#define XNET_API __declspec(dllexport)
#else
#define XNET_API __declspec(dllimport)
#endif
