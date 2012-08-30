#ifndef DEBUG_H_
#define DEBUG_H_

#define DEBUG_LEVEL_ERROR 0
#define DEBUG_LEVEL_WARNING 1
#define DEBUG_LEVEL_NOTIFY 2
#define DEBUG_LEVEL_INFO 3

extern "C" {

#ifdef DEBUG

//-----------------------------

void debug(int level, const char *function, const char *format, ...);
void debug_set_level(int lvl);
void debug_append(int level, const char *format, ...);

#define DEBUG_LVL(lvl) debug_set_level(int lvl);

#define DINFO(fmt, ...)    debug(DEBUG_LEVEL_INFO,    __FUNCTION__, fmt, ##__VA_ARGS__)
#define DNOTIFY(fmt, ...)  debug(DEBUG_LEVEL_NOTIFY,  __FUNCTION__, fmt, ##__VA_ARGS__)
#define DWARNING(fmt, ...) debug(DEBUG_LEVEL_WARNING, __FUNCTION__, fmt, ##__VA_ARGS__)
#define DERROR(fmt, ...)   debug(DEBUG_LEVEL_ERROR,   __FUNCTION__, fmt, ##__VA_ARGS__)

#define DINFOA(fmt, ...)    debug_append(DEBUG_LEVEL_INFO,    fmt, ##__VA_ARGS__)
#define DNOTIFYA(fmt, ...)  debug_append(DEBUG_LEVEL_NOTIFY,  fmt, ##__VA_ARGS__)
#define DWARNINGA(fmt, ...) debug_append(DEBUG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define DERRORA(fmt, ...)   debug_append(DEBUG_LEVEL_ERROR,   fmt, ##__VA_ARGS__)

#else

#define DINFO(fmt, ...)
#define DNOTIFY(fmt, ...)
#define DWARNING(fmt, ...)
#define DERROR(fmt, ...)

#define DINFOA(fmt, ...)
#define DNOTIFYA(fmt, ...)
#define DWARNINGA(fmt, ...)
#define DERRORA(fmt, ...)

#endif /* DEBUG */

}

#endif /* DEBUG_H_ */
