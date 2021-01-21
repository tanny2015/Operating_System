#ifndef _SC_H_
#define _SC_H_

#include "receiver.h"
void SCShutdown();
void SCWaitForShutdown();
void createSC(List* sharedList); 

#endif