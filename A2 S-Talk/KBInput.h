#ifndef _KBInput_H_
#define _KBInput_H_
#include "sender.h"

void KBShutdown();
void createKB(List* sharedList);
void KBWaitForShutdown();

#endif