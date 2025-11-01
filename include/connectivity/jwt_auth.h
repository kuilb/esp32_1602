#ifndef JWT_AUTH_H
#define JWT_AUTH_H

#include "mydefine.h"
#include "myhader.h"



// 默认 LocationID
extern char* location;

extern char apiHost[128];
extern char kid[64];
extern char project_id[64];
extern char base64_key[256];

// Ed25519 种子（32 字节）
extern uint8_t seed32[32];

void init_jwt();

void generateSeed32();

String base64url_encode(const uint8_t* data, size_t len);

String generate_jwt(const String& kid, const String& project_id, const uint8_t* seed32);

#endif