#include <boost/json.hpp>
#include <boost/json/src.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <fstream>
#include <iostream>
#include <ranges>

#include "jsonlogic/logic.hpp"

namespace bjsn = boost::json;

bjsn::value parseStream(std::istream &inps) {
  bjsn::stream_parser p;
  std::string line;

  // \todo skips ws in strings
  while (inps >> line) {
    std::error_code ec;

    p.write(line.c_str(), line.size(), ec);

    if (ec) return nullptr;
  }

  std::error_code ec;
  p.finish(ec);
  if (ec) return nullptr;

  return p.release();
}

bjsn::value parseFile(const std::string &filename) {
  std::ifstream is{filename};

  return parseStream(is);
}

template <class N, class T>
bool matchOpt1(const std::vector<std::string> &args, N &pos, std::string opt,
               T &fld) {
  std::string arg(args.at(pos));

  if (arg.find(opt) != 0) return false;

  ++pos;
  fld = boost::lexical_cast<T>(args.at(pos));
  ++pos;
  return true;
}

template <class N, class Fn>
bool matchOpt1(const std::vector<std::string> &args, N &pos, std::string opt,
               Fn fn) {
  std::string arg(args.at(pos));

  if (arg.find(opt) != 0) return false;

  ++pos;
  fn(args.at(pos));
  ++pos;
  return true;
}

template <class N, class Fn>
bool matchOpt0(const std::vector<std::string> &args, N &pos, std::string opt,
               Fn fn) {
  std::string arg(args.at(pos));

  if (arg.find(opt) != 0) return false;

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
  bool generate_expected = false;
  bool simple_apply = false;
  std::string filename = {};
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

  if (config.simple_apply) return jsonlogic::apply(rule, data);

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

  if (config.verbose) std::cerr << "falling back to normal apply" << std::endl;

  return logic.apply(jsonlogic::data_accessor(data));
}

int main(int argc, const char **argv) {
  constexpr bool MATCH = false;

  int errorCode = 0;
  settings config;
  std::vector<std::string> arguments(argv, argv + argc);
  size_t argn = 1;

  auto setVerbose = [&config]() -> void { config.verbose = true; };
  auto setResult = [&config]() -> void { config.generate_expected = true; };
  auto setSimple = [&config]() -> void { config.simple_apply = true; };
  auto setFile = [&config](const std::string &name) -> bool {
    const bool jsonFile = endsWith(name, ".json");

    if (jsonFile) config.filename = name;

    return jsonFile;
  };

  while (argn < arguments.size()) {
    // clang-format off
    MATCH
    || matchOpt0(arguments, argn, "-v", setVerbose)
    || matchOpt0(arguments, argn, "--verbose", setVerbose)
    || matchOpt0(arguments, argn, "-r", setResult)
    || matchOpt0(arguments, argn, "--result", setResult)
    || matchOpt0(arguments, argn, "-s", setSimple)
    || matchOpt0(arguments, argn, "--simple", setSimple)
    || noSwitch0(arguments, argn, setFile)
    ;
    // clang-format on
  }

  bjsn::value all = parseStream(std::cin);
  bjsn::object &allobj = all.as_object();

  bjsn::value rule = allobj["rule"];
  const bool hasData = allobj.contains("data");
  bjsn::value dat;
  const bool hasExpected = allobj.contains("expected");

  if (hasData)
    dat = allobj["data"];
  else
    dat.emplace_object();

  try {
    jsonlogic::any_expr res = call_apply(config, rule, dat);

    if (config.verbose) std::cerr << res << std::endl;

    if (config.generate_expected) {
      std::stringstream resStream;

      resStream << res;

      allobj["expected"] = parseStream(resStream);

      if (config.verbose) std::cerr << allobj["expected"] << std::endl;
    } else if (hasExpected) {
      std::stringstream expStream;
      std::stringstream resStream;

      expStream << allobj["expected"];
      resStream << res;
      errorCode = expStream.str() != resStream.str();

      if (config.verbose && errorCode)
        std::cerr << "test failed: "
                  << "\n  exp: " << expStream.str()
                  << "\n  got: " << resStream.str() << std::endl;
    } else {
      errorCode = 1;

      if (config.verbose)
        std::cerr << "unexpected completion, result: " << res << std::endl;
    }
  } catch (const std::exception &ex) {
    if (config.verbose) std::cerr << "caught error: " << ex.what() << std::endl;

    if (config.generate_expected)
      allobj.erase("expected");
    else if (hasExpected)
      errorCode = 1;
  } catch (...) {
    if (config.verbose) std::cerr << "caught unknown error" << std::endl;

    errorCode = 1;
  }

  if (config.generate_expected && (errorCode == 0))
    std::cout << allobj << std::endl;

  if (config.verbose && errorCode)
    std::cerr << "errorCode: " << errorCode << std::endl;

  return errorCode;
}
