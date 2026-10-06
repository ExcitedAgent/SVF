extern int missing(int);
static int helper(int x) { return missing(x); }
int compute(int x) { return helper(x); }
