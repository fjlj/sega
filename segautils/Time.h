#pragma once

#include <stdint.h>

typedef uint64_t Microseconds;
typedef uint64_t Milliseconds;
typedef double Seconds;

Microseconds t_m2u(Milliseconds t);
Microseconds t_s2u(Seconds t);
Milliseconds t_u2m(Microseconds t);
Milliseconds t_s2m(Seconds t);
Seconds t_u2s(Microseconds t);
Seconds t_m2s(Milliseconds t);
