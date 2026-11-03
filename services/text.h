#ifndef KITCHEN_TEXT_H
#define KITCHEN_TEXT_H
#include <stddef.h>
/* Bounded UI-only formatter: %s %d %u %lu %%, no allocation or stdio locks.
 * Returns required characters excluding NUL, like snprintf. */
size_t text_format(char *dst,size_t capacity,const char *format,...);
#endif
