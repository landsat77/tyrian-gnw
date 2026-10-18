/*
 * The few C library functions OpenTyrian uses that the firmware ABI does
 * not export.
 */
#include <stddef.h>

static int lower(int c)
{
    return (c >= 'A' && c <= 'Z') ? c + 'a' - 'A' : c;
}

int ty_strncasecmp(const char *a, const char *b, size_t n)
{
    for (; n; n--, a++, b++) {
        int x = lower((unsigned char)*a), y = lower((unsigned char)*b);
        if (x != y || !x)
            return x - y;
    }
    return 0;
}

int ty_strcasecmp(const char *a, const char *b)
{
    return ty_strncasecmp(a, b, (size_t)-1);
}

/* sin / cos (cutscene rotations) and bsearch: not in the firmware ABI */
static double reduce(double x)
{
    const double two_pi = 6.283185307179586476925286766559;
    long k = (long)(x / two_pi);
    x -= (double)k * two_pi;
    if (x > 3.14159265358979323846)
        x -= two_pi;
    else if (x < -3.14159265358979323846)
        x += two_pi;
    return x;
}

double sin(double x)
{
    x = reduce(x);
    double x2 = x * x, term = x, sum = x;
    for (int n = 1; n < 12; n++) {
        term *= -x2 / (double)((2 * n) * (2 * n + 1));
        sum += term;
    }
    return sum;
}

double cos(double x)
{
    x = reduce(x);
    double x2 = x * x, term = 1.0, sum = 1.0;
    for (int n = 1; n < 12; n++) {
        term *= -x2 / (double)((2 * n - 1) * (2 * n));
        sum += term;
    }
    return sum;
}

void *bsearch(const void *key, const void *base, size_t n, size_t size,
              int (*cmp)(const void *, const void *))
{
    const char *lo = (const char *)base;
    while (n > 0) {
        const char *mid = lo + (n / 2) * size;
        int r = cmp(key, mid);
        if (r == 0)
            return (void *)mid;
        if (r > 0) {
            lo = mid + size;
            n -= n / 2 + 1;
        } else {
            n /= 2;
        }
    }
    return NULL;
}

/* libsupc++ (emergency exception pool tunables): no environment */
char *ty_getenv(const char *name)
{
    (void)name;
    return NULL;
}

unsigned long ty_strtoul(const char *s, char **end, int base)
{
    unsigned long v = 0;
    if (base == 0)
        base = 10;
    while (*s == ' ')
        s++;
    for (;; s++) {
        int d;
        if (*s >= '0' && *s <= '9') d = *s - '0';
        else if (*s >= 'a' && *s <= 'z') d = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'Z') d = *s - 'A' + 10;
        else break;
        if (d >= base)
            break;
        v = v * base + d;
    }
    if (end)
        *end = (char *)s;
    return v;
}

float sinf(float x)
{
    return (float)sin((double)x);
}

/* io/file.cpp (paths): no current directory */
char *getcwd(char *buf, size_t size)
{
    (void)buf;
    (void)size;
    return NULL;
}

char *stpcpy(char *dst, const char *src)
{
    while ((*dst = *src++) != '\0')
        dst++;
    return dst;
}

/* joystick angles (no joystick on the console): a plain series */
float atanf(float x)
{
    if (x > 1.0f)
        return 1.5707963f - atanf(1.0f / x);
    if (x < -1.0f)
        return -1.5707963f - atanf(1.0f / x);
    /* |x| <= 1 */
    float x2 = x * x;
    return x * (0.9998660f + x2 * (-0.3302995f + x2 * (0.1801410f + x2 * (-0.0851330f + x2 * 0.0208351f))));
}
