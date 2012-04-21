#ifndef DEBUG_H_
#define DEBUG_H_

#define DEBUG_LEVEL_ERROR 0
#define DEBUG_LEVEL_WARNING 1
#define DEBUG_LEVEL_NOTIFY 2
#define DEBUG_LEVEL_INFO 3


#ifdef DEBUG

//-----------------------------

void debug(int level, const char *function, char *format, ...);
void debug_set_level(int lvl);

#define DEBUG_LVL(lvl) debug_set_level(int lvl);

#define DINFO(fmt, ...)    debug(DEBUG_LEVEL_INFO,    __FUNCTION__, fmt, ##__VA_ARGS__)
#define DNOTIFY(fmt, ...)  debug(DEBUG_LEVEL_NOTIFY,  __FUNCTION__, fmt, ##__VA_ARGS__)
#define DWARNING(fmt, ...) debug(DEBUG_LEVEL_WARNING, __FUNCTION__, fmt, ##__VA_ARGS__)
#define DERROR(fmt, ...)   debug(DEBUG_LEVEL_ERROR,   __FUNCTION__, fmt, ##__VA_ARGS__)


#else

#define DINFO(fmt, ...)
#define DNOTIFY(fmt, ...)
#define DWARNING(fmt, ...)
#define DERROR(fmt, ...)


#endif /* DEBUG */


#endif /* DEBUG_H_ */
