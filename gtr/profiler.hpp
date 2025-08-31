#pragma once
#include <unordered_map>

#ifdef _MSC_VER
#include <intrin.h>
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
namespace gtr::profiler {
static uint64_t GetOSTimerFreq() {
    LARGE_INTEGER Freq;
    QueryPerformanceFrequency(&Freq);
    return Freq.QuadPart;
}

static uint64_t ReadOSTimer() {
    LARGE_INTEGER Value;
    QueryPerformanceCounter(&Value);
    return Value.QuadPart;
}
} // namespace gtr::profiler
#else
#include <time.h>
#include <x86intrin.h>
namespace gtr::profiler {
static uint64_t GetOSTimerFreq(void) { return 1000000000ULL; }
static uint64_t ReadOSTimer(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    // Convert to a single 64-bit tick count:
    return static_cast<uint64_t>(ts.tv_sec) * GetOSTimerFreq() + static_cast<uint64_t>(ts.tv_nsec);
}
} // namespace gtr::profiler
#endif
namespace gtr::profiler {
inline uint64_t ReadCPUTimer() { return __rdtsc(); }

static uint64_t EstimateCPUTimerFreq(void) {
    constexpr uint64_t MillisecondsToWait = 100;
    const uint64_t OSFreq = GetOSTimerFreq();

    const uint64_t CPUStart = ReadCPUTimer();
    const uint64_t OSStart = ReadOSTimer();
    uint64_t OSEnd = 0;
    uint64_t OSElapsed = 0;
    const uint64_t OSWaitTime = OSFreq * MillisecondsToWait / 1000;
    while (OSElapsed < OSWaitTime) {
        OSEnd = ReadOSTimer();
        OSElapsed = OSEnd - OSStart;
    }

    const uint64_t CPUEnd = ReadCPUTimer();
    const uint64_t CPUElapsed = CPUEnd - CPUStart;

    uint64_t CPUFreq = 0;
    if (OSElapsed) {
        CPUFreq = OSFreq * CPUElapsed / OSElapsed;
    }
    return CPUFreq;
}

struct profile_anchor {
    uint64_t TSCElapsedExclusive; // NOTE(casey): Does NOT include children
    uint64_t TSCElapsedInclusive; // NOTE(casey): DOES include children
    uint64_t HitCount;
    char Label[256];
};

static void BeginProfile(void);

struct profiler {
    profile_anchor Anchors[4096];
    profiler() {
        BeginProfile();
        CPUFreq = EstimateCPUTimerFreq();
    }
    uint64_t CPUFreq;
    uint64_t StartTSC{0};
    uint64_t EndTSC{0};
};
extern profiler GlobalProfiler;
extern uint32_t GlobalProfilerParent;

struct profile_block {
    profile_block(const char *Label_, const uint32_t AnchorIndex_) {
        ParentIndex = GlobalProfilerParent;
        AnchorIndex = AnchorIndex_;
        Label = Label_;

        profile_anchor *Anchor = GlobalProfiler.Anchors + AnchorIndex;
        OldTSCElapsedInclusive = Anchor->TSCElapsedInclusive;

        GlobalProfilerParent = AnchorIndex;
        StartTSC = ReadCPUTimer();
    }

    ~profile_block(void) {
        const uint64_t Elapsed = ReadCPUTimer() - StartTSC;
        GlobalProfilerParent = ParentIndex;

        profile_anchor *Parent = GlobalProfiler.Anchors + ParentIndex;
        profile_anchor *Anchor = GlobalProfiler.Anchors + AnchorIndex;

        Parent->TSCElapsedExclusive -= Elapsed;
        Anchor->TSCElapsedExclusive += Elapsed;
        Anchor->TSCElapsedInclusive = OldTSCElapsedInclusive + Elapsed;
        ++Anchor->HitCount;

        strcpy(Anchor->Label, Label);
    }

