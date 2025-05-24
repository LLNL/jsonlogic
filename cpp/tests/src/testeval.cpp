#include <boost/json.hpp>
#include <boost/json/src.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

#include <jsonlogic/logic.hpp>

enum class ResultStatus : std::uint8_t {
  NoError = 0,  // no error
  Error = 1,    // error in execution. Set resultError if known
  NoResult = 2, // For generated // unused right now?
};

/**
 * @brief Prints the expected and received stringstream values to std::cerr.
 *
 * This function is typically used for debugging or testing purposes to display
 * the expected output (`e`) and the actual output (`r`). If the `ns` flag is
 * set to true, it marks the output as nonstandard.
 *
 * @param e   The expected value as a stringstream.
 * @param r   The received (actual) value as a stringstream.
 * @param ns  Optional flag indicating if the output is nonstandard (default:
 * false).
 */
void p_exp_got(const std::stringstream &e, const std::stringstream &r,
               bool ns = false) {
  std::cerr << "\n  exp: " << e.str() << (ns ? " *nonstandard" : "")
            << "\n  got: " << r.str() << std::endl;
}

bool isError(ResultStatus r) { return static_cast<uint8_t>(r) > 1; }
namespace bjsn = boost::json;

bjsn::value parseStream(std::istream &inps) {
  bjsn::stream_parser p;
  std::string line;

  // \todo skips ws in strings
  while (inps >> line) {
    std::error_code ec;

    p.write(line.c_str(), line.size(), ec);

    if (ec)
      return nullptr;
  }

  std::error_code ec;
  p.finish(ec);
  if (ec)
    return nullptr;

  return p.release();
}

bjsn::value parseFile(const std::string &filename) {
  std::ifstream is{filename};

  return parseStream(is);
}

template <class N, class T>
bool matchOpt1(const std::vector<std::string> &args, N &pos,
               const std::string &opt, T &fld) {
  const std::string &arg = args.at(pos);

  if (!arg.starts_with(opt))
    return false;

  ++pos;
  fld = boost::lexical_cast<T>(args.at(pos));
  ++pos;
  return true;
}

template <class N, class Fn>
bool matchOpt1(const std::vector<std::string> &args, N &pos,
               const std::string &opt, Fn fn) {
  std::string arg(args.at(pos));

  if (!arg.starts_with(opt))
    return false;

  ++pos;
  fn(args.at(pos));
  ++pos;
  return true;
}

template <class N, class Fn>
bool matchOpt0(const std::vector<std::string> &args, N &pos,
               const std::string &opt, Fn fn) {
  const std::string &arg(args.at(pos));

  if (!arg.starts_with(opt))
    return false;

  fn();
  ++pos;
  return true;
}

template <class N, class Fn>
bool noSwitch0(const std::vector<std::string> &args, N &pos, Fn /*fn*/) {
  //~ if (fn(args[pos]))
  //~ return true;

  std::cerr << "unrecognized argument: " << args[pos] << std::endl;
  ++pos;
  return false;
}

bool endsWith(const std::string &str, const std::string &suffix) {
  return (str.size() >= suffix.size() &&
          std::equal(suffix.rbegin(), suffix.rend(), str.rbegin()));
}

struct settings {
  bool verbose = false;
  bool quiet = false;
  bool generate_expected = false;
  bool simple_apply = false;
  std::string filename;
};

/// converts val to a value_variant
/// \details
///    the object \p val's lifetime MUST exceeds the returned
///    value_variant's lifetime (in case val is a string).
/// \throws std::runtime_error if \p val cannot be converted.
jsonlogic::value_variant to_value_variant(const bjsn::value &n) {
  jsonlogic::value_variant res;

  switch (n.kind()) {
  case bjsn::kind::string: {
    const bjsn::string &str = n.get_string();
    res = std::string_view(str.data(), str.size());
    break;
  }

  case bjsn::kind::int64: {
    res = n.get_int64();
    break;
  }

  case bjsn::kind::uint64: {
    res = n.get_uint64();
    break;
  }

  case bjsn::kind::double_: {
    res = n.get_double();
    break;
  }

  case bjsn::kind::bool_: {
    res = n.get_bool();
    break;
  }

  case bjsn::kind::null: {
    res = std::monostate{};
    break;
  }

  default:
    throw std::runtime_error{"cannot convert"};
  }

  assert(!res.valueless_by_exception());
  return res;
}

