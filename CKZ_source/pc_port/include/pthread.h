#pragma once
#include <SDL3/SDL_mutex.h>

typedef SDL_Mutex* pthread_mutex_t;

inline int pthread_mutex_init(pthread_mutex_t* mutex, const void*)
{
    *mutex = SDL_CreateMutex();
    return *mutex == nullptr;
}
inline void pthread_mutex_destroy(pthread_mutex_t* mutex) { SDL_DestroyMutex(*mutex); }
inline void pthread_mutex_lock(pthread_mutex_t* mutex) { SDL_LockMutex(*mutex); }
inline void pthread_mutex_unlock(pthread_mutex_t* mutex) { SDL_UnlockMutex(*mutex); }
