// LLVM terminal capability calls are unavailable in the iOS app.
int setupterm(char *term, int fildes, int *errret) { (void)term; (void)fildes; if (errret) *errret = -1; return -1; }
void *set_curterm(void *nterm) { (void)nterm; return 0; }
int del_curterm(void *oterm) { (void)oterm; return 0; }
int tigetnum(char *capname) { (void)capname; return -1; }
