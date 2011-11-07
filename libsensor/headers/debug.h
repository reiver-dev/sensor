#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)

#else
#define DEBUG_PRINTF(...)
#endif /* DEBUG_PRINT */


#endif /* DEBUG_H_ */
