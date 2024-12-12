/* $begine sbuf.h */
#ifndef __SBUF_H__
#define __SBUF_H__

#include <semaphore.h>

typedef struct
{
    int *buf;    /* Buffer array */
    int n;       /* Maximum number of slots */
    int front;   /* buf[front % n] is the first item */
    int rear;    /* buf[(rear - 1) % n] is the last item*/
    sem_t mutex; /* Protect access to buf */
    sem_t slots; /* Count available slots */
    sem_t items; /* Count available items */
} sbuf_t;

void sbuf_init(sbuf_t *sp, int n);
void sbuf_destroy(sbuf_t *sp);
void sbuf_enque(sbuf_t *sp, int item);
int sbuf_deque(sbuf_t *sp);

#endif /* __SBUF_H__ */
/* $end sbuf.h */
