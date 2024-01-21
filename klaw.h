#ifndef KLAW_H
#define KLAW_H

#include "MKL05Z4.h"
#define S2 10
#define S3 11
#define S2_MASK (1<<10)
#define S3_MASK (1<<11)

void klaw_INIT(void);
void klaw_INT(void);

#endif