#include <errno.h>
#include <limits.h>

// AVR LIBC gives this a junk value because they didn't want to waste storage on a string
// we need to redefine to a sane one
#undef EILSEQ
#define EILSEQ 35


long long_arg(char ** strp) {
    char * pstr = *strp;
    long val = 0;
    char sign = 0;
    char c;
    errno = 0;
    while ((c = *pstr++)) {
        if (c >= '0' && c <= '9') {
            if (sign == 0) {
                sign = '+';
            }
            val = val * 10 + (c - '0');
        } else if (c == '-') {
            if (!sign) {
                sign = c;
            } else {
                errno = EILSEQ;
                break;
            }
        } else if (c == ' ' || c == '\n') {
            if (sign == 0) {
                continue;
            } else {
                break;
            }
        } else {
            errno = EILSEQ;
            break;
        }
    }
    *strp = pstr;
    if (sign == '-')
        return val * -1;
    return val;
}

int int_arg(char ** strp) {
    long val = long_arg(strp);
    if (errno) {
        return INT_MAX;
    }
    if (val < INT_MIN || val > INT_MAX) {
        errno = ERANGE;
        return INT_MAX;
    }
    return (int) val;
}