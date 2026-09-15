#include "shared.h"
extern "C" int* value_a() { return &shared_value<int>(); }
