// the few C library hooks newlib wants (no files, no console)
#include <errno.h>
#include <sys/stat.h>
extern char __heap_start; static char*brk;
void*_sbrk(int n){ if(!brk) brk=&__heap_start; char*r=brk; if(brk+n>(char*)0x022E0000){ errno=12; return (void*)-1; } brk+=n; return r; }
int _write(int f,char*p,int n){ (void)f;(void)p; return n; } int _read(int f,char*p,int n){ (void)f;(void)p;(void)n; return 0; } int _close(int f){ (void)f; return -1; }
int _fstat(int f,struct stat*s){ (void)f; s->st_mode=S_IFCHR; return 0; } int _isatty(int f){ (void)f; return 1; } int _lseek(int f,int p,int w){ (void)f;(void)p;(void)w; return 0; }
void _exit(int c){ (void)c; for(;;){} } int _kill(int p,int s){ (void)p;(void)s; return -1; } int _getpid(void){ return 1; } int _open(const char*n,int f,int m){ (void)n;(void)f;(void)m; return -1; }