    char const *Label;
    uint64_t OldTSCElapsedInclusive;
    uint64_t StartTSC;
    uint32_t ParentIndex;
    uint32_t AnchorIndex;
};

#define NameConcat2(A, B) A##B
#define NameConcat(A, B) NameConcat2(A, B)
#define TimeBlock(name)                                                                                                                                                            \
    static const uint32_t NameConcat(_anchor_, __LINE__) = gtr::profiler::register_anchor(__FILE__, __LINE__);                                                                     \
    gtr::profiler::profile_block NameConcat(_block_, __LINE__) { name, NameConcat(_anchor_, __LINE__) }
#define TimeFunction TimeBlock(__func__)

inline uint32_t register_anchor(const char *file, uint32_t line) {
    struct profile_block_key {
        char file[128];
        uint32_t line;
        bool operator==(const profile_block_key &other) const { return strncmp(file, other.file, 128) == 0 && line == other.line; }
    };

    struct profile_block_key_hash {
        uint64_t operator()(const profile_block_key &key) const { return std::hash<const char *>()(key.file) ^ std::hash<uint32_t>()(key.line); }
    };

    profile_block_key key{};
    strcpy(key.file, file);
    key.line = line;
    static std::unordered_map<profile_block_key, uint32_t, profile_block_key_hash> global_profiler_anchor;
    static uint32_t next_index = 1;
    if (const auto it = global_profiler_anchor.find(key); it != global_profiler_anchor.end()) {
        // If the anchor already exists, use its index
        return it->second;
    }
    global_profiler_anchor[key] = next_index;
    return next_index++;
}

inline void PrintTimeElapsed(char *in, uint64_t TotalTSCElapsed, profile_anchor *Anchor) {
    double Percent = 100.0 * (static_cast<double>(Anchor->TSCElapsedInclusive) / static_cast<double>(TotalTSCElapsed));

    const double avg_ticks = static_cast<double>(Anchor->TSCElapsedInclusive) / static_cast<double>(Anchor->HitCount);

    double avg_us = 1000000 * avg_ticks / static_cast<double>(GlobalProfiler.CPUFreq);

    char label1[128]{};
    char label2[128]{};
    std::sprintf(label1, "  %s[%llu]: %llu (%.2f%%, avg inc. %.4lfus", Anchor->Label, Anchor->HitCount, Anchor->TSCElapsedExclusive, Percent, avg_us);
    if (Anchor->TSCElapsedInclusive != Anchor->TSCElapsedExclusive) {
        const double PercentWithChildren = 100.0 * (static_cast<double>(Anchor->TSCElapsedInclusive) / static_cast<double>(TotalTSCElapsed));
        std::sprintf(label2, ", %.2f%% w/children", PercentWithChildren);
    }
    std::strcat(in, label1);
    std::strcat(in, label2);
    std::strcat(in, ")\n");
}

inline void BeginProfile(void) { GlobalProfiler.StartTSC = ReadCPUTimer(); }
inline void Reset() {
    BeginProfile();

    for (uint32_t AnchorIndex = 0; AnchorIndex < std::size(GlobalProfiler.Anchors); ++AnchorIndex) {
        profile_anchor *Anchor = GlobalProfiler.Anchors + AnchorIndex;
        std::memset(static_cast<void *>(Anchor), 0, sizeof(profile_anchor));
    }
}

inline thread_local char profile_output[1024];

inline char *EndAndPrintProfile() {
    GlobalProfiler.EndTSC = ReadCPUTimer();
    memset(profile_output, 0, sizeof(profile_output));
    uint64_t TotalCPUElapsed = GlobalProfiler.EndTSC - GlobalProfiler.StartTSC;
    if (GlobalProfiler.CPUFreq) {
        std::sprintf(profile_output, " Total time : % 0.4fms(CPU freq % llu)\n", 1000.0 * static_cast<double>(TotalCPUElapsed) / static_cast<double>(GlobalProfiler.CPUFreq),
                     GlobalProfiler.CPUFreq);
    }

    for (uint32_t AnchorIndex = 0; AnchorIndex < std::size(GlobalProfiler.Anchors); ++AnchorIndex) {
        if (profile_anchor *Anchor = GlobalProfiler.Anchors + AnchorIndex; Anchor->TSCElapsedInclusive) {
            PrintTimeElapsed(profile_output, TotalCPUElapsed, Anchor);
        }
    }

    return profile_output;
}
} // namespace gtr::profiler