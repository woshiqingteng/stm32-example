/**
 * @file    syscalls.c
 * @brief   Minimal newlib syscall stubs (USART _write lives in usart.c).
 */

#include <sys/stat.h>
#include <errno.h>

extern char _end;
extern char _estack;

int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

off_t _lseek(int file, off_t ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

ssize_t _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

void *_sbrk(int incr)
{
    static char *heap_end = 0;
    char *prev = heap_end;

    if (heap_end == 0)
    {
        heap_end = &_end;
    }
    if (heap_end + incr > &_estack - 0x400)
    {
        errno = ENOMEM;
        return (void *)-1;
    }

    heap_end += incr;
    return prev;
}
