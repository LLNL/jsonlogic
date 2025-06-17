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

  // x: double
  // y: int
  // z: string
  // (x / y > 5) or (x < 3.0 and y > 5 and z == "foo") or (y == 4 and x > 10.0
  // and "bar" in z)
  std::string expr;
  try {
    expr = read_file("complex1.json");
    std::cout << "Successfully read complex1.json from current directory"
              << std::endl;
  } catch (const std::exception &) {
    try {
      expr = read_file("bench/src/complex1.json");
      std::cout << "Successfully read complex1.json from bench/src/"
                << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "Error: Could not find complex1.json: " << e.what()
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
  std::vector<uint64_t> ys;
  ys.reserve(N);

  std::vector<std::string> zs;
  zs.reserve(N);

  std::vector<std::string> strset{"foo", "bar", "baz", "quux", "foobar"};

  // Create data
  for (size_t i = 0; i < N; ++i) {
    xs.push_back(faker::number::decimal<double>(0, 50));
    ys.push_back(faker::number::integer<uint64_t>(0, 255));
    auto ind = faker::number::integer<size_t>(0, strset.size() - 1);
    zs.push_back(strset[ind]);
  }

  // JL 1

  // Create jsonlogic benchmark
  auto jv_xy = boost::json::parse(expr);
  boost::json::object data_obj;

  size_t matches = 0;
  auto jl_lambda = [&] {
    matches = 0;
    for (size_t i = 0; i < N; ++i) {
      data_obj["x"] = xs[i];
      data_obj["y"] = ys[i];
      data_obj["z"] = zs[i];
      auto data = boost::json::value_from(data_obj);
      auto v_xy = jsonlogic::apply(jv_xy, data);

      bool val = jsonlogic::truthy(v_xy);

      if (val) {
        ++matches;
      }
    }
  };

  auto jl_bench = Benchmark("2ints-jl1", jl_lambda);

  // JL 2

  auto jl2_lambda = [&] {
    matches = 0;
    auto [rule, ignore1, igmore2] = jsonlogic::create_logic(jv_xy);
    for (size_t i = 0; i < N; ++i) {
      auto v_xy = jsonlogic::apply(rule, {xs[i], ys[i], zs[i]});
      bool val = jsonlogic::truthy(v_xy);

      if (val) {
        ++matches;
      }
    }
  };

  auto jl2_bench = Benchmark("2ints-jl2", jl2_lambda);

  auto jl_results = jl_bench.run(N_RUNS);
  std::cout << "- jl1 matches: " << matches << std::endl;
  auto jl2_results = jl2_bench.run(N_RUNS);
  std::cout << "jl2 matches: " << matches << std::endl;

  jl_results.summarize();
  jl2_results.summarize();
  //~ jl_results.compare_to(cpp_results);
  //~ cpp_results.compare_to(jl_results);

  jl2_results.compare_to(jl_results);
  return 0;
} catch (const std::exception &e) {
  std::cerr << "Fatal error: " << e.what() << '\n';
  return 1;
} catch (...) {
  std::cerr << "Fatal unkown error\n";
  return 2;
}
