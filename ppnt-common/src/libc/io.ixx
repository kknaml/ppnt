export module ppnt.libc:io;


export extern "C" {
    struct _IO_FILE;
    using FILE = _IO_FILE;
    extern FILE *stdin;
    extern FILE *stdout;
    extern FILE *stderr;

}
