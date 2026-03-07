/*
 * mos_libc.h - Capa de compatibilidad libc -> MOS API para ESP32-MOS
 *
 * Permite compilar código C estándar (printf, fopen, malloc, etc.)
 * en modo -nostdlib usando la tabla de saltos MOS.
 *
 * Uso:
 *   #define MOS_LIBC_IMPL   (en UN solo .c, antes del include)
 *   #include "mos_libc.h"
 *
 *   En el resto de archivos (sin MOS_LIBC_IMPL):
 *   #include "mos_libc.h"
 *
 * El _start recibe el puntero MOS y debe llamar a mos_libc_init(mos)
 * antes de cualquier llamada a printf/fopen/etc.
 */

#ifndef MOS_LIBC_H
#define MOS_LIBC_H

#include "mos_api_table.h"
#include <stdarg.h>

/* ── Tipos libc mínimos ───────────────────────────────────────────────── */
typedef unsigned char  uint8_t;
typedef unsigned int   size_t;
typedef long           off_t;

#define NULL  ((void *)0)
#define EOF   (-1)

/* FILE handle: envuelve el fh de MOS (uint8_t) */
typedef struct { uint8_t fh; } FILE;

/* Streams estándar pre-declarados */
extern FILE *stdout;
extern FILE *stderr;
extern FILE *stdin;

/* ── API pública ──────────────────────────────────────────────────────── */

/* Llamar desde _start antes de usar cualquier función libc */
void mos_libc_init(t_mos_api *mos);

/* I/O de consola */
int  printf(const char *fmt, ...);
int  fprintf(FILE *stream, const char *fmt, ...);
int  puts(const char *s);
int  fputc(int c, FILE *stream);
size_t fwrite(const void *buf, size_t sz, size_t n, FILE *stream);

/* Archivos */
FILE   *fopen(const char *path, const char *mode);
int     fclose(FILE *f);
int     fgetc(FILE *f);
char   *fgets(char *buf, int n, FILE *f);
int     feof(FILE *f);
size_t  fread(void *buf, size_t sz, size_t n, FILE *f);
long    ftell(FILE *f);
int     fseek(FILE *f, long offset, int whence);

/* Memoria */
void *malloc(size_t size);
void  free(void *ptr);
void *calloc(size_t n, size_t sz);

/* String utils (implementados aquí para no depender de libc) */
int    atoi(const char *s);
size_t strlen(const char *s);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, size_t n);
char  *strcpy(char *dst, const char *src);
char  *strncpy(char *dst, const char *src, size_t n);
char  *strcat(char *dst, const char *src);
char  *strdup(const char *s);
char  *strstr(const char *hay, const char *needle);
char  *strrchr(const char *s, int c);
char  *strchrnul(const char *s, int c);
char  *strndup(const char *s, size_t n);
int    memcmp(const void *a, const void *b, size_t n);
long   strtol(const char *s, char **endp, int base);
int    tolower(int c);
int    toupper(int c);
int    isprint(int c);
int    isdigit(int c);
int    isalpha(int c);
int    isalnum(int c);
int    isspace(int c);
int    isupper(int c);
int    islower(int c);
int    ispunct(int c);

/* Printf con destino a buffer */
int vsnprintf(char *buf, size_t sz, const char *fmt, va_list ap);
int  snprintf(char *buf, size_t sz, const char *fmt, ...);

/* Memoria */
void *realloc(void *ptr, size_t size);

/* Consola extra */
int putchar(int c);

/* errno / strerror (stubs — MOS no expone errno) */
extern int errno;
char *strerror(int e);

/* Algoritmos */
void qsort(void *base, size_t n, size_t sz,
           int (*cmp)(const void *, const void *));

/* fseek whence */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* ── Implementación (solo cuando MOS_LIBC_IMPL está definido) ─────────── */
#ifdef MOS_LIBC_IMPL

#include <stdarg.h>

/* Puntero global a la tabla MOS */
static t_mos_api *_mos;

/* Streams estándar: stdout/stderr apuntan a consola (fh=0), stdin no usado */
static FILE _stdout_f = {0};
static FILE _stderr_f = {0};
FILE *stdout = &_stdout_f;
FILE *stderr = &_stderr_f;
FILE *stdin  = NULL;

void mos_libc_init(t_mos_api *mos)
{
    _mos = mos;
}

/* ── String utils ─────────────────────────────────────────────────────── */

