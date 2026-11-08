#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void* snovi_create(void);
void snovi_destroy(void* handle);
char* snovi_execute(void* handle, const char* action, const char* argument);
void snovi_free(char* value);
#ifdef __cplusplus
}
#endif
