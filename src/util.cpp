#include <errno.h>
#include <limits.h>


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
        } else if (c == ' ' && sign == 0) {
            continue;
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
    if (val < INT_MIN || val > INT_MAX) {
        errno = ERANGE;
        return 0;
    }
    return (int) val;
}