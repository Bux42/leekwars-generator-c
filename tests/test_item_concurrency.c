#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
typedef HANDLE Thread;
#else
#include <pthread.h>
typedef pthread_t Thread;
#endif

#include "lw_item.h"

#define THREAD_COUNT 8
#define ITERATIONS 100000

#ifdef _WIN32
static DWORD WINAPI exercise_item(LPVOID arg) {
#else
static void *exercise_item(void *arg) {
#endif
    LwItem *item = arg;
    for (int i = 0; i < ITERATIONS; ++i) {
        lw_item_retain(item);
        lw_item_release(item);
    }
    return 0;
}

int main(void) {
    LwItem *item = calloc(1, sizeof(*item));
    Thread threads[THREAD_COUNT];
    if (!item) return 1;
    lw_item_init(item, 1, 0, "shared", 0, NULL);
    if (!lw_item_manage(item)) return 1;
    for (int i = 0; i < THREAD_COUNT; ++i) {
#ifdef _WIN32
        threads[i] = CreateThread(NULL, 0, exercise_item, item, 0, NULL);
        if (!threads[i]) return 1;
#else
        if (pthread_create(&threads[i], NULL, exercise_item, item)) return 1;
#endif
    }
    for (int i = 0; i < THREAD_COUNT; ++i) {
#ifdef _WIN32
        if (WaitForSingleObject(threads[i], INFINITE) != WAIT_OBJECT_0) return 1;
        CloseHandle(threads[i]);
#else
        if (pthread_join(threads[i], NULL)) return 1;
#endif
    }
    if (item->references != 1) {
        fprintf(stderr, "incorrect reference count: %ld\n", item->references);
        return 1;
    }
    lw_item_release(item);
    return 0;
}
