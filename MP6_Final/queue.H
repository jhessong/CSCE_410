#ifndef QUEUE_H
#define QUEUE_H

#include "thread.H"

class Queue {
    private:
        Thread* thread;
        Queue* next;
    public:
        Queue() {
            thread = NULL;
            next = NULL;
        }

        Queue(Thread* new_thread) {
            thread = new_thread;
            next = NULL;
        }

        void enqueue (Thread* new_thread) {
            if(thread == NULL) thread = new_thread;
            else {
                if(next == NULL) next = new Queue(new_thread);
                else next->enqueue(new_thread);
            }
        }

        Thread* dequeue() {
            if(thread == NULL) return NULL;
            Thread* top = thread;
            if(next != NULL) {
                thread = next->thread;
                Queue* prev_next = next;
                next = next->next;
                delete prev_next;
            }
            else thread = NULL;
            return top;
        }
};

#endif
