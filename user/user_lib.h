/*
 * header file to be used by applications.
 */

int printu(const char *s, ...);
int exit(int code);
void* naive_malloc();
void naive_free(void* va);
int fork();
void yield();
void print_backtrace(int depth);
ssize_t wait(int pid);
ssize_t sem_new(int val);
void sem_P(int ind);
void sem_V(int ind);