#include <bench.hpp>
#include <boost/json.hpp>
#include <boost/json/src.hpp>
#include <cstdio>
#include <faker-cxx/location.h>
#include <faker-cxx/number.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <jsonlogic/logic.hpp>
#include <string>
#include <vector>

std::string read_file(const std::string &filename) {
  std::ifstream file(filename);
  if (!file)
    throw std::runtime_error("Failed to open file");

  return {std::string((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>())};
}

const unsigned long SEED_ = 42;
static const size_t N_ = 1'000'000;
static const int N_RUNS_ = 3;

int main(int argc, const char **argv) try {

  // Test expression: x > 5 and y < 3
  std::string expr;
  try {
    expr = read_file("simple-and.json");
    std::cout << "Successfully read simple-and.json from current directory"
              << std::endl;
  } catch (const std::exception &) {
    try {
      expr = read_file("bench/src/simple-and.json");
      std::cout << "Successfully read simple-and.json from bench/src/"
                << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "Error: Could not find simple-and.json: " << e.what()
                << std::endl;
      // Print current working directory
      std::cerr << "Current directory: " << std::filesystem::current_path()
                << std::endl;
      throw;
    }
  }

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
  std::vector<double> xs;
  xs.reserve(N);
  std::vector<int> ys;
  ys.reserve(N);

  // Create test data
  for (size_t i = 0; i < N; ++i) {
    xs.push_back(faker::number::decimal<double>(
        0, 10)); // 0-10 range to get some matches
    ys.push_back(
        faker::number::integer<int>(0, 6)); // 0-6 range to get some matches
  }

  // JL1 - Using boost::json::object approach

  auto jv_expr = boost::json::parse(expr);
  boost::json::object data_obj;
  jsonlogic::logic_rule rule = jsonlogic::create_logic(jv_expr);

  size_t matches = 0;
  auto jl1_lambda = [&] {
    matches = 0;
    for (size_t i = 0; i < N; ++i) {
      data_obj["x"] = xs[i];
      data_obj["y"] = ys[i];
      auto varaccess = jsonlogic::json_accessor(boost::json::value_from(data_obj));
      auto result = rule.apply(varaccess);

      bool val = jsonlogic::truthy(result);

      if (val) {
        ++matches;
      }
    }
  };

  auto jl1_bench = Benchmark("simple-and-jl1", jl1_lambda);

  // JL2 - Using create_logic approach

  auto jl2_lambda = [&] {
    matches = 0;
    jsonlogic::logic_rule rule = jsonlogic::create_logic(jv_expr);
    for (size_t i = 0; i < N; ++i) {
      auto result = rule.apply({xs[i], ys[i]});
      bool val = jsonlogic::truthy(result);

      if (val) {
        ++matches;
      }
    }
  };

  auto jl2_bench = Benchmark("simple-and-jl2", jl2_lambda);

  // Run benchmarks
  auto jl1_results = jl1_bench.run(N_RUNS);
  std::cout << "JL1 matches: " << matches << std::endl;

  auto jl2_results = jl2_bench.run(N_RUNS);
  std::cout << "JL2 matches: " << matches << std::endl;

  // Display results
  jl1_results.summarize();
  jl2_results.summarize();
  jl2_results.compare_to(jl1_results);

  return 0;
} catch (const std::exception &e) {
  std::cerr << "Fatal error: " << e.what() << '\n';
  return 1;
} catch (...) {
  std::cerr << "Fatal unknown error\n";
  return 2;
}
