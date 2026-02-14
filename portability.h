#ifndef PORTABILITY_H
#define PORTABILITY_H

#include <stdio.h>
#include <stdlib.h>

/**
 * Windows/Linux portability management for network and multithreaded programming.
 * Uses Winsock2 on Windows and POSIX sockets on Linux.
 */
#ifdef _WIN32
    #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0600 /* Target Windows Vista minimum for inet_pton */
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    
    typedef int socklen_t;
    typedef SOCKET socket_t;
    #define close_socket(s) closesocket(s)
    #define INVALID_SOCKET_VAL INVALID_SOCKET
    
    /* Thread abstraction for Win32 API */
    typedef HANDLE thread_t;
    #define THREAD_FUNCTION DWORD WINAPI
    #define THREAD_RETURN_VAL 0
    #define THREAD_EXIT() ExitThread(0)

    /* Mutex abstraction for Win32 (Critical Sections) */
    typedef CRITICAL_SECTION mutex_t;
    #define mutex_init(m) InitializeCriticalSection(m)
    #define mutex_lock(m) EnterCriticalSection(m)
    #define mutex_unlock(m) LeaveCriticalSection(m)
    #define mutex_destroy(m) DeleteCriticalSection(m)

    static inline int thread_create(thread_t* thread, LPTHREAD_START_ROUTINE start_routine, void* arg) {
        *thread = CreateThread(NULL, 0, start_routine, arg, 0, NULL);
        return (*thread == NULL) ? -1 : 0;
    }

    static inline void thread_detach(thread_t thread) {
        CloseHandle(thread);
    }

    /* Fallback implementation for inet_pton on older/restricted Windows environments */
    static inline int portable_inet_pton(int af, const char* src, void* dst) {
        if (af == AF_INET) {
            unsigned long addr = inet_addr(src);
            if (addr == INADDR_NONE) return 0;
            memcpy(dst, &addr, sizeof(addr));
            return 1;
        }
        return -1;
    }
    #define inet_pton portable_inet_pton

    /* Macro for Windows network stack initialization (WSA) */
    #define init_networking() do { \
        WSADATA wsa; \
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) { \
            fprintf(stderr, "WSAStartup failed\n"); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)
    #define cleanup_networking() WSACleanup()

    extern char* _strdup(const char*);
    #define strdup _strdup

#else
    /* Configuration for POSIX systems (Linux, macOS, etc.) */
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <pthread.h>

    typedef int socket_t;
    #define close_socket(s) close(s)
    #define INVALID_SOCKET_VAL -1

    /* Thread abstraction for POSIX Pthreads */
    typedef pthread_t thread_t;
    #define THREAD_FUNCTION void*
    #define THREAD_RETURN_VAL NULL
    #define THREAD_EXIT() pthread_exit(NULL)

    /* Mutex abstraction for POSIX Pthreads */
    typedef pthread_mutex_t mutex_t;
    #define mutex_init(m) pthread_mutex_init(m, NULL)
    #define mutex_lock(m) pthread_mutex_lock(m)
    #define mutex_unlock(m) pthread_mutex_unlock(m)
    #define mutex_destroy(m) pthread_mutex_destroy(m)

    static inline int thread_create(thread_t* thread, void* (*start_routine)(void*), void* arg) {
        return pthread_create(thread, NULL, start_routine, arg);
    }

    static inline void thread_detach(thread_t thread) {
        pthread_detach(thread);
    }

    #define init_networking() do {} while(0)
    #define cleanup_networking() do {} while(0)
#endif

#endif
