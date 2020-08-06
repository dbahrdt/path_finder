// src: https://stackoverflow.com/a/12883731

#ifndef MASTER_ARBEIT_STOPWATCH_H
#define MASTER_ARBEIT_STOPWATCH_H
#include <chrono>
#include <iostream>
namespace pathFinder {
template <typename Clock = std::chrono::high_resolution_clock> class Stopwatch {
  typename Clock::time_point _last;

public:
  Stopwatch() : _last(Clock::now()) {}

  void reset() { *this = Stopwatch(); }

  typename Clock::duration elapsed() const { return Clock::now() - _last; }
  double elapsedMil() const { return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - _last).count(); }
  double elapsedMicro() const { return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - _last).count(); }
};
} // namespace pathFinder
#endif // MASTER_ARBEIT_STOPWATCH_H
