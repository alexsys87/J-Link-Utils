// Forced into every translation unit: an ImGui assertion fails the smoke test.
#pragma once
#ifdef __cplusplus
extern "C"
#endif
void SmokeAssertFailed(const char* expr, const char* file, int line);
#define IM_ASSERT(_EXPR) ((_EXPR) ? (void)0 : SmokeAssertFailed(#_EXPR, __FILE__, __LINE__))
