#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef DEBUG
#define DEBUG_PRINT(format, args ...) printf(format, ## args)

#else
#define DEBUG_PRINT(format, args ...)

#endif /* DEBUG_PRINT */


#endif /* DEBUG_H_ */