size_t strlen(const char *s)
{
    size_t n = 0;
    while (*s++) n++;
    return n;
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n)
{
    while (n-- && *a && *a == *b) { a++; b++; }
    if (!n) return 0;  /* n fue 0 tras el decremento: iguales hasta n */
    /* Tras el bucle: o se agotó n (ya retornado), o diferencia */
    return (unsigned char)*a - (unsigned char)*b;
}

char *strcpy(char *dst, const char *src)
{
    char *d = dst;
    while ((*d++ = *src++));
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
    char *d = dst;
    while (n-- && (*d++ = *src++));
    while (n-- > 0) *d++ = '\0';
    return dst;
}

char *strstr(const char *hay, const char *needle)
{
    if (!*needle) return (char *)hay;
    for (; *hay; hay++) {
        const char *h = hay, *n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return (char *)hay;
    }
    return NULL;
}

int tolower(int c)
{
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}

int isprint(int c)
{
    return (c >= 32 && c < 127);
}

int toupper(int c)
{
    if (c >= 'a' && c <= 'z') return c - ('a' - 'A');
    return c;
}

char *strcat(char *dst, const char *src)
{
    char *d = dst;
    while (*d) d++;
    while ((*d++ = *src++));
    return dst;
}

char *strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = (char *)_mos->malloc(n);
    if (p) strcpy(p, s);
    return p;
}

char *strndup(const char *s, size_t n)
{
    size_t len = 0;
    while (len < n && s[len]) len++;
    char *p = (char *)_mos->malloc(len + 1);
    if (p) { for (size_t i = 0; i < len; i++) p[i] = s[i]; p[len] = '\0'; }
    return p;
}

char *strrchr(const char *s, int c)
{
    const char *last = NULL;
    for (; *s; s++) {
        if (*s == (char)c) last = s;
    }
    if (c == '\0') return (char *)s;
    return (char *)last;
}

char *strchrnul(const char *s, int c)
{
    for (; *s && *s != (char)c; s++);
    return (char *)s;
}

int memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;
    while (n--) {
        if (*pa != *pb) return (int)*pa - (int)*pb;
        pa++; pb++;
    }
    return 0;
}

