#pragma once

#include <memory>
#include <boost/json.hpp>

#include "managed_string_view.hpp"

#if !defined(WITH_JSONLOGIC_EXTENSIONS)
// turn on to enable regex matching
#define WITH_JSONLOGIC_EXTENSIONS 1
#endif /* !defined(WITH_JSONLOGIC_EXTENSIONS) */

#if !defined(ENABLE_OPTIMIZATIONS)
// enables experimental optimizations
#define ENABLE_OPTIMIZATIONS 1
#endif /* !defined(ENABLE_OPTIMIZATIONS) */


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
// API to evaluate/apply an expression

struct array_value;

/// a type representing views on value types in jsonlogic
/// \details
///    (1) the variant contains options for all primitive types + strings and arrays.
///    (2) some rules treat the absence of a value differently from a null value
///    \todo document lifetime requirements
using value_variant_base = std::variant< std::monostate, // not available
                                         std::nullptr_t, // value is null
                                         bool,
                                         std::int64_t,
                                         std::uint64_t,
                                         double,
                                         managed_string_view,
                                         array_value const*
                                       >;

struct value_variant : value_variant_base {
  using base = value_variant_base;
  using base::base;

  value_variant()
  : base(std::monostate{})
  {}

  value_variant(const value_variant&);
  value_variant(value_variant&&);
  value_variant& operator=(const value_variant&);
  value_variant& operator=(value_variant&&);

  ~value_variant();
};

bool operator==(const value_variant& lhs, const value_variant& rhs);



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
    std::function<value_variant(value_variant, int)>;


#if DEPRECATED_FIX_STRINGS

// since value_variant contains string_views whose underlying strings may
//   be dynamically created during rule evaluation, the free standing API
//   can no longer be supported.

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
value_variant apply(const expr &exp, const variable_accessor &var_accessor);
value_variant apply(const any_expr &exp, const variable_accessor &var_accessor);
value_variant apply(const any_expr &exp);
value_variant apply(const any_expr &exp, std::vector<value_variant> vars);
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
value_variant apply(boost::json::value rule, boost::json::value data);

#endif /* DEPRECATED_FIX_STRINGS */

/// creates a variable accessor to access data in \ref data.
/// \{
variable_accessor json_accessor(boost::json::value data);
variable_accessor variant_accessor(std::vector<value_variant> data);
/// \}

//
// conversion functions


/// creates a json representation from \p e
//~ boost::json::value to_json(const any_expr &e);

/// returns true if \p el is truthy
/// \details
///    truthy performs the appropriate type conversions
///    but does not evaluate \p el, in case it is a
///    jsonlogic expression.
bool truthy(const value_variant &el);

/// returns true if \p el is !truthy
bool falsy(const value_variant &el);

/// prints out \p n to \p os
/// \pre n must be a value
std::ostream &operator<<(std::ostream &os, const value_variant &n);

/// result type of create_logic
//~ using logic_rule_base = std::tuple<any_expr, std::vector<std::string_view>, bool>;

struct logic_data;

/// convenience class providing named accessors to logic_rule_base
struct logic_rule {
    explicit logic_rule(std::unique_ptr<logic_data>&& rule_data);
    logic_rule(logic_rule&&);
    logic_rule& operator=(logic_rule&&);
    ~logic_rule();

    /// returns the logic expression
    //~ any_expr const &syntax_tree() const;

    /// returns static variable names (i.e., variable names that are not computed)
    std::vector<std::string_view> const &variable_names() const;

    /// returns if the expression contains computed names.
    bool has_computed_variable_names() const;

    /// evaluates the logic_rule.
    /// \return a jsonlogic value
    /// \throws variable_resolution_error when evaluation accesses a variable
    value_variant apply() ;

    /// evaluates the logic_rule and uses \p var_accessor to query variables.
    /// \param var_accessor a variable accessor to retrieve variables from the context
    /// \return a jsonlogic value
    value_variant apply(const variable_accessor &var_accessor) ;

    /// evaluates the logic_rule and uses \p vars to obtain values for non-computed variable names.
    /// \param  vars a variable array with values for non-computed variable names.
    /// \return a jsonlogic value
    /// \throws variable_resolution_error when evaluation accesses a computed variable name
    value_variant apply(std::vector<value_variant> vars) ;

    /// returns the data held internally for internal use.
    logic_data& internal_data();

  private:
    logic_rule()                             = delete;
    logic_rule(const logic_rule&)            = delete;
    logic_rule& operator=(const logic_rule&) = delete;

    std::unique_ptr<logic_data> data;
};

//
// API to create an expression


/// interprets the json object \ref n as a jsonlogic expression and
///   returns a jsonlogic representation together with some information
///   on variables inside the jsonlogic expression.
logic_rule create_logic(const boost::json::value& n);

} // namespace jsonlogic


namespace std
{
  template <>
  struct hash<jsonlogic::value_variant_base> {
    size_t operator()(const jsonlogic::value_variant_base& v) const;
  };

  template <>
  struct hash<jsonlogic::value_variant> {
    size_t operator()(const jsonlogic::value_variant& v) const;
  };

  template <>
  struct hash<jsonlogic::array_value> {
    size_t operator()(const jsonlogic::array_value& v) const;
  };
};
