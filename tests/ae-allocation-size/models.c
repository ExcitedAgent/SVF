#define ALLOC __attribute__((annotate("ALLOC_HEAP_RET")))
ALLOC __attribute__((annotate("AllocSize:Arg1"))) void *extent_good(unsigned long a, unsigned long n) { return 0; }
ALLOC __attribute__((annotate("AllocSize:Arg0*Arg1"))) void *extent_product(unsigned long a, unsigned long n) { return 0; }
ALLOC void *extent_missing(unsigned long n) { return 0; }
ALLOC __attribute__((annotate("AllocSize:UNKNOWN"))) void *extent_unknown(unsigned long n) { return 0; }
ALLOC __attribute__((annotate("AllocSize:Arg0junk"))) void *extent_malformed(unsigned long n) { return 0; }
ALLOC __attribute__((annotate("AllocSize:Arg0*"))) void *extent_trailing(unsigned long n) { return 0; }
ALLOC __attribute__((annotate("AllocSize:Arg0"), annotate("AllocSize:Arg1"))) void *extent_conflict(unsigned long a, unsigned long n) { return 0; }
ALLOC __attribute__((annotate("AllocSize:Arg7"))) void *extent_badindex(unsigned long n) { return 0; }
ALLOC __attribute__((annotate("AllocSize:Arg0"))) void *extent_pointer(void *n) { return 0; }
ALLOC __attribute__((annotate("AllocSize:Arg0"))) void *extent_wide(__int128 n) { return 0; }
ALLOC __attribute__((annotate("AllocSize:Arg0"))) void *extent_zero(unsigned long n) { return 0; }
ALLOC __attribute__((annotate("AllocSize:Arg0"))) void *extent_negative(unsigned long n) { return 0; }
