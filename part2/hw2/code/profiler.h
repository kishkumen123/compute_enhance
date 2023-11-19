#pragma once

///////////////////////////////
// NOTE: Profiler
///////////////////////////////

// TODO:
// Clear profiler anchors, so that we can use it in games
// Introduce a back buffer to collect anchors for each frame. This way if we get a spike, we can pause and use a scrubber to look at the frame that spiked and analyize it

#ifndef PROFILER
#define PROFILER 0
#endif

#ifndef PROFILER_TIMER
#define PROFILER_TIMER __rdtsc()
#endif

#if PROFILER
struct ProfileAnchor{
    u64 tsc_elapsed_exclusive; // note: Does NOT include children
    u64 tsc_elapsed_inclusive; // note: DOES include children
    u64 hit_count;
    u64 bytes_processed;
    String8 label;
};
static ProfileAnchor profile_anchors[4096];
static u32 parent_anchor_index;

struct ProfileBlock{
    String8 label;
    u64 old_tsc_elapsed_inclusive;
    u64 start_tsc;
    u32 parent_index;
    u32 anchor_index;

    ProfileBlock(String8 label_, u32 anchor_index_, u64 byte_count){
        parent_index = parent_anchor_index;

        anchor_index = anchor_index_;
        label = label_;

        ProfileAnchor *anchor = profile_anchors + anchor_index;
        old_tsc_elapsed_inclusive = anchor->tsc_elapsed_inclusive;
        anchor->bytes_processed += byte_count;

        parent_anchor_index = anchor_index;
        start_tsc = PROFILER_TIMER;
    }

    ~ProfileBlock(void){
        u64 elapsed = PROFILER_TIMER - start_tsc;
        parent_anchor_index = parent_index;

        ProfileAnchor *Parent = profile_anchors + parent_index;
        ProfileAnchor *anchor = profile_anchors + anchor_index;

        Parent->tsc_elapsed_exclusive -= elapsed;
        anchor->tsc_elapsed_exclusive += elapsed;
        anchor->tsc_elapsed_inclusive = old_tsc_elapsed_inclusive + elapsed;
        ++anchor->hit_count;

        anchor->label = label;
    }
};

#define begin_timed_bandwidth(name, byte_count) ProfileBlock GLUE(block, __LINE__)(str8_literal(name), __COUNTER__ + 1, byte_count)
#define begin_timed_scope(name) begin_timed_bandwidth(name, 0)
#define begin_timed_function() begin_timed_scope(__FUNCTION__)
#define profiler_check_anchor_count() static_assert(__COUNTER__ <= array_count(profile_anchors), "Number of profiler anchors exceeds the size of profile_anchors")
#define end_profiler() profiler_check_anchor_count(); end_profiler_()

static void
print_anchor_data(u64 total_cpu_elapsed, u64 cpu_freq){
    for(u32 anchor_index = 0; anchor_index < ArrayCount(profile_anchors); ++anchor_index){
        ProfileAnchor *anchor = profile_anchors + anchor_index;
        if(anchor->tsc_elapsed_inclusive){
            f64 percent = 100.0 * ((f64)anchor->tsc_elapsed_exclusive / (f64)total_cpu_elapsed);
            print("  %s[%llu]: %llu (%.2f%%", anchor->label.str, anchor->hit_count, anchor->tsc_elapsed_exclusive, percent);
            if(anchor->tsc_elapsed_inclusive != anchor->tsc_elapsed_exclusive){
                f64 percent_with_children = 100.0 * ((f64)anchor->tsc_elapsed_inclusive / (f64)total_cpu_elapsed);
                print(", %.2f%% w/children", percent_with_children);
            }
            print(")");
            if(anchor->bytes_processed){
                f64 seconds = (f64)anchor->tsc_elapsed_inclusive / (f64)cpu_freq;
                f64 bytes_per_second = (f64)anchor->bytes_processed / seconds;
                f64 gigabytes_per_second = bytes_per_second / (f64)GB(1);
                f64 megabytes = (f64)anchor->bytes_processed / (f64)MB(1);
                print("  %.3fmb at %.2fgb/s", megabytes, gigabytes_per_second);
            }
            print("\n");
        }
    }
}

#else

#define print_anchor_data(...)
#define begin_timed_bandwidth(...)
#define begin_timed_scope(...)
#define begin_timed_function()
#define profiler_check_anchor_count()
#define end_profiler() end_profiler_()

#endif

static u64
estimate_cpu_frequency(void){
	u64 ms_to_wait = 100;

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
	u64 os_freq = (u64)frequency.QuadPart;

	u64 cpu_start = PROFILER_TIMER;

    LARGE_INTEGER QPC;
    QueryPerformanceCounter(&QPC);
	u64 os_start = (u64)QPC.QuadPart;
	u64 os_end = 0;
	u64 os_elapsed = 0;
	u64 os_wait_time = os_freq * ms_to_wait / 1000;
	while(os_elapsed < os_wait_time){
        QueryPerformanceCounter(&QPC);
        os_end = (u64)QPC.QuadPart;
		os_elapsed = os_end - os_start;
	}

	u64 cpu_end = PROFILER_TIMER;
	u64 cpu_elapsed = cpu_end - cpu_start;

	u64 cpu_freq = 0;
	if(os_elapsed){
		cpu_freq = os_freq * cpu_elapsed / os_elapsed;
	}

	return cpu_freq;
}

struct Profiler{
    u64 start_tsc;
    u64 end_tsc;
};
static Profiler profiler;

static void
begin_profiler(void){
    profiler.start_tsc = PROFILER_TIMER;
}

static void
end_profiler_(void){
    profiler.end_tsc = PROFILER_TIMER;
    u64 total_cpu_elapsed = profiler.end_tsc - profiler.start_tsc;

    u64 cpu_freq = estimate_cpu_frequency();
    if(cpu_freq){
        print("\nTotal time: %0.4fms (CPU freq %llu)\n", 1000.0 * (f64)total_cpu_elapsed / (f64)cpu_freq, cpu_freq);
    }

    print_anchor_data(total_cpu_elapsed, cpu_freq);
}
