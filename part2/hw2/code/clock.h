#pragma once

///////////////////////////////
// NOTE: Clock
///////////////////////////////
typedef u64 GetTicks(void);
typedef f64 GetSecondsElapsed(u64 start, u64 end);
typedef f64 GetMsElapsed(u64 start, u64 end);
typedef u64 GetCycles(void);

struct Clock{
    f64 dt;
    u64 frequency; // NOTE: How many ticks there are per second
    GetTicks* get_ticks;
    GetSecondsElapsed* get_seconds_elapsed;
    GetMsElapsed* get_ms_elapsed;
    GetCycles* get_cycles;
};
static Clock clock = {0};

static u64
get_os_timer_frequency(void){
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return((u64)frequency.QuadPart);
}

#define read_os_timer() get_ticks()
static u64
get_ticks(){
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return((u64)result.QuadPart);
}

#define read_cpu_timer() get_cycles()
static u64
get_cycles(){
    return __rdtsc();
}

static f64
get_seconds_elapsed(u64 start, u64 end){
    f64 result;
    result = ((f64)(end - start) / ((f64)clock.frequency));
    return(result);
}

static f64
get_ms_elapsed(u64 start, u64 end){
    f64 result;
    result = (1000 * ((f64)(end - start) / ((f64)clock.frequency)));
    return(result);
}

static void init_clock(Clock* clock){
    clock->frequency = get_os_timer_frequency();

    clock->get_ticks = get_ticks;
    clock->get_seconds_elapsed = get_seconds_elapsed;
    clock->get_ms_elapsed = get_ms_elapsed;
    clock->get_cycles = get_cycles;
}
