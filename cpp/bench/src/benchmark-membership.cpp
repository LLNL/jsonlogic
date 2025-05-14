#include <faker-cxx/faker.h>

#include <bench.hpp>
#include <boost/json.hpp>
#include <boost/json/src.hpp>
#include <iostream>
#include <jsonlogic/logic.hpp>
#include <set>

const int SEED_ = 42;
static const int HAYSTACK_RANGE_ = 1 << 18;
static const size_t HAYSTACK_SZ_ = 100000;
static const size_t N_ = 100000;
static const int N_RUNS_ = 10;
int main(int argc, const char **argv) {
  size_t N = N_;
  if (argc > 1) {
    N = std::stoul(argv[1]);
  }
  size_t N_RUNS = N_RUNS_;
  if (argc > 2) {
    N_RUNS = std::stoul(argv[2]);
  }

  int SEED = SEED_;
  if (argc > 3) {
    SEED = std::stoul(argv[3]);
  }

  int HAYSTACK_SZ = HAYSTACK_SZ_;
  if (argc > 4) {
    HAYSTACK_SZ = std::stoul(argv[4]);
  }

  int HAYSTACK_RANGE = HAYSTACK_RANGE_;
  if (argc > 5) {
    HAYSTACK_RANGE = std::stoul(argv[5]);
  }

  if (HAYSTACK_SZ > HAYSTACK_RANGE) {
    std::cerr << "HAYSTACK_SZ must be (significantly) less than "
              << HAYSTACK_RANGE << "\n";
    return 1;
  }

  faker::getGenerator().seed(SEED);
  std::set<uint64_t> haystack_set;
  boost::json::array haystack;
  std::vector<uint64_t> xs;
  xs.reserve(N);
  std::cout << "done initializing data\n";
  std::cout << "N: " << N << "\n";
  std::cout << "N_RUNS: " << N_RUNS << "\n";
  std::cout << "SEED: " << SEED << "\n";
  std::cout << "HAYSTACK_SZ: " << HAYSTACK_SZ << "\n";
  std::cout << "HAYSTACK_RANGE_: " << HAYSTACK_RANGE << "\n";

  uint64_t val;
  // create haystack
  while (haystack_set.size() <= HAYSTACK_SZ) {
    val = faker::number::integer(0, HAYSTACK_RANGE);
    if (!haystack_set.contains(val)) {
      haystack_set.insert(val);
      haystack.push_back(val);
    }
  }

  std::cout << "initialized haystack\n";
  // Create data
  for (size_t i = 0; i < N; ++i) {
    xs.push_back(faker::number::integer(0, HAYSTACK_RANGE));
  }

  std::cout << "initialized xs\n";
  // Create jsonlogic benchmark
  std::string expr_str_xy = R"({"in":[{"var": "x"},{"var": "haystack"}]})";
  auto jv_in = boost::json::parse(expr_str_xy);
  std::cout << "initialized jv_in\n";
  boost::json::object data_obj;

  data_obj["haystack"] = haystack;
  std::cout << "initialized data_obj\n";
  //   data_obj["haystack"] = haystack;
  size_t matches = 0;
  auto jl_lambda = [&] {
    matches = 0;
    for (size_t i = 0; i < N; ++i) {
      data_obj["x"] = xs[i];
      auto data = boost::json::value_from(data_obj);
      auto v_in = jsonlogic::apply(jv_in, data);

      bool val = jsonlogic::truthy(v_in);

      if (val) {
        ++matches;
      }
    }
  };

  auto jl_bench = Benchmark("needle in haystack jl", jl_lambda);

  auto cpp_lambda = [&] {
    matches = 0;
    for (size_t i = 0; i < N; ++i) {
      if (haystack_set.contains(xs[i])) {
        ++matches;
      }
    }
  };

  auto cpp_bench = Benchmark("needle in haystack cpp", cpp_lambda);

  auto jl_results = jl_bench.run(N_RUNS);
  std::cout << "jl matches: " << matches << std::endl;
  auto cpp_results = cpp_bench.run(N_RUNS);
  std::cout << "cpp matches: " << matches << std::endl;

  jl_results.summarize();
  cpp_results.summarize();
  jl_results.compare_to(cpp_results);
  cpp_results.compare_to(jl_results);
  return 0;
}