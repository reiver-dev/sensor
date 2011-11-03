#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef DEBUG
#define DEBUG_PRINTF(format, args ...) printf(format, ## args)
#define DEBUG_PRINT(format) printf(format);

#else
#define DEBUG_PRINTF(format, args ...)
#define DEBUG_PRINT(format)

#endif /* DEBUG_PRINT */


#endif /* DEBUG_H_ */
