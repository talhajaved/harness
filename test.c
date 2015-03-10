/* #define NOSYSCALL */
#ifdef NOSYSCALL
int getpid() { return 55; }
#endif
int main( int argc, char * argv[] ) {
 int i, a, limit = atoi(argv[1]);
 for( i = 0; i < limit; ++i ) a = getpid();
}