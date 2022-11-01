#ifndef LCRYP_RANDOMENV_H
#define LCRYP_RANDOMENV_H
#include <crypto/sha512.h>
void RandAddDynamicEnv(CSHA512& hasher);
void RandAddStaticEnv(CSHA512& hasher);
#endif
