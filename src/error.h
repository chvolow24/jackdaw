#ifndef JDAW_ERROR_H
#define JDAW_ERROR_H

void error_register_signal_handlers();
void error_exit(const char *fmt, ...);

#endif
