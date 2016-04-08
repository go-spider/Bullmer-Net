#ifndef _XUTILS_H_INCLUDE_
#define _XUTILS_H_INCLUDE_

#ifdef __cplusplus
extern "C" {
#endif

XNET_API void dbgprint(char* format,...);

XNET_API void x_random_string(char const* str, u32_t length, char * buf, u32_t size);

#ifdef __cplusplus
}
#endif

#endif // _XUTILS_H_INCLUDE_