long strtol(const char *s, char **endp, int base)
{
    long val = 0; int sign = 1;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') { sign = -1; s++; } else if (*s == '+') s++;
    if (base == 0) {
        if (*s == '0' && (s[1] == 'x' || s[1] == 'X')) { base = 16; s += 2; }
        else if (*s == '0') { base = 8; s++; }
        else base = 10;
    } else if (base == 16 && *s == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    for (;;) {
        int d;
        if (*s >= '0' && *s <= '9') d = *s - '0';
        else if (*s >= 'a' && *s <= 'f') d = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F') d = *s - 'A' + 10;
        else break;
        if (d >= base) break;
        val = val * base + d; s++;
    }
    if (endp) *endp = (char *)s;
    return sign * val;
}

int isdigit(int c) { return (c >= '0' && c <= '9'); }
int isalpha(int c) { return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')); }
int isalnum(int c) { return isdigit(c) || isalpha(c); }
int isspace(int c) { return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'); }
int isupper(int c) { return (c >= 'A' && c <= 'Z'); }
int islower(int c) { return (c >= 'a' && c <= 'z'); }
int ispunct(int c) { return (c > 32 && c < 127 && !isalnum(c)); }

/* qsort — implementación heapsort iterativa, O(n log n), sin recursión extra */
static void _qsort_swap(char *a, char *b, size_t sz)
{
    char tmp;
    while (sz--) { tmp = *a; *a++ = *b; *b++ = tmp; }
}

void qsort(void *base, size_t n, size_t sz,
           int (*cmp)(const void *, const void *))
{
    char *b = (char *)base;
    size_t i, j, root, child;

    if (n < 2) return;

    /* Build max-heap */
    for (i = n/2; i-- > 0; ) {
        root = i;
        for (;;) {
            child = 2*root + 1;
            if (child >= n) break;
            if (child+1 < n && cmp(b+child*sz, b+(child+1)*sz) < 0)
                child++;
            if (cmp(b+root*sz, b+child*sz) >= 0) break;
            _qsort_swap(b+root*sz, b+child*sz, sz);
            root = child;
        }
    }

    /* Sort */
    for (j = n-1; j > 0; j--) {
        _qsort_swap(b, b+j*sz, sz);
        root = 0;
        for (;;) {
            child = 2*root + 1;
            if (child >= j) break;
            if (child+1 < j && cmp(b+child*sz, b+(child+1)*sz) < 0)
                child++;
            if (cmp(b+root*sz, b+child*sz) >= 0) break;
            _qsort_swap(b+root*sz, b+child*sz, sz);
            root = child;
        }
    }
}

int atoi(const char *s)
{
    int sign = 1, val = 0;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') val = val * 10 + (*s++ - '0');
    return sign * val;
}

/* ── printf mínimo ────────────────────────────────────────────────────── */

/* Escribe un entero decimal sin signo en buf, retorna longitud */
static int _uitoa(unsigned int v, char *buf)
{
    if (v == 0) { buf[0] = '0'; return 1; }
    char tmp[12]; int n = 0, len;
    while (v) { tmp[n++] = '0' + (v % 10); v /= 10; }
    for (len = 0; len < n; len++) buf[len] = tmp[n - 1 - len];
    return len;
}

/* Escribe un hex sin signo en buf */
static int _uxtoa(unsigned int v, char *buf, int upper)
{
    const char *hex = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char tmp[9]; int n = 0, len;
    if (v == 0) { buf[0] = '0'; return 1; }
    while (v) { tmp[n++] = hex[v & 0xF]; v >>= 4; }
    for (len = 0; len < n; len++) buf[len] = tmp[n - 1 - len];
    return len;
}

/* ── Motor de formato genérico (sink configurable) ───────────────────── */

/* Sink: destino de salida. buf==NULL → consola; buf!=NULL → buffer acotado */
typedef struct {
    char   *buf;   /* NULL = consola */
    size_t  cap;   /* capacidad disponible (incluyendo NUL) */
    size_t  pos;   /* bytes escritos (sin contar NUL) */
} _FmtSink;

static void _sink_ch(_FmtSink *s, char c)
{
    if (s->buf) {
        if (s->pos + 1 < s->cap) s->buf[s->pos] = c;
    } else {
        _mos->putch((uint8_t)c);
    }
    s->pos++;
}

static void _sink_str(_FmtSink *s, const char *p)
{
    if (s->buf) {
        while (*p) { _sink_ch(s, *p++); }
    } else {
        /* Consola: putch carácter a carácter (puts añade \r\n) */
        while (*p) _mos->putch((uint8_t)*p++);
    }
}

/* Formatear con ancho mínimo w y relleno pad_ch */
static void _sink_pad(_FmtSink *s, const char *str, int slen, int width, int left, char pad_ch)
{
    int pad = width - slen;
    if (!left) for (int i = 0; i < pad; i++) _sink_ch(s, pad_ch);
    for (int i = 0; i < slen; i++) _sink_ch(s, str[i]);
    if (left)  for (int i = 0; i < pad; i++) _sink_ch(s, ' ');
}

static int _vfmt(_FmtSink *s, const char *fmt, va_list ap)
{
    char tmp[32];
    for (; *fmt; fmt++) {
        if (*fmt != '%') { _sink_ch(s, *fmt); continue; }
        fmt++;

        /* Flags */
        int left = 0, zero = 0;
        while (*fmt == '-' || *fmt == '0') {
            if (*fmt == '-') left = 1;
            if (*fmt == '0') zero = 1;
            fmt++;
        }
        char pad_ch = (zero && !left) ? '0' : ' ';

        /* Ancho */
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');

        /* Precisión */
        int prec = -1;
        if (*fmt == '.') {
            fmt++; prec = 0;
            while (*fmt >= '0' && *fmt <= '9') prec = prec * 10 + (*fmt++ - '0');
        }

        /* Modificador de longitud */
        int lng = 0;
        if (*fmt == 'l') { lng = 1; fmt++; }
        if (*fmt == 'l') { fmt++; } /* ll → tratar como l */

        switch (*fmt) {
        case 'd': case 'i': {
            long v = lng ? va_arg(ap, long) : (long)va_arg(ap, int);
            int neg = (v < 0);
            if (neg) v = -v;
            int n = _uitoa((unsigned int)v, tmp); tmp[n] = '\0';
            int slen = n + neg;
            int pad = width - slen; if (pad < 0) pad = 0;
            if (!left && pad_ch == '0') { if (neg) _sink_ch(s, '-'); for (int i=0;i<pad;i++) _sink_ch(s,'0'); }
            else if (!left) { for (int i=0;i<pad;i++) _sink_ch(s,' '); if (neg) _sink_ch(s,'-'); }
            else { if (neg) _sink_ch(s,'-'); }
            _sink_str(s, tmp);
            if (left) for (int i=0;i<pad;i++) _sink_ch(s,' ');
            break;
        }
        case 'u': {
            unsigned long v = lng ? va_arg(ap, unsigned long) : (unsigned long)va_arg(ap, unsigned int);
            int n = _uitoa((unsigned int)v, tmp); tmp[n] = '\0';
            _sink_pad(s, tmp, n, width, left, pad_ch);
            break;
        }
        case 'x': case 'X': {
            unsigned long v = lng ? va_arg(ap, unsigned long) : (unsigned long)va_arg(ap, unsigned int);
            int n = _uxtoa((unsigned int)v, tmp, (*fmt=='X')); tmp[n] = '\0';
            _sink_pad(s, tmp, n, width, left, pad_ch);
            break;
        }
        case 's': {
            const char *p = va_arg(ap, const char *);
            if (!p) p = "(null)";
            int n = (int)strlen(p);
            if (prec >= 0 && n > prec) n = prec;
            _sink_pad(s, p, n, width, left, ' ');
            break;
        }
        case 'c': {
            char c = (char)va_arg(ap, int);
            tmp[0] = c;
            _sink_pad(s, tmp, 1, width, left, ' ');
            break;
        }
        case 'p': {
            unsigned int v = (unsigned int)(unsigned long)va_arg(ap, void *);
            _sink_str(s, "0x");
            int n = _uxtoa(v, tmp, 0); tmp[n] = '\0';
            _sink_str(s, tmp);
            break;
        }
        case '%':
            _sink_ch(s, '%');
            break;
        default:
            _sink_ch(s, '%');
            if (lng) _sink_ch(s, 'l');
            _sink_ch(s, *fmt);
            break;
        }
    }
    if (s->buf) {
        /* NUL-terminado */
        if (s->cap > 0) {
            size_t end = s->pos < s->cap ? s->pos : s->cap - 1;
            s->buf[end] = '\0';
        }
    }
    return (int)s->pos;
}

/* Wrappers de consola (usados por printf/fprintf) */
static int _vprintf_impl(const char *fmt, va_list ap)
{
    _FmtSink s = { NULL, 0, 0 };
    return _vfmt(&s, fmt, ap);
}

int printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = _vprintf_impl(fmt, ap);
    va_end(ap);
    return r;
}

int fprintf(FILE *stream, const char *fmt, ...)
{
    (void)stream;  /* stdout y stderr van a consola */
    va_list ap;
    va_start(ap, fmt);
    int r = _vprintf_impl(fmt, ap);
    va_end(ap);
    return r;
}

int puts(const char *s)
{
    _mos->puts(s);
    _mos->puts("\r\n");
    return 0;
}

int fputc(int c, FILE *stream)
{
    (void)stream;
    _mos->putch((uint8_t)c);
    return c;
}

int fwrite_console(const void *buf, size_t n)
{
    const char *p = (const char *)buf;
    for (size_t i = 0; i < n; i++) _mos->putch((uint8_t)p[i]);
    return (int)n;
}

/* ── Archivos ─────────────────────────────────────────────────────────── */

/* Pool estático de FILE handles (MOS soporta pocos ficheros abiertos) */
#define MAX_FILES 8
static FILE _file_pool[MAX_FILES];
static int  _file_used[MAX_FILES];

FILE *fopen(const char *path, const char *mode)
{
    uint8_t fh = _mos->fopen(path, mode);
    if (!fh) return NULL;
    for (int i = 0; i < MAX_FILES; i++) {
        if (!_file_used[i]) {
            _file_pool[i].fh = fh;
            _file_used[i] = 1;
            return &_file_pool[i];
        }
    }
    /* sin hueco: cerrar y retornar NULL */
    _mos->fclose(fh);
    return NULL;
}

int fclose(FILE *f)
{
    if (!f) return EOF;
    int r = _mos->fclose(f->fh);
    /* liberar slot */
    for (int i = 0; i < MAX_FILES; i++) {
        if (&_file_pool[i] == f) { _file_used[i] = 0; break; }
    }
    return r ? EOF : 0;
}

int fgetc(FILE *f)
{
    if (!f) return EOF;
    return _mos->fgetc(f->fh);
}

char *fgets(char *buf, int n, FILE *f)
{
    if (!f || n <= 0) return NULL;
    int i = 0;
    while (i < n - 1) {
        int c = _mos->fgetc(f->fh);
        if (c == EOF) break;
        buf[i++] = (char)c;
        if (c == '\n') break;
    }
    if (i == 0) return NULL;
    buf[i] = '\0';
    return buf;
}

int feof(FILE *f)
{
    if (!f) return 1;
    return _mos->feof(f->fh);
}

size_t fread(void *buf, size_t sz, size_t n, FILE *f)
{
    if (!f) return 0;
    return _mos->fread(buf, sz, n, f->fh);
}

size_t fwrite(const void *buf, size_t sz, size_t n, FILE *stream)
{
    if (!stream) return 0;
    /* stdout/stderr → consola */
    if (stream == stdout || stream == stderr) {
        fwrite_console(buf, sz * n);
        return n;
    }
    return _mos->fwrite(buf, sz, n, stream->fh);
}

long ftell(FILE *f)
{
    if (!f) return -1L;
    return _mos->ftell(f->fh);
}

int fseek(FILE *f, long offset, int whence)
{
    if (!f) return -1;
    return _mos->flseek(f->fh, offset, whence);
}

/* ── vsnprintf / snprintf ─────────────────────────────────────────────── */

int vsnprintf(char *buf, size_t sz, const char *fmt, va_list ap)
{
    _FmtSink s = { buf, sz, 0 };
    return _vfmt(&s, fmt, ap);
}

int snprintf(char *buf, size_t sz, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(buf, sz, fmt, ap);
    va_end(ap);
    return r;
}

/* ── realloc ──────────────────────────────────────────────────────────── */

/* MOS no tiene realloc. Para soportarlo correctamente necesitamos saber
   el tamaño del bloque anterior. Instrumentamos malloc/free/realloc con
   un header de 4 bytes delante del bloque usuario.
   IMPORTANTE: esto hace que malloc() retorne ptr+4; free() ajusta ptr-4.
   Todos los comandos que usan MOS_LIBC_IMPL se benefician de esto. */

#define _RHEADER 4   /* sizeof(size_t) redondeado a 4 para alineación */

void *malloc(size_t size)
{
    unsigned char *raw = (unsigned char *)_mos->malloc(size + _RHEADER);
    if (!raw) return NULL;
    /* Guardar tamaño en los primeros 4 bytes */
    raw[0] = (unsigned char)(size & 0xFF);
    raw[1] = (unsigned char)((size >> 8) & 0xFF);
    raw[2] = (unsigned char)((size >> 16) & 0xFF);
    raw[3] = (unsigned char)((size >> 24) & 0xFF);
    return raw + _RHEADER;
}

void free(void *ptr)
{
    if (!ptr) return;
    _mos->free((unsigned char *)ptr - _RHEADER);
}

void *calloc(size_t n, size_t sz)
{
    size_t total = n * sz;
    void *p = malloc(total);
    if (p) { char *c = (char *)p; for (size_t i = 0; i < total; i++) c[i] = 0; }
    return p;
}

void *realloc(void *ptr, size_t size)
{
    if (!ptr) return malloc(size);
    unsigned char *raw = (unsigned char *)ptr - _RHEADER;
    size_t old_size = (size_t)raw[0] | ((size_t)raw[1]<<8) |
                      ((size_t)raw[2]<<16) | ((size_t)raw[3]<<24);
    unsigned char *nraw = (unsigned char *)_mos->malloc(size + _RHEADER);
    if (!nraw) return NULL;
    nraw[0] = (unsigned char)(size & 0xFF);
    nraw[1] = (unsigned char)((size >> 8) & 0xFF);
    nraw[2] = (unsigned char)((size >> 16) & 0xFF);
    nraw[3] = (unsigned char)((size >> 24) & 0xFF);
    size_t copy = old_size < size ? old_size : size;
    unsigned char *src = (unsigned char *)ptr;
    unsigned char *dst = nraw + _RHEADER;
    for (size_t i = 0; i < copy; i++) dst[i] = src[i];
    _mos->free(raw);
    return dst;
}

/* ── putchar ──────────────────────────────────────────────────────────── */

int putchar(int c)
{
    _mos->putch((uint8_t)c);
    return c;
}

/* ── errno / strerror (stubs) ─────────────────────────────────────────── */

int errno = 0;

char *strerror(int e)
{
    (void)e;
    return "error";
}

/* ── fflush_all (stub — MOS no tiene buffering de I/O) ───────────────── */

static inline void fflush_all(void) { /* nop */ }

#endif /* MOS_LIBC_IMPL */
#endif /* MOS_LIBC_H */
