#include "shared.h"
extern "C" int* value_b() { return &shared_value<int>(); }
