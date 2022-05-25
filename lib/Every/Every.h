#pragma once

#define LIB8STATIC __attribute__((unused)) static inline
#define GET_MILLIS millis

/// Return the current seconds since boot in a 16-bit value.  Used as part of the
/// "every N time-periods" mechanism
LIB8STATIC uint16_t seconds16()
{
    uint32_t ms = GET_MILLIS();
    uint16_t s16;
    s16 = ms / 1000;
    return s16;
}

/// Return the current minutes since boot in a 16-bit value.  Used as part of the
/// "every N time-periods" mechanism
LIB8STATIC uint16_t minutes16()
{
    uint32_t ms = GET_MILLIS();
    uint16_t m16;
    m16 = (ms / (60000L)) & 0xFFFF;
    return m16;
}

/// Return the current hours since boot in an 8-bit value.  Used as part of the
/// "every N time-periods" mechanism
LIB8STATIC uint8_t hours8()
{
    uint32_t ms = GET_MILLIS();
    uint8_t h8;
    h8 = (ms / (3600000L)) & 0xFF;
    return h8;
}

/// Helper routine to divide a 32-bit value by 1024, returning
/// only the low 16 bits. You'd think this would be just
///   result = (in32 >> 10) & 0xFFFF;
/// and on ARM, that's what you want and all is well.
/// But on AVR that code turns into a loop that executes
/// a four-byte shift ten times: 40 shifts in all, plus loop
/// overhead. This routine gets exactly the same result with
/// just six shifts (vs 40), and no loop overhead.
/// Used to convert millis to 'binary seconds' aka bseconds:
/// one bsecond == 1024 millis.
LIB8STATIC uint16_t div1024_32_16(uint32_t in32)
{
    uint16_t out16;
#if defined(__AVR__)
    asm volatile(
        "  lsr %D[in]  \n\t"
        "  ror %C[in]  \n\t"
        "  ror %B[in]  \n\t"
        "  lsr %D[in]  \n\t"
        "  ror %C[in]  \n\t"
        "  ror %B[in]  \n\t"
        "  mov %B[out],%C[in] \n\t"
        "  mov %A[out],%B[in] \n\t"
        : [in] "+r"(in32),
        [out] "=r"(out16));
#else
    out16 = (in32 >> 10) & 0xFFFF;
#endif
    return out16;
}

/// bseconds16 returns the current time-since-boot in
/// "binary seconds", which are actually 1024/1000 of a
/// second long.
LIB8STATIC uint16_t bseconds16()
{
    uint32_t ms = GET_MILLIS();
    uint16_t s16;
    s16 = div1024_32_16(ms);
    return s16;
}

// Classes to implement "Every N Milliseconds", "Every N Seconds",
// "Every N Minutes", "Every N Hours", and "Every N BSeconds".
#define INSTANTIATE_EVERY_N_TIME_PERIODS(NAME, TIMETYPE, TIMEGETTER) \
    class NAME {                                                     \
    public:                                                          \
        TIMETYPE mPrevTrigger;                                       \
        TIMETYPE mPeriod;                                            \
                                                                     \
        NAME()                                                       \
        {                                                            \
            reset();                                                 \
            mPeriod = 1;                                             \
        };                                                           \
        NAME(TIMETYPE period)                                        \
        {                                                            \
            reset();                                                 \
            setPeriod(period);                                       \
        };                                                           \
        void setPeriod(TIMETYPE period) { mPeriod = period; };       \
        TIMETYPE getTime() { return (TIMETYPE)(TIMEGETTER()); };     \
        TIMETYPE getPeriod() { return mPeriod; };                    \
        TIMETYPE getElapsed() { return getTime() - mPrevTrigger; }   \
        TIMETYPE getRemaining() { return mPeriod - getElapsed(); }   \
        TIMETYPE getLastTriggerTime() { return mPrevTrigger; }       \
        bool ready()                                                 \
        {                                                            \
            bool isReady = (getElapsed() >= mPeriod);                \
            if (isReady) {                                           \
                reset();                                             \
            }                                                        \
            return isReady;                                          \
        }                                                            \
        void reset() { mPrevTrigger = getTime(); };                  \
        void trigger() { mPrevTrigger = getTime() - mPeriod; };      \
                                                                     \
        operator bool() { return ready(); }                          \
    };
INSTANTIATE_EVERY_N_TIME_PERIODS(CEveryNMillis, uint32_t, GET_MILLIS);
INSTANTIATE_EVERY_N_TIME_PERIODS(CEveryNSeconds, uint16_t, seconds16);
INSTANTIATE_EVERY_N_TIME_PERIODS(CEveryNBSeconds, uint16_t, bseconds16);
INSTANTIATE_EVERY_N_TIME_PERIODS(CEveryNMinutes, uint16_t, minutes16);
INSTANTIATE_EVERY_N_TIME_PERIODS(CEveryNHours, uint8_t, hours8);

#define CONCAT_HELPER(x, y) x##y
#define CONCAT_MACRO(x, y) CONCAT_HELPER(x, y)
#define EVERY_N_MILLIS(N) EVERY_N_MILLIS_I(CONCAT_MACRO(PER, __COUNTER__), N)
#define EVERY_N_MILLIS_I(NAME, N) \
    static CEveryNMillis NAME(N); \
    if (NAME)
#define EVERY_N_SECONDS(N) EVERY_N_SECONDS_I(CONCAT_MACRO(PER, __COUNTER__), N)
#define EVERY_N_SECONDS_I(NAME, N) \
    static CEveryNSeconds NAME(N); \
    if (NAME)
#define EVERY_N_BSECONDS(N) EVERY_N_BSECONDS_I(CONCAT_MACRO(PER, __COUNTER__), N)
#define EVERY_N_BSECONDS_I(NAME, N) \
    static CEveryNBSeconds NAME(N); \
    if (NAME)
#define EVERY_N_MINUTES(N) EVERY_N_MINUTES_I(CONCAT_MACRO(PER, __COUNTER__), N)
#define EVERY_N_MINUTES_I(NAME, N) \
    static CEveryNMinutes NAME(N); \
    if (NAME)
#define EVERY_N_HOURS(N) EVERY_N_HOURS_I(CONCAT_MACRO(PER, __COUNTER__), N)
#define EVERY_N_HOURS_I(NAME, N) \
    static CEveryNHours NAME(N); \
    if (NAME)