jsonlogic::any_expr call_apply(settings &config, const bjsn::value &rule,
                               const bjsn::value &data) {
  using value_vector = std::vector<jsonlogic::value_variant>;

  if (config.simple_apply)
    return jsonlogic::apply(rule, data);

  jsonlogic::logic_rule logic = jsonlogic::create_logic(rule);

  if (!logic.has_computed_variable_names()) {
    if (config.verbose)
      std::cerr << "execute with precomputed value array." << std::endl;

    try {
      auto value_maker =
          [&data](std::string_view nm) -> jsonlogic::value_variant {
        return to_value_variant(data.as_object().at(nm));
      };

      // extract all variable values into vector
      auto const varvalues =
          logic.variable_names() | boost::adaptors::transformed(value_maker);

      // NOTE: data's lifetime must extend beyond the call to apply.
      return logic.apply(value_vector(varvalues.begin(), varvalues.end()));
    } catch (...) {
    }
  }

  if (config.verbose)
    std::cerr << "falling back to normal apply" << std::endl;

  return logic.apply(jsonlogic::data_accessor(data));
}

int main(int argc, const char **argv) {
  constexpr bool MATCH = false;

  ResultStatus resultStatus{0};

  bool result_matches_expected = false;
  settings config;
  std::vector<std::string> arguments(argv, argv + argc);
  size_t argn = 1;

  auto setVerbose = [&config]() -> void { config.verbose = true; };
  auto setQuiet = [&config]() -> void { config.quiet = true; };
  auto setResult = [&config]() -> void { config.generate_expected = true; };
  auto setSimple = [&config]() -> void { config.simple_apply = true; };
  auto setFile = [&config](const std::string &name) -> bool {
    const bool jsonFile = endsWith(name, ".json");

    if (jsonFile)
      config.filename = name;

    return jsonFile;
  };

  while (argn < arguments.size()) {
    // clang-format off
    MATCH
    || matchOpt0(arguments, argn, "-v", setVerbose)
    || matchOpt0(arguments, argn, "--verbose", setVerbose)
    || matchOpt0(arguments, argn, "-q", setQuiet)
    || matchOpt0(arguments, argn, "--quiet", setQuiet)
    || matchOpt0(arguments, argn, "-r", setResult)
    || matchOpt0(arguments, argn, "--result", setResult)
    || matchOpt0(arguments, argn, "-s", setSimple)
    || matchOpt0(arguments, argn, "--simple", setSimple)
    || noSwitch0(arguments, argn, setFile)
    ;
    // clang-format on
  }

  if (config.verbose && config.quiet) {
    std::cerr << "Cannot configure --verbose and --quiet together.\n";
    exit(1);
  }

  bjsn::value all = parseStream(std::cin);
  bjsn::object &allobj = all.as_object();

  bjsn::value rule = allobj["rule"];
  const bool hasData = allobj.contains("data");
  bjsn::value dat;
  const bool shouldFail =
      allobj.contains("shouldfail") && allobj["shouldfail"].as_bool();
  const bool isNonStandard =
      allobj.contains("nonstandard") && allobj["nonstandard"].as_bool();

  std::stringstream expStream;
  std::stringstream resStream;

  std::optional<std::string> resultError = std::nullopt;

  if (hasData)
    dat = allobj["data"];
  else
    dat.emplace_object();

  try {
    jsonlogic::any_expr res = call_apply(config, rule, dat);

    if (config.verbose)
      std::cerr << res << std::endl;

    if (config.generate_expected) {
      allobj["expected"] = parseStream(resStream);
    }

    if (config.verbose)
      std::cerr << allobj["expected"] << std::endl;

    expStream << allobj["expected"];
    resStream << res;

    result_matches_expected = expStream.str() == resStream.str();
    resultStatus = ResultStatus::NoError;

  } catch (const std::exception &ex) {
    resultStatus = ResultStatus::Error;
    resultError = ex.what();
    if (config.verbose)
      std::cerr << "caught error: " << ex.what() << std::endl;

    if (config.generate_expected)
      allobj.erase("expected");
  } catch (...) {
    resultStatus = ResultStatus::Error;
    if (config.verbose || !config.quiet)
      std::cerr << "caught unknown error" << std::endl;
  }

  if (config.generate_expected && result_matches_expected)
    std::cout << allobj << std::endl;

  if (resultStatus == ResultStatus::NoError) {
    if (shouldFail) {
      if (config.verbose || !config.quiet) {
        p_exp_got(expStream, resStream, isNonStandard);
      }
      return 1;
    }
    return static_cast<int>(!result_matches_expected);
  }

  // beyond here, we have Errors

  // we have an error, but we should fail.
  if (shouldFail) {
    return 0;
  }

  // we have errors, and we should not fail.

  if (config.verbose || !config.quiet) {
    std::cerr << "test failed: ";

    switch (resultStatus) {
    case ResultStatus::NoResult:
      std::cerr << "no result generated\n";
      break;

    default:
      std::cerr << "error: " << resultError.value_or("Unknown error")
                << std::endl;
      break;
    }
  }

  if (config.verbose)
    std::cerr << "result_matches_expected: " << result_matches_expected
              << std::endl;

  return static_cast<int>(resultStatus);
}
