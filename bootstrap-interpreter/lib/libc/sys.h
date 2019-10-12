#ifndef SYS_H
#define SYS_H

// sys/types.h
typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ssize_t;
typedef __PTRDIFF_TYPE__ off_t;
typedef int pid_t;

// unistd.h
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
ssize_t read(int fd, void *buf, size_t nbyte);
ssize_t write(int fd, const void *buf, size_t nbyte);
int close(int fd);
void _exit(int status);
pid_t getpid(void);
//int fsync(int fd);

// fcntl.h
#define O_RDONLY 0
int open(const char *path, int oflag);

// signal.h
#define SIG_DFL 0
#define SIGINT 2
#define SIGABRT 6
int kill(pid_t pid, int sig);

// dummy implementation
static inline void (*signal(int sig, void (*func)(int)))(int) {
    (void)sig;
    return func;
}

// sys/mman.h
#define MAP_FAILED    ((void*)-1)
#define PAGE_SIZE     4096
#define PROT_READ     0x1         /* page can be read */
#define PROT_WRITE    0x2         /* page can be written */
#define MAP_PRIVATE   0x02        /* Changes are private */
#define MAP_ANONYMOUS 0x20        /* don't use a file */
void* mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t len);

#endif
