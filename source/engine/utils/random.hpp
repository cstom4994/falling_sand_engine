#ifndef ME_RANDOM_H
#define ME_RANDOM_H

#include "engine/core/core.hpp"

class CLGMRandom {
public:
    void SetSeed(double seed);

    CLGMRandom() : seed(0) {}
    CLGMRandom(double seed) { SetSeed(seed); }

    double operator()() { return Next(); }

    //! returns a random between 0 and 1
    double Next();

    float Randomf(float low, float high) { return low + ((high - low) * (float)Next()); }

    int Random(int low, int high) { return low + (int)((double)(high - low + 1) * (double)Next()); }

    int RandomSafe(int range0, int range1) { return Random(range0 < range1 ? range0 : range1, range0 > range1 ? range0 : range1); }

    float RandomfSafe(float range0, float range1) { return Randomf(range0 < range1 ? range0 : range1, range0 > range1 ? range0 : range1); }

    double seed;
};

inline void CLGMRandom::SetSeed(double s) {
    if (s >= 2147483647.0) s = s / 2.0;

    seed = s;
    Next();
}

inline double CLGMRandom::Next() {
    // ME_ASSERT(seed);
    //   m = 2147483647 = 2^31 - 1; a = 16807;
    //   127773 = m div a; 2836 = m mod a
    long iseed = (long)seed;
    long hi = iseed / 127773L;       // integer division
    long lo = iseed - hi * 127773L;  // modulo
    iseed = 16807 * lo - 2836 * hi;
    if (iseed <= 0) iseed += 2147483647L;
    seed = (double)iseed;
    return seed * 4.656612875e-10;
}

void InitializeProceduralRandom(CLGMRandom& rng, double x, double y, unsigned int seed) {
    auto SomeHashFunc = [](unsigned int xh, unsigned int zh, unsigned int seed) {
        unsigned v3 = (seed >> 13) ^ (xh - zh - seed);
        unsigned v4 = (v3 << 8) ^ (zh - v3 - seed);
        unsigned v5 = (v4 >> 13) ^ (seed - v3 - v4);
        unsigned v6 = (v5 >> 12) ^ (v3 - v4 - v5);
        unsigned v7 = (v6 << 16) ^ (v4 - v6 - v5);
        unsigned v8 = (v7 >> 5) ^ (v5 - v6 - v7);
        unsigned v9 = (v8 >> 3) ^ (v6 - v7 - v8);
        return (((v9 << 10) ^ (v7 - v9 - v8)) >> 15) ^ (v8 - v9 - ((v9 << 10) ^ (v7 - v9 - v8)));
    };

    auto xh = ((seed ^ 11887) & 0xFFF) + 0.0 + x;
    auto yh = (((seed ^ 2468753007) >> 12) & 0xFFF) + 0.0 + y;

    bool h = fabs(yh) >= 102400.0 || fabs(xh) <= 1.0;

    unsigned xh1 = (long long)(xh * 134217727.0);
    unsigned yh1 = (long long)(h ? yh * 134217727.0 : (yh * 3483.328 + xh1) * yh);

    auto hash = SomeHashFunc(xh1, yh1, seed);

    rng.SetSeed(hash / 4294967295.0 * 2147483639.0 + 1.0);

    for (int i = 0; i < (seed & 3); ++i) rng.Next();
}

#endif