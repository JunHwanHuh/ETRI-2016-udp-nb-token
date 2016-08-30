#include <sys/time.h>
#include "mtime.h"

long long int GetTimeStamp() {
    long long int timestamp_usec; /* timestamp in microsecond */
    struct timeval timer_usec; 
    if (!gettimeofday(&timer_usec, NULL)) {
    timestamp_usec = ((long long int) timer_usec.tv_sec) * 1000000ll + 
                        (long long int) timer_usec.tv_usec;
    }
    else {
      timestamp_usec = -1;
    }
    return timestamp_usec;
}