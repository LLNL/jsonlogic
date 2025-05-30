
#pragma once

#include <boost/json.hpp>

#include "details/ast-core.hpp"

namespace jsonlogic {

//
// exception classes

/// thrown when no type conversion rules are able to satisfy an operation's type requirements
struct type_error : std::runtime_error {
  using base = std::runtime_error;
  using base::base;
};

/// thrown when a variable name cannot be found
struct variable_resolution_error : std::runtime_error {
  using base = std::runtime_error;
  using base::base;
};


//
// API to create an expression

/// result type of create_logic
// \todo remove dependence on boost::json::string and use std::string_view instead.
using logic_rule_base = std::tuple<any_expr, std::vector<boost::json::string>, bool>;

/// interprets the json object \ref n as a jsonlogic expression and
///   returns a jsonlogic representation together with some information
///   on variables inside the jsonlogic expression.
logic_rule_base create_logic(const boost::json::value& n);

//
// API to evaluate/apply an expression

/// a type representing views on value types in jsonlogic
/// \details
///    (1) the variant contains options for all primitive types.
///    (2) some rules treat the absence of a value differently from a null value
///    (3) boost::json::value is a fallback type that is used if one of
///        the following conditions is true:
///        - represent a value with compound type such as an array or object
///        - represent a string value for which it cannot be guaranteed that the
///          original string outlives the string_view
///        \todo consider using an any_expr as fallback type
using value_variant = std::variant< std::monostate,
                                    std::nullptr_t,
                                    bool,
                                    std::int64_t,
                                    std::uint64_t,
                                    double,
                                    std::string_view,
                                    boost::json::value // fallback type
                                  >;



/// Callback function type to query variables from the evaluation context
/// \param  json::value a json value describing the variable
/// \param  int         an index for precomputed variable names
/// \return an any_expr
/// \post   the returned any_expr MUST be a non-null value
/// \throw  variable_resolution_error if a variable is not available.
///         This exception will be caught by the evaluator
///         and the result replaced by an appropriate value (e.g., nullptr).
/// \throw  std::logic_error or std::runtime_error for other issues
///         (e.g., when an accessor does not support computed variable names).
///         Any exception other than variable_resolution_error will
///         result in termination.
/// \todo
///   * consider replacing the dependence on the json library by
///     a string_view or std::variant<string_view, int ...> ..
///   * consider removing the limitation on any_expr being a value..
using variable_accessor =
    std::function<any_expr(value_variant, int)>;

/// evaluates \ref exp and uses \ref vars to query variables from
///   the context.
/// \param  exp  a jsonlogic expression
/// \param  var_accessor a variable accessor to retrieve variables from the context
/// \param
/// \return a jsonlogic value
/// \details
///    * apply(const any_expr &exp) throws a variable_resolution_error
///      when evaluation accesses a variable.
///    * any_expr apply(const any_expr &exp, std::vector<value_variant> vars)
///      throws a variable_resolution_error when a computed variable name
///      is requested.
/// \{
any_expr apply(const expr &exp, const variable_accessor &var_accessor);
any_expr apply(const any_expr &exp, const variable_accessor &var_accessor);
any_expr apply(const any_expr &exp);
any_expr apply(const any_expr &exp, std::vector<value_variant> vars);
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
// conversion functions

/// creates a jsonlogic value representation for \ref val
/// \post any_expr != nullptr
/// \{
any_expr to_expr(std::nullptr_t val);
any_expr to_expr(bool val);
any_expr to_expr(std::int64_t val);
any_expr to_expr(std::uint64_t val);
any_expr to_expr(double val);
any_expr to_expr(std::string_view val);
any_expr to_expr(const boost::json::array &val);
// any_expr to_expr(boost::json::string val);
/// \}

/// creates a value representation for \p n in jsonlogic form.
/// \param  n any boost::json type, except for boost::json::object
/// \return a value in jsonlogic form
/// \throw  an std::logic_error if \p n contains a boost::json::object
/// \post   any_expr != nullptr
any_expr to_expr(const boost::json::value &n);

/// creates a value representation for \p val in jsonlogic form.
any_expr to_expr(value_variant val);

/// creates a json representation from \p e
boost::json::value to_json(const any_expr &e);

/// returns true if \p el is truthy
/// \details
///    truthy performs the appropriate type conversions
///    but does not evaluate \p el, in case it is a
///    jsonlogic expression.
bool truthy(const any_expr &el);

/// returns true if \p el is !truthy
bool falsy(const any_expr &el);

/// prints out \p n to \p os
/// \pre n must be a value
std::ostream &operator<<(std::ostream &os, any_expr &n);


/// convenience class providing named accessors to logic_rule_base
struct logic_rule : logic_rule_base {
    using base = logic_rule_base;

    // no explicit
    logic_rule(logic_rule_base&& logic_object)
    : base(std::move(logic_object))
    {}

    logic_rule(logic_rule&&)            = default;
    logic_rule& operator=(logic_rule&&) = default;
    ~logic_rule()                       = default;

    /// the logic expression
    /// \{
    any_expr const &syntax_tree() const;
    // any_expr        expr() &&    { return std::get<0>(std::move(*this)); }
    /// \}

    /// returns variable names that are not computed.
    std::vector<boost::json::string> const &variable_names() const;

    /// returns if the expression contains computed names.
    bool has_computed_variable_names() const;

    /// evaluates the logic_rule.
    /// \return a jsonlogic value
    /// \throws variable_resolution_error when evaluation accesses a variable
    any_expr apply() const;

    /// evaluates the logic_rule and uses \p var_accessor to query variables.
    /// \param var_accessor a variable accessor to retrieve variables from the context
    /// \return a jsonlogic value
    any_expr apply(const variable_accessor &var_accessor) const;

    /// evaluates the logic_rule and uses \p vars to obtain values for non-computed variable names.
    /// \param  vars a variable array with values for non-computed variable names.
    /// \return a jsonlogic value
    /// \throws variable_resolution_error when evaluation accesses a computed variable name
    any_expr apply(std::vector<value_variant> vars) const;

  private:
    logic_rule()                             = delete;
    logic_rule(const logic_rule&)            = delete;
    logic_rule& operator=(const logic_rule&) = delete;
};


} // namespace jsonlogic
