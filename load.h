#ifndef LOAD_H
#define LOAD_H

#include "calculations.h"

base *load_bases(const char *filename, unsigned *out_count);
target *load_targets(const char *filename, unsigned *out_count);

#endif