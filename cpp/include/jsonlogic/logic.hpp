
#pragma once

#include <boost/json.hpp>

#include "details/ast-core.hpp"

namespace jsonlogic {

//
// exception classes

struct type_error : std::runtime_error {
  using base = std::runtime_error;
  using base::base;
};

//
// API to create an expression

/// the outpuf of translating a json object to an jsonlogic::expr
struct logic_details
    : std::tuple<any_expr, std::vector<boost::json::string>, bool> {
  using base = std::tuple<any_expr, std::vector<boost::json::string>, bool>;
  using base::base;

  /// the logic expression
  /// \{
  any_expr const &synatx_tree() const { return std::get<0>(*this); }
  // any_expr        expr() &&    { return std::get<0>(std::move(*this)); }
  /// \}

  /// returns variable names that are not computed
  /// \{
  std::vector<boost::json::string> const &variable_names() const {
    return std::get<1>(*this);
  }
  // std::vector<std::string>        variable_names() &&    { return
  // std::get<1>(std::move(*this)); }
  /// \}

  /// returns if the expression contains computed names
  bool has_computed_variable_names() const { return std::get<2>(*this); }
};

/// interprets the json object \ref n as a jsonlogic expression and
///   returns a jsonlogic representation together with some information
///   on variables inside the jsonlogic expression.
logic_details create_logic(boost::json::value n);

//
// API to evaluate/apply an expression

/// Type for a callback function to query variables from the context
/// \param  json::value a json value describing the variable
/// \param  int         an index for precomputed variable names
/// \return an any_expr
/// \post   the returned any_expr MUST be a non-null value
/// \todo
///   * consider replacing the dependence on the json library by
///     a string_view or std::variant<string_view, int ...> ..
///   * consider removing the limitation on any_expr being a value..
using variable_accessor =
    std::function<any_expr(const boost::json::value &, int)>;

/// evaluates \ref exp and uses \ref vars to query variables from
///   the context.
/// \param  exp  a jsonlogic expression
/// \param  vars a variable accessor to retrieve variables from the context
/// \return a jsonlogic value
/// \details
///    the version without variable accessors throws an std::runtime_error
///    when evaluation accesses a variable.
/// \{
any_expr apply(const any_expr &exp, const variable_accessor &vars);
any_expr apply(const any_expr &exp);
/// \}

/// evaluates the rule \ref rule with the provided data \ref data.
/// \param  rule a jsonlogic expression
/// \param  data a json object containing data that the jsonlogic expression
///         may access through variables.
/// \return a jsonlogic value
/// \details
///    converts rule to a jsonlogic expression and creates a variable_accessor
///    to query variables from data, before calling apply() on jsonlogic
///    expression.
any_expr apply(boost::json::value rule, boost::json::value data);

/// creates a variable accessor to access data in \ref data.
variable_accessor data_accessor(boost::json::value data);

//
// conversion functions from

/// creates a jsonlogic value representation for \ref val
/// \post any_expr != nullptr
/// \{
any_expr to_expr(std::nullptr_t val);
any_expr to_expr(bool val);
any_expr to_expr(std::int64_t val);
any_expr to_expr(std::uint64_t val);
any_expr to_expr(double val);
any_expr to_expr(boost::json::string val);
any_expr to_expr(const boost::json::array &val);
/// \}

/// creates a value representation for \ref n in jsonlogic form.
/// \param  n any boost::json type, except for boost::json::object
/// \return a value in jsonlogic form
/// \throw  an std::logic_error if \ref n contains a boost::json::object
/// \post   any_expr != nullptr
any_expr to_expr(const boost::json::value &n);

/// creates a json representation from \ref e
// \todo ...
boost::json::value to_json(const any_expr &e);

/// returns true if \ref el is truthy
/// \details
///    truthy performs the appropriate type conversions
///    but does not evaluate \ref el, in case it is a
///    jsonlogic expression.
bool truthy(const any_expr &el);

/// returns true if \ref el is !truthy
bool falsy(const any_expr &el);

/// prints out n to os
/// \pre n must be a value
std::ostream &operator<<(std::ostream &os, any_expr &n);

} // namespace jsonlogic
