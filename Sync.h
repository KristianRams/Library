#pragma once

#include "Types.h"

#if defined(_WIN32)
#include <windows.h>
#include <synchapi.h>
#endif

#ifdef linux
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#endif

#include <stdio.h>

// A platform independent wrapper around multithreading primatives.

void sleep_seconds(u32 seconds) { 
#if defined(_WIN32)
    u32 milliseconds = seconds * 1000;
    Sleep(milliseconds);
#endif

#ifdef linux
    sleep(seconds);
#endif
}

struct Semaphore {
#if defined(_WIN32)
    HANDLE semaphore_handle;
#endif

#ifdef linux
    sem_t semaphore_handle;
#endif
};

void semaphore_create(Semaphore *semaphore, u32 count) {
#if defined(_WIN32)
    u64 value = (1 << 32) - 1;
    semaphore->semaphore_handle = CreateSemaphore(NULL, 0, value, NULL);
#endif 

#ifdef linux
    sem_init(&semaphore->semaphore_handle, 0, count);
#endif
}

void semaphore_destroy(Semaphore *semaphore) { 
#if defined(_WIN32)
    CloseHandle(semaphore->semaphore_handle);
#endif 

#ifdef linux
    sem_destroy(&semaphore->semaphore_handle);
#endif 
}

void semaphore_lock(Semaphore *semaphore) { 
#if defined(_WIN32)
    WaitForSingleObjectEx(semaphore->semaphore_handle, INFINITE, FALSE);
#endif 

#ifdef linux
    sem_wait(&semaphore->semaphore_handle);
#endif 
}

void semaphore_unlock(Semaphore *semaphore) { 
#if defined(_WIN32)
    ReleaseSemaphore(semaphore->semaphore_handle, 1, NULL);
#endif 

#ifdef linux
    sem_post(&semaphore->semaphore_handle);
#endif 
}



struct Mutex {
#if defined(_WIN32)
    SRWLOCK lock; // Reader | Writer lock.
#endif

#ifdef linux 
    pthread_mutex_t pthread_mutex;
#endif 
};

void mutex_create(Mutex *mutex) { 
#if defined(_WIN32)
    InitializeSRWLock(&mutex->lock);
#endif

#ifdef linux    
    pthread_mutex_init(&mutex->pthread_mutex, NULL);
#endif
}

void mutex_destroy(Mutex *mutex) { 
#if defined(_WIN32)
    return;
#endif

#ifdef linux
    pthread_mutex_destroy(&mutex->pthread_mutex);
#endif
}

void mutex_lock(Mutex *mutex) { 
#if defined(_WIN32)
    AcquireSRWLockExclusive(&mutex->lock);
#endif

#ifdef linux
    pthread_mutex_lock(&mutex->pthread_mutex);
#endif
}

void mutex_unlock(Mutex *mutex) { 
#if defined(_WIN32)
    ReleaseSRWLockExclusive(&mutex->lock);
#endif

#ifdef linux
    pthread_mutex_unlock(&mutex->pthread_mutex);
#endif
}



struct Scoped_Lock {
    Mutex *mutex;

    Scoped_Lock(Mutex *_mutex) { 
        mutex = _mutex;
        mutex_lock(mutex);
    }

    ~Scoped_Lock() { 
        mutex_unlock(mutex);
    }
};

// input_data  will hold the necessary parameters
// output_data will hold the necessary return data
// return_value can be used if you don't need to return out any data.
struct Thread_Context { 
    void *input_data;
    s32   input_data_size;
    
    void *output_data;
    s32   output_data_size;
    
    s32   return_value; // Of the thread, set in Thread_Procedure.
};

typedef void (*Thread_Procedure)(Thread_Context *);

struct Thread { 
#if defined(_WIN32)
    HANDLE thread_handle;
#endif 

#ifdef linux
    pthread_attr_t thread_attributes;
    pthread_t thread_handle;
#endif 

    Thread_Context    *context;
    Thread_Procedure  procedure = NULL;
    s32               id = 0;
};

void _thread_join(Thread *thread) {
#if defined(_WIN32)
    WaitForSingleObject(thread->thread_handle, INFINITE);
#endif 

#ifdef linux
    pthread_join(thread->thread_handle, NULL);
#endif 
}

void thread_join(Thread *thread) { 
#if defined(_WIN32)
    _thread_join(thread);
    CloseHandle(thread->thread_handle);
#endif 

#ifdef linux
    pthread_attr_destroy(&thread->thread_attributes);
    _thread_join(thread);
#endif 
}

// This is very annoying !!!!
#if defined(_WIN32)
DWORD WINAPI internal_thread_procedure(void *parameters) { 
    Thread *t = (Thread *)parameters;
    t->procedure(t->context);
    return 0;
}
#endif 

#ifdef linux
void *internal_thread_procedure(void *parameters) { 
    Thread *t = (Thread *)parameters;
    t->procedure(t->context);
    pthread_exit(NULL);
}
#endif 

void thread_start(Thread *thread, Thread_Procedure thread_procedure) {
    thread->procedure = thread_procedure;
    
#if defined(_WIN32)
    thread->thread_handle = CreateThread(NULL, 0, internal_thread_procedure, (void *)thread, 0, (LPDWORD)&thread->id);
#endif 

#ifdef linux
    pthread_attr_init(&thread->thread_attributes);
    pthread_attr_setdetachstate(&thread->thread_attributes, PTHREAD_CREATE_JOINABLE);
    pthread_create(&thread->thread_handle, &thread->thread_attributes, internal_thread_procedure, (void *)thread);
    thread->id = thread->thread_handle;
#endif 
}
