#pragma once

#define REFLECT_CLASS(...)
#define REFLECT_MEMBER(...)
#define REFLECT_FUNCTION(...)

#define ALLOW_PRIVATE_REFLECT() friend void ::Parser();

extern void Parser();