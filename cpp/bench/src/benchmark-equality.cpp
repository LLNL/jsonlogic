#include <bench.hpp>
#include <boost/json.hpp>
#include <boost/json/src.hpp>
#include <chrono>
#include <cstdio>
#include <dlfcn.h>
#include <faker-cxx/location.h>
#include <faker-cxx/number.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <jsonlogic/logic.hpp>
#include <string>

namespace experimental {

struct dynamic_lib {
  explicit dynamic_lib(const char *dllname)
      : handle(dlopen(dllname, RTLD_LAZY)) {
    if (handle == nullptr)
      throw std::runtime_error{std::string{"Unable to load shared object: "} +
                               dlerror()};
  }

  ~dynamic_lib() {
    if (handle != nullptr)
      dlclose(handle);
  }

  dynamic_lib(dynamic_lib &&other) noexcept {
    std::swap(this->handle, other.handle);
  }

  dynamic_lib &operator=(dynamic_lib &&other) noexcept {
    std::swap(this->handle, other.handle);
    return *this;
  }

  template <class FnType> FnType function(const char *name) const {
    return (FnType)(dlsym(handle, name));
  }

  dynamic_lib() = delete;
  dynamic_lib(const dynamic_lib &) = delete;
  dynamic_lib &operator=(const dynamic_lib &) = delete;

private:
  void *handle = nullptr;
};

dynamic_lib compile_and_load(const std::string &code,
                             const std::string &libname) {
  // Step 1: Write code to a temporary file.
  // There is still a very minor risk of a race condition here, but
  // it would require guessing the name of the file and creating it
  // before the ofstream sourceFile statement a few lines down.
  const std::string tempSourceFile = tmpnam(nullptr) + std::string{".cpp"};

  std::ofstream sourceFile{tempSourceFile};
  if (!sourceFile)
    throw std::runtime_error{"Unable to create temporary source file."};

  sourceFile << code;
  sourceFile.close();

  // Step 2: Compile the source file into a shared object
  std::string inclDirs = RUNTIME_INCLUDES; // RUNTIME_INCLUDES is a macro

  std::string compileCommand =
      "g++ -Wall -Wextra -O3 -march=native -std=c++17 -shared -fPIC " +
      inclDirs + " -o " + libname + " " + tempSourceFile;

  std::cerr << compileCommand << std::endl;

  int compileResult = std::system(compileCommand.c_str());
  if (compileResult != 0)
    throw std::runtime_error{"Compilation failed."};

  std::remove(tempSourceFile.c_str());

  std::filesystem::path currentPath = std::filesystem::current_path();
  std::string fullPath = currentPath / libname;
  return dynamic_lib{fullPath.c_str()};
}

// using basic_type = std::string;
using basic_type = std::uint64_t;

inline std::uint64_t gendata(std::uint64_t) {
  return faker::number::integer<uint64_t>(0, 255);
}

inline std::string gendata(const std::string &) {
  return faker::location::city();
}

inline double gendata(double) { return faker::number::decimal(0.5); }

std::string variant_type_name(std::uint64_t) { return "std::uint64_t"; }

std::string variant_type_name(double) { return "double"; }

std::string variant_type_name(const std::string &) {
  return "std::string_view";
}

std::string gen_code(const std::string &fnname, const std::string &eltype) {
  std::stringstream os;
  os << "#include <jsonlogic/logic.hpp>\n"
     << "extern \"C\"\n"
     << "jsonlogic::value_variant " << fnname
     << "(std::vector<jsonlogic::value_variant> vars) {\n"
     << "  return std::get<" << eltype << ">(vars[0]) == std::get<" << eltype
     << ">(vars[1]);\n"
     << "}\n"
     << std::flush;

  return os.str();
}

} // namespace experimental

const unsigned long SEED_ = 42;
static const size_t N_ = 1'000'000;
static const int N_RUNS_ = 3;
int main(int argc, const char **argv) {

  try {
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

    // Create data
    for (size_t i = 0; i < N; ++i) {
      xs.push_back(faker::number::integer<uint64_t>(0, 255));
      ys.push_back(faker::number::integer<uint64_t>(0, 255));
    }

    // JL 1

    // Create jsonlogic benchmark
    std::string expr_str_xy = R"({"==":[{"var": "x"},{"var": "y"}]})";
    auto jv_xy = boost::json::parse(expr_str_xy);
    boost::json::object data_obj;

    size_t matches = 0;
    auto jl_lambda = [&] {
      matches = 0;
      for (size_t i = 0; i < N; ++i) {
        data_obj["x"] = xs[i];
        data_obj["y"] = ys[i];
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
        auto v_xy = jsonlogic::apply(rule, {xs[i], ys[i]});
        bool val = jsonlogic::truthy(v_xy);

        if (val) {
          ++matches;
        }
      }
    };

    auto jl2_bench = Benchmark("2ints-jl2", jl2_lambda);

    // C++ 1

    auto cpp_lambda = [&] {
      matches = 0;
      for (size_t i = 0; i < N; ++i) {
        if (xs[i] == ys[i]) {
          ++matches;
        }
      }
    };

    auto cpp_bench = Benchmark("2ints-cpp1", cpp_lambda);

    // C++ 2

    using evalfn_type =
        jsonlogic::value_variant (*)(std::vector<jsonlogic::value_variant>);

    std::chrono::steady_clock::time_point start_addl =
        std::chrono::steady_clock::now();
    std::string fake_mangled{"superfn"};
    std::string dllname{"superdll.so"};
    std::string benchmarktype =
        experimental::variant_type_name(experimental::basic_type{});
    std::string cxxcode = experimental::gen_code(fake_mangled, benchmarktype);
    experimental::dynamic_lib dll =
        experimental::compile_and_load(cxxcode, dllname);
    auto fn = dll.function<evalfn_type>(fake_mangled.c_str());
    std::chrono::steady_clock::time_point end_addl =
        std::chrono::steady_clock::now();

    ChronoUnit elapsed_addl = end_addl - start_addl;

    std::cout << "Adding " << elapsed_addl << " to cpp2\n";
    auto cpp2_lambda = [&] {
      matches = 0;
      for (size_t i = 0; i < N; ++i) {
        const bool eq = std::get<bool>(fn({xs[i], ys[i]}));

        if (eq) {
          ++matches;
        }
      }
    };

    auto cpp2_bench = Benchmark("2ints-cpp2", cpp2_lambda, elapsed_addl);

    auto jl_results = jl_bench.run(N_RUNS);
    std::cout << "- jl1 matches: " << matches << std::endl;
    auto jl2_results = jl2_bench.run(N_RUNS);
    std::cout << "jl2 matches: " << matches << std::endl;
    auto cpp_results = cpp_bench.run(N_RUNS);
    std::cout << "cpp1 matches: " << matches << std::endl;
    auto cpp2_results = cpp2_bench.run(N_RUNS);
    std::cout << "cpp2 matches: " << matches << std::endl;

    jl_results.summarize();
    jl2_results.summarize();
    cpp_results.summarize();
    cpp2_results.summarize();
    //~ jl_results.compare_to(cpp_results);
    //~ cpp_results.compare_to(jl_results);

    jl2_results.compare_to(jl_results);
    cpp2_results.compare_to(jl2_results);
    cpp2_results.compare_to(cpp_results);
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "Fatal unkown error\n";
    return 2;
  }
}