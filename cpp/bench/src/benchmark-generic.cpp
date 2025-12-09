#include <bench.hpp>
#include <boost/json.hpp>
#include <boost/json/src.hpp>
#include <cxxopts.hpp>
#include <faker-cxx/location.h>
#include <faker-cxx/number.h>
#include <faker-cxx/person.h>
#include <faker-cxx/string.h>
#include <fstream>
#include <iostream>
#include <jsonlogic/logic.hpp>
#include <random>
#include <string>
#include <variant>
#include <vector>

using namespace std;
namespace bjsn = boost::json;

std::string read_file(const std::string &filename) {
  std::ifstream file(filename);
  if (!file)
    throw std::runtime_error("Failed to open file: " + filename);
  return {std::istreambuf_iterator<char>(file),
          std::istreambuf_iterator<char>()};
}

// Supported types: 'i' = int, 'd' = double, 's' = string, 'b' = bool
// Use a char-based enum for type codes
enum class VarType : char { Int = 'i', Double = 'd', String = 's', Bool = 'b' };

using VarValue = std::variant<int, double, std::string, bool>;

VarValue fake_value(VarType type) {
  switch (type) {
  case VarType::Int:
    return faker::number::integer<int>(0, 10);
  case VarType::Double:
    return faker::number::decimal<double>(0, 10);
  case VarType::String:
    return faker::string::alphanumeric();
  case VarType::Bool:
    return faker::number::integer<int>(0, 1) != 0;
  default:
    throw std::runtime_error(std::string("Unknown VarType: '") +
                             static_cast<char>(type) + "')");
  }
}

int main(int argc, const char **argv) try {
  cxxopts::Options options("benchmark-generic", "Generic JSONLogic benchmark");
  options.add_options()("f,file", "Input JSON file",
                        cxxopts::value<std::string>())(
      "n,nrows", "Number of data rows",
      cxxopts::value<size_t>()->default_value("10000"))(
      "r,runs", "Number of runs", cxxopts::value<size_t>()->default_value("3"))(
      "s,seed", "Random seed",
      cxxopts::value<size_t>()->default_value("42"))("h,help", "Print usage");

  auto result = options.parse(argc, argv);
  if (result.count("help") || !result.count("file")) {
    std::cout << options.help() << std::endl;
    return 0;
  }
  std::string jsonfile = result["file"].as<std::string>();
  size_t N = result["nrows"].as<size_t>();
  size_t N_RUNS = result["runs"].as<size_t>();
  size_t SEED = result["seed"].as<size_t>();
  faker::getGenerator().seed(SEED);

  // Read and parse input JSON
  auto j = bjsn::parse(read_file(jsonfile));
  if (!j.is_object())
    throw std::runtime_error("Input JSON must be an object");
  const auto &obj = j.as_object();
  if (!obj.contains("rule") || !obj.contains("types"))
    throw std::runtime_error("Input JSON must contain 'rule' and 'types'");

  const auto &rule = obj.at("rule");
  const auto &types = obj.at("types").as_object();

  // Prepare variable names and types
  std::vector<std::string> var_names;
  std::vector<VarType> var_types;
  for (const auto &kv : types) {
    var_names.push_back(kv.key_c_str());
    // Extract the type code as a char from the JSON string value (must be a
    // single-character string)
    const auto &type_str = kv.value().as_string();
    if (type_str.empty()) {
      throw std::runtime_error(std::string("Type string for variable '") +
                               kv.key_c_str() + "' is empty");
    }
    var_types.push_back(static_cast<VarType>(type_str.front()));
  }

  // Generate fake data for each variable
  std::vector<std::vector<VarValue>> data(var_names.size());
  for (size_t i = 0; i < var_names.size(); ++i) {
    data[i].reserve(N);
    for (size_t j = 0; j < N; ++j) {
      data[i].push_back(fake_value(var_types[i]));
    }
  }
  
  size_t matches = 0;

#if UNSUPPORTED
  // JL1: Use boost::json::object for each row
  auto jl1_lambda = [&] {
    matches = 0;
    boost::json::object data_obj;
    for (size_t i = 0; i < N; ++i) {
      for (size_t v = 0; v < var_names.size(); ++v) {
        const auto &val = data[v][i];
        if (std::holds_alternative<int>(val))
          data_obj[var_names[v]] = std::get<int>(val);
        else if (std::holds_alternative<double>(val))
          data_obj[var_names[v]] = std::get<double>(val);
        else if (std::holds_alternative<std::string>(val))
          data_obj[var_names[v]] = std::get<std::string>(val);
        else if (std::holds_alternative<bool>(val))
          data_obj[var_names[v]] = std::get<bool>(val);
      }
      auto result = jsonlogic::apply(rule, bjsn::value_from(data_obj));
      bool val = jsonlogic::truthy(result);
      if (val)
        ++matches;
    }
  };
  auto jl1_bench = Benchmark("generic-jl1", jl1_lambda);
#endif /*UNSUPPORTED*/

  // JL2: Use create_logic and pass values as tuple
  auto jl2_lambda = [&] {
    matches = 0;
    auto jl2 = jsonlogic::create_logic(rule);
    for (size_t i = 0; i < N; ++i) {
      std::vector<jsonlogic::value_variant> args;
      for (size_t v = 0; v < var_names.size(); ++v) {
        const auto &val = data[v][i];
        if (std::holds_alternative<int>(val))
          args.push_back(std::get<int>(val));
        else if (std::holds_alternative<double>(val))
          args.push_back(std::get<double>(val));
        else if (std::holds_alternative<std::string>(val))
          args.push_back(jsonlogic::managed_string_view(std::get<std::string>(val)));
        else if (std::holds_alternative<bool>(val))
          args.push_back(std::get<bool>(val));
      }
      auto result = jl2.apply(args);
      bool val = jsonlogic::truthy(result);
      if (val)
        ++matches;
    }
  };
  auto jl2_bench = Benchmark("generic-jl2", jl2_lambda);

#if UNSUPPORTED
  // Run benchmarks
  auto jl1_results = jl1_bench.run(N_RUNS);
  std::cout << "JL1 matches: " << matches << std::endl;
#endif /*UNSUPPORTED*/
  
  auto jl2_results = jl2_bench.run(N_RUNS);
  std::cout << "JL2 matches: " << matches << std::endl;
#if UNSUPPORTED
  jl1_results.summarize();
#endif /*UNSUPPORTED*/
  jl2_results.summarize();
#if UNSUPPORTED  
  jl2_results.compare_to(jl1_results);
#endif /*UNSUPPORTED*/  
  return 0;
} catch (const std::exception &e) {
  std::cerr << "Fatal error: " << e.what() << '\n';
  return 1;
} catch (...) {
  std::cerr << "Fatal unknown error\n";
  return 2;
}
