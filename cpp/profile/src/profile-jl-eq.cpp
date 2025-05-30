#include <boost/json.hpp>
#include <boost/json/src.hpp>
#include <cstdio>
#include <dlfcn.h>
#include <faker-cxx/location.h>
#include <faker-cxx/number.h>
#include <iostream>
#include <jsonlogic/logic.hpp>
#include <span>
#include <string>

const unsigned long SEED_ = 42;
static const size_t N_ = 1'000'000;
static const int N_RUNS_ = 3;
int main(int argc, const char **argv) try {
  std::span<const char *> args(argv, argc);

  size_t N = N_;
  if (argc > 1) {
    N = std::stoul(args[1]);
  }
  size_t N_RUNS = N_RUNS_;
  if (argc > 2) {
    N_RUNS = std::stoul(args[2]);
  }

  size_t SEED = SEED_;
  if (argc > 3) {
    SEED = std::stoul(args[3]);
  }

  faker::getGenerator().seed(SEED);
  std::vector<uint64_t> xs;
  xs.reserve(N);
  std::vector<uint64_t> ys;
  ys.reserve(N);

  for (size_t nruns = 0; nruns < N_RUNS; ++nruns) {
    // Create data
    for (size_t i = 0; i < N; ++i) {
      xs.push_back(faker::number::integer<uint64_t>(0, 255));
      ys.push_back(faker::number::integer<uint64_t>(0, 255));
    }

    //   // Create jsonlogic benchmark
    std::string expr_str_xy = R"({"==":[{"var": "x"},{"var": "y"}]})";
    auto jv_xy = boost::json::parse(expr_str_xy);

    // JL 2

    size_t matches = 0;

    for (size_t i = 0; i < N; ++i) {
      auto [rule, _a, _b] = jsonlogic::create_logic(jv_xy);
      auto v_xy = jsonlogic::apply(rule, {xs[i], ys[i]});
      bool val = jsonlogic::truthy(v_xy);
      if (val) {
        ++matches;
      }
    }
    if (nruns > N_RUNS - 5) {
      std::cout << "matches: " << matches << "\n";
    }
  }
  return 0;
} catch (const std::exception &e) {
  std::cerr << "Fatal error: " << e.what() << '\n';
  return 1;
} catch (...) {
  std::cerr << "Fatal unkown error\n";
  return 2;
}
