
#ifndef SHELL_CODE_H
#define SHELL_CODE_H

struct nel_eng ;
extern int shellcode_init(struct nel_eng *eng);
extern int nel_has_shellcode(unsigned char *data, int len);


#endif
