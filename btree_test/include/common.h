#ifndef _COMMON_H_
#define _COMMON_H_

#undef PDEBUG
#ifdef TREETESTDEBUG
# ifdef __KERNEL__
#include <linux/kernel.h>
#   define PDEBUG(fmt, ...) printk(KERN_DEBUG  fmt,  ##__VA_ARGS__)
#   define PERROR(fmt, ...) printk(KERN_ERR "[treetestfs] file:%s line%d : " __FILE__, __LINE__, ##__VA_ARGS__ )
# else
#include <stdio.h>
#   define PDEBUG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#   define PERROR(fmt, ...) fprintf(stderr, "[treetestfs] file:%s line%d : "fmt, __FILE__, __LINE__, ##__VA_ARGS__)
# endif
#else
#define PDEBUG(fmt, args...)
#define PERROR(fmt, args...)
#endif

#endif
