#pragma once

#include <chrono>

struct BenchmarkTimes {
    using TimePoint = std::chrono::high_resolution_clock::time_point;

    TimePoint tStart;
    TimePoint tSchema;
    TimePoint tCache;
    TimePoint tStructs;
    TimePoint tHeaders;
    TimePoint tFunctions;
    TimePoint tParameters;
    TimePoint tConsts;
    TimePoint tEnums;
    TimePoint tIncludes;
    TimePoint tExportMaps;
    TimePoint tEnd;

    static auto mark() -> TimePoint {
        return std::chrono::high_resolution_clock::now();
    }

    static auto delta(TimePoint a, TimePoint b) -> double {
        return std::chrono::duration<double>(b - a).count();
    }
};
