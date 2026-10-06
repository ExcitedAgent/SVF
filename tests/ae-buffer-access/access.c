extern void *malloc(unsigned long);
extern void *memmove(void *, const void *, unsigned long);

#if defined(REPEATED)
static char *access(char *buffer, long index) { return buffer + index; }
#endif

int main(int argc, char **argv)
{
#if defined(HEAP)
    char *buffer = malloc(8);
#elif defined(DYNAMIC)
    int length = 8;
    char buffer[length];
#else
    char buffer[8];
#endif
#if defined(COPY) || defined(COPY_SAFE)
    char destination[4];
#if defined(COPY_SAFE)
    memmove(destination, buffer, 4);
#else
    memmove(destination, buffer, 8);
#endif
    return destination[0];
#elif defined(SAFE)
    char *volatile result = buffer + 7;
#elif defined(BOUNDARY)
    char *volatile result = buffer + 8;
#elif defined(NESTED)
    char *base = buffer + 2;
    char *volatile result = base + 7;
#elif defined(RANGE)
    char *volatile result = buffer + (argc > 1 ? 8 : 9);
#elif defined(REPEATED)
    char *volatile result = access(buffer, 8);
    result = access(buffer, 9);
#else
    char *volatile result = buffer + 9;
#endif
#if !defined(COPY) && !defined(COPY_SAFE)
    return result != 0;
#endif
}
