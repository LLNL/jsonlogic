#include <algorithm>
#include <exception>
#include <iostream>
#include <limits>
#include <numeric>
#include <regex>
#include <string>
#include <unordered_map>

#include <boost/json.hpp>

#include "jsonlogic/details/ast-full.hpp"
#include "jsonlogic/details/cxx-compat.hpp"
#include "jsonlogic/logic.hpp"

namespace json = boost::json;

namespace {
constexpr bool DEBUG_OUTPUT = false;
}

namespace jsonlogic {
namespace {
CXX_NORETURN
void unsupported() {
  throw std::logic_error("functionality not yet implemented");
}

CXX_NORETURN
void throw_type_error() { throw type_error("typing error"); }

template <class Error = std::runtime_error, class T>
T &deref(T *p, const char *msg = "assertion failed") {
  if (p == nullptr) {
    CXX_UNLIKELY;
    throw Error{msg};
  }

  return *p;
}

template <class Error = std::runtime_error, class T>
T &deref(std::unique_ptr<T> &p, const char *msg = "assertion failed") {
  if (p.get() == nullptr) {
    CXX_UNLIKELY;
    throw Error{msg};
  }

  return *p;
}

template <class Error = std::runtime_error, class T>
const T &deref(const std::unique_ptr<T> &p,
               const char *msg = "assertion failed") {
  if (p.get() == nullptr) {
    CXX_UNLIKELY;
    throw Error{msg};
  }

  return *p;
}

template <class T> T &up_cast(T &n) { return n; }

template <class T> struct down_caster_internal {
  T *operator()(const expr &) const { return nullptr; }

  T *operator()(T &o) const { return &o; }
};

template <class T> T *may_down_cast(expr &e) {
  return generic_visit(down_caster_internal<T>{}, &e);
}

template <class T> T &down_cast(expr &e) {
  if (T *casted = may_down_cast<T>(e)) {
    CXX_LIKELY;
    return *casted;
  }

  throw_type_error();
}
} // namespace

//
// foundation classes
// \{

// accept implementations
void equal::accept(visitor &v) { v.visit(*this); }
void strict_equal::accept(visitor &v) { v.visit(*this); }
void not_equal::accept(visitor &v) { v.visit(*this); }
void strict_not_equal::accept(visitor &v) { v.visit(*this); }
void less::accept(visitor &v) { v.visit(*this); }
void greater::accept(visitor &v) { v.visit(*this); }
void less_or_equal::accept(visitor &v) { v.visit(*this); }
void greater_or_equal::accept(visitor &v) { v.visit(*this); }
void logical_and::accept(visitor &v) { v.visit(*this); }
void logical_or::accept(visitor &v) { v.visit(*this); }
void logical_not::accept(visitor &v) { v.visit(*this); }
void logical_not_not::accept(visitor &v) { v.visit(*this); }
void add::accept(visitor &v) { v.visit(*this); }
void subtract::accept(visitor &v) { v.visit(*this); }
void multiply::accept(visitor &v) { v.visit(*this); }
void divide::accept(visitor &v) { v.visit(*this); }
void modulo::accept(visitor &v) { v.visit(*this); }
void min::accept(visitor &v) { v.visit(*this); }
void max::accept(visitor &v) { v.visit(*this); }
void map::accept(visitor &v) { v.visit(*this); }
void reduce::accept(visitor &v) { v.visit(*this); }
void filter::accept(visitor &v) { v.visit(*this); }
void all::accept(visitor &v) { v.visit(*this); }
void none::accept(visitor &v) { v.visit(*this); }
void some::accept(visitor &v) { v.visit(*this); }
void array::accept(visitor &v) { v.visit(*this); }
void merge::accept(visitor &v) { v.visit(*this); }
void cat::accept(visitor &v) { v.visit(*this); }
void substr::accept(visitor &v) { v.visit(*this); }
void membership::accept(visitor &v) { v.visit(*this); }
void var::accept(visitor &v) { v.visit(*this); }
void missing::accept(visitor &v) { v.visit(*this); }
void missing_some::accept(visitor &v) { v.visit(*this); }
void log::accept(visitor &v) { v.visit(*this); }
void if_expr::accept(visitor &v) { v.visit(*this); }

void null_value::accept(visitor &v) { v.visit(*this); }
void bool_value::accept(visitor &v) { v.visit(*this); }
void int_value::accept(visitor &v) { v.visit(*this); }
void unsigned_int_value::accept(visitor &v) { v.visit(*this); }
void real_value::accept(visitor &v) { v.visit(*this); }
void string_value::accept(visitor &v) { v.visit(*this); }
void object_value::accept(visitor &v) { v.visit(*this); }

void error::accept(visitor &v) { v.visit(*this); }

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
void regex_match::accept(visitor &v) { v.visit(*this); }
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */

// to_json implementations
template <class T> json::value value_generic<T>::to_json() const {
  return value();
}

json::value null_value::to_json() const { return value(); }

// num_evaluated_operands implementations
int oper::num_evaluated_operands() const { return size(); }

template <int MaxArity> int oper_n<MaxArity>::num_evaluated_operands() const {
  return std::min(MaxArity, oper::num_evaluated_operands());
}

struct forwarding_visitor : visitor {
  void visit(expr &) override {} // error
  void visit(oper &n) override { visit(up_cast<expr>(n)); }
  void visit(equal &n) override { visit(up_cast<oper>(n)); }
  void visit(strict_equal &n) override { visit(up_cast<oper>(n)); }
  void visit(not_equal &n) override { visit(up_cast<oper>(n)); }
  void visit(strict_not_equal &n) override { visit(up_cast<oper>(n)); }
  void visit(less &n) override { visit(up_cast<oper>(n)); }
  void visit(greater &n) override { visit(up_cast<oper>(n)); }
  void visit(less_or_equal &n) override { visit(up_cast<oper>(n)); }
  void visit(greater_or_equal &n) override { visit(up_cast<oper>(n)); }
  void visit(logical_and &n) override { visit(up_cast<oper>(n)); }
  void visit(logical_or &n) override { visit(up_cast<oper>(n)); }
  void visit(logical_not &n) override { visit(up_cast<oper>(n)); }
  void visit(logical_not_not &n) override { visit(up_cast<oper>(n)); }
  void visit(add &n) override { visit(up_cast<oper>(n)); }
  void visit(subtract &n) override { visit(up_cast<oper>(n)); }
  void visit(multiply &n) override { visit(up_cast<oper>(n)); }
  void visit(divide &n) override { visit(up_cast<oper>(n)); }
  void visit(modulo &n) override { visit(up_cast<oper>(n)); }
  void visit(min &n) override { visit(up_cast<oper>(n)); }
  void visit(max &n) override { visit(up_cast<oper>(n)); }
  void visit(map &n) override { visit(up_cast<oper>(n)); }
  void visit(reduce &n) override { visit(up_cast<oper>(n)); }
  void visit(filter &n) override { visit(up_cast<oper>(n)); }
  void visit(all &n) override { visit(up_cast<oper>(n)); }
  void visit(none &n) override { visit(up_cast<oper>(n)); }
  void visit(some &n) override { visit(up_cast<oper>(n)); }
  void visit(merge &n) override { visit(up_cast<oper>(n)); }
  void visit(cat &n) override { visit(up_cast<oper>(n)); }
  void visit(substr &n) override { visit(up_cast<oper>(n)); }
  void visit(membership &n) override { visit(up_cast<oper>(n)); }
  void visit(var &n) override { visit(up_cast<oper>(n)); }
  void visit(missing &n) override { visit(up_cast<oper>(n)); }
  void visit(missing_some &n) override { visit(up_cast<oper>(n)); }
  void visit(log &n) override { visit(up_cast<oper>(n)); }

  void visit(if_expr &n) override { visit(up_cast<expr>(n)); }

  void visit(value_base &n) override { visit(up_cast<expr>(n)); }
  void visit(null_value &n) override { visit(up_cast<value_base>(n)); }
  void visit(bool_value &n) override { visit(up_cast<value_base>(n)); }
  void visit(int_value &n) override { visit(up_cast<value_base>(n)); }
  void visit(unsigned_int_value &n) override { visit(up_cast<value_base>(n)); }
  void visit(real_value &n) override { visit(up_cast<value_base>(n)); }
  void visit(string_value &n) override { visit(up_cast<value_base>(n)); }

  void visit(array &n) override { visit(up_cast<oper>(n)); }
  void visit(object_value &n) override { visit(up_cast<expr>(n)); }

  void visit(error &n) override { visit(up_cast<expr>(n)); }

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
  // extensions
  void visit(regex_match &n) override { visit(up_cast<expr>(n)); }
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */
};

namespace {

struct variable_map {
  void insert(var &el);
  std::vector<json::string> to_vector() const;
  bool hasComputedVariables() const { return hasComputed; }

private:
  using container_type = std::map<json::string, int>;

  container_type mapping = {};
  bool hasComputed = false;
};

void variable_map::insert(var &var) {
  try {
    any_expr &arg = var.back();
    string_value &str = down_cast<string_value>(*arg);
    const bool comp = (str.value().find('.') != json::string::npos &&
                       str.value().find('[') != json::string::npos);

    if (comp) {
      hasComputed = true;
    } else if (str.value() !=
               "") // do nothing for free variables membership "lambdas"
    {
      auto [pos, success] = mapping.emplace(str.value(), mapping.size());

      var.num(pos->second);
    }
  } catch (const type_error &) {
    hasComputed = true;
  }
}

std::vector<json::string> variable_map::to_vector() const {
  std::vector<json::string> res;

  res.resize(mapping.size());

  for (const container_type::value_type &el : mapping)
    res.at(el.second) = el.first;

  return res;
}

/// translates all children
/// \{
oper::container_type translate_children(json::array &children, variable_map &);

oper::container_type translate_children(json::value &n, variable_map &);
/// \}

template <class ExprT> ExprT &mkOperator_(json::object &n, variable_map &m) {
  assert(n.size() == 1);

  ExprT &res = deref(new ExprT);

  res.set_operands(translate_children(n.begin()->value(), m));
  return res;
}

template <class ExprT> expr &mk_operator(json::object &n, variable_map &m) {
  return mkOperator_<ExprT>(n, m);
}

expr &mk_variable(json::object &n, variable_map &m) {
  var &v = mkOperator_<var>(n, m);

  m.insert(v);
  return v;
}

array &mk_array(json::array &children, variable_map &m) {
  array &res = deref(new array);

  res.set_operands(translate_children(children, m));
  return res;
}

template <class value_t> value_t &mk_value(typename value_t::value_type n) {
  return deref(new value_t(std::move(n)));
}

null_value &mk_null_value() { return deref(new null_value); }

using dispatch_table =
    std::map<json::string, expr &(*)(json::object &, variable_map &)>;

dispatch_table::const_iterator lookup(const dispatch_table &m,
                                      const json::object &op) {
  if (op.size() != 1)
    return m.end();

  return m.find(op.begin()->key());
}

any_expr translate_internal(json::value n, variable_map &varmap) {
  static const dispatch_table dt = {
    {"==", &mk_operator<equal>},
    {"===", &mk_operator<strict_equal>},
    {"!=", &mk_operator<not_equal>},
    {"!==", &mk_operator<strict_not_equal>},
    {"if", &mk_operator<if_expr>},
    {"!", &mk_operator<logical_not>},
    {"!!", &mk_operator<logical_not_not>},
    {"or", &mk_operator<logical_or>},
    {"and", &mk_operator<logical_and>},
    {">", &mk_operator<greater>},
    {">=", &mk_operator<greater_or_equal>},
    {"<", &mk_operator<less>},
    {"<=", &mk_operator<less_or_equal>},
    {"max", &mk_operator<max>},
    {"min", &mk_operator<min>},
    {"+", &mk_operator<add>},
    {"-", &mk_operator<subtract>},
    {"*", &mk_operator<multiply>},
    {"/", &mk_operator<divide>},
    {"%", &mk_operator<modulo>},
    {"map", &mk_operator<map>},
    {"reduce", &mk_operator<reduce>},
    {"filter", &mk_operator<filter>},
    {"all", &mk_operator<all>},
    {"none", &mk_operator<none>},
    {"some", &mk_operator<some>},
    {"merge", &mk_operator<merge>},
    {"membership", &mk_operator<membership>},
    {"cat", &mk_operator<cat>},
    {"log", &mk_operator<log>},
    {"var", &mk_variable},
    {"missing", &mk_operator<missing>},
    {"missing_some", &mk_operator<missing_some>},
#if WITH_JSON_LOGIC_CPP_EXTENSIONS
    /// extensions
    {"regex", &mk_operator<regex_match>},
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */
  };

  expr *res = nullptr;

  switch (n.kind()) {
  case json::kind::object: {
    json::object &obj = n.get_object();
    dispatch_table::const_iterator pos = lookup(dt, obj);

    if (pos != dt.end()) {
      CXX_LIKELY;
      res = &pos->second(obj, varmap);
    } else {
      // does jsonlogic support value objects?
      unsupported();
    }

    break;
  }

  case json::kind::array: {
    // array is an operator that combines its subexpressions into an array
    res = &mk_array(n.get_array(), varmap);
    break;
  }

  case json::kind::string: {
    res = &mk_value<string_value>(std::move(n.get_string()));
    break;
  }

  case json::kind::int64: {
    res = &mk_value<int_value>(n.get_int64());
    break;
  }

  case json::kind::uint64: {
    res = &mk_value<unsigned_int_value>(n.get_uint64());
    break;
  }

  case json::kind::double_: {
    res = &mk_value<real_value>(n.get_double());
    break;
  }

  case json::kind::bool_: {
    res = &mk_value<bool_value>(n.get_bool());
    break;
  }

  case json::kind::null: {
    res = &mk_null_value();
    break;
  }

  default:
    unsupported();
  }

  return any_expr(res);
}

oper::container_type translate_children(json::array &children,
                                        variable_map &varmap) {
  oper::container_type res;

  res.reserve(children.size());

  for (json::value &elem : children)
    res.emplace_back(translate_internal(elem, varmap));

  return res;
}

oper::container_type translate_children(json::value &n, variable_map &varmap) {
  if (json::array *arr = n.if_array()) {
    CXX_LIKELY;
    return translate_children(*arr, varmap);
  }

  oper::container_type res;

  res.emplace_back(translate_internal(n, varmap));
  return res;
}
} // namespace

logic_details create_logic(json::value n) {
  variable_map varmap;
  any_expr node = translate_internal(std::move(n), varmap);
  bool hasComputedVariables = varmap.hasComputedVariables();

  return {std::move(node), varmap.to_vector(), hasComputedVariables};
}

//
// to value_base conversion

any_expr to_expr(std::nullptr_t) { return any_expr(new null_value); }
any_expr to_expr(bool val) { return any_expr(new bool_value(val)); }
any_expr to_expr(std::int64_t val) { return any_expr(new int_value(val)); }
any_expr to_expr(std::uint64_t val) {
  return any_expr(new unsigned_int_value(val));
}
any_expr to_expr(double val) { return any_expr(new real_value(val)); }
any_expr to_expr(json::string val) {
  return any_expr(new string_value(std::move(val)));
}

any_expr to_expr(const json::array &val) {
  oper::container_type elems;

  std::transform(val.begin(), val.end(), std::back_inserter(elems),
                 [](const json::value &el) -> any_expr { return to_expr(el); });

  array &arr = deref(new array);

  arr.set_operands(std::move(elems));
  return any_expr(&arr);
}

any_expr to_expr(const json::value &n) {
  any_expr res;

  switch (n.kind()) {
  case json::kind::string: {
    res = to_expr(n.get_string());
    break;
  }

  case json::kind::int64: {
    res = to_expr(n.get_int64());
    break;
  }

  case json::kind::uint64: {
    res = to_expr(n.get_uint64());
    break;
  }

  case json::kind::double_: {
    res = to_expr(n.get_double());
    break;
  }

  case json::kind::bool_: {
    res = to_expr(n.get_bool());
    break;
  }

  case json::kind::null: {
    res = to_expr(nullptr);
    break;
  }

  case json::kind::array: {
    res = to_expr(n.get_array());
    break;
  }

  default:
    unsupported();
  }

  assert(res.get());
  return res;
}

namespace {

//
// coercion functions

struct internal_coercion_error {};
struct not_int64_error : internal_coercion_error {};
struct not_uint64_error : internal_coercion_error {};
struct unpacked_array_req : internal_coercion_error {};

/// conversion to int64
/// \{
inline std::int64_t to_concrete(std::int64_t v, const std::int64_t &) {
  return v;
}
inline std::int64_t to_concrete(const json::string &str, const std::int64_t &) {
  return std::stoll(std::string{str.c_str()});
}
inline std::int64_t to_concrete(double v, const std::int64_t &) { return v; }
inline std::int64_t to_concrete(bool v, const std::int64_t &) { return v; }
inline std::int64_t to_concrete(std::nullptr_t, const std::int64_t &) {
  return 0;
}

inline std::int64_t to_concrete(std::uint64_t v, const std::int64_t &) {
  if (v > std::uint64_t(std::numeric_limits<std::int64_t>::max())) {
    CXX_UNLIKELY;
    throw not_int64_error{};
  }

  return v;
}
/// \}

/// conversion to uint64
/// \{
inline std::uint64_t to_concrete(std::uint64_t v, const std::uint64_t &) {
  return v;
}
inline std::uint64_t to_concrete(const json::string &str,
                                 const std::uint64_t &) {
  return std::stoull(std::string{str.c_str()});
}
inline std::uint64_t to_concrete(double v, const std::uint64_t &) { return v; }
inline std::uint64_t to_concrete(bool v, const std::uint64_t &) { return v; }
inline std::uint64_t to_concrete(std::nullptr_t, const std::uint64_t &) {
  return 0;
}

inline std::uint64_t to_concrete(std::int64_t v, const std::uint64_t &) {
  if (v < 0) {
    CXX_UNLIKELY;
    throw not_uint64_error{};
  }

  return v;
}
/// \}

/// conversion to double
/// \{
inline double to_concrete(const json::string &str, const double &) {
  return std::stod(std::string{str.c_str()});
}
inline double to_concrete(std::int64_t v, const double &) { return v; }
inline double to_concrete(std::uint64_t v, const double &) { return v; }
inline double to_concrete(double v, const double &) { return v; }
inline double to_concrete(bool v, const double &) { return v; }
inline double to_concrete(std::nullptr_t, const double &) { return 0; }
/// \}

/// conversion to string
/// \{
template <class Val>
inline json::string to_concrete(Val v, const json::string &) {
  return json::string{std::to_string(v)};
}

inline json::string to_concrete(bool v, const json::string &) {
  return json::string{v ? "true" : "false"};
}
inline json::string to_concrete(const json::string &s, const json::string &) {
  return s;
}
inline json::string to_concrete(std::nullptr_t, const json::string &) {
  return json::string{"null"};
}
/// \}

/// conversion to boolean
///   implements truthy, falsy as described by https://jsonlogic.com/truthy.html
/// \{
inline bool to_concrete(bool v, const bool &) { return v; }
inline bool to_concrete(std::int64_t v, const bool &) { return v; }
inline bool to_concrete(std::uint64_t v, const bool &) { return v; }
inline bool to_concrete(double v, const bool &) { return v; }
inline bool to_concrete(const json::string &v, const bool &) {
  return v.size() != 0;
}
inline bool to_concrete(std::nullptr_t, const bool &) { return false; }

// \todo logical_not sure if conversions from arrays to values should be
// supported like this
inline bool to_concrete(const array &v, const bool &) {
  return v.num_evaluated_operands();
}
/// \}

struct comparison_operator_base {
  enum {
    defined_for_string = true,
    defined_for_real = true,
    defined_for_integer = true,
    defined_for_boolean = true,
    defined_for_null = true,
    defined_for_array = true
  };

  using result_type = bool;
};

/// \brief a strict equality operator operates on operands of the same
///        type. The operation on two different types returns false.
///        NO type coercion is performed.
struct strict_equality_operator : comparison_operator_base {
  std::tuple<bool, bool> coerce(array *, array *) {
    return {true, false}; // arrays are never equal
  }

  template <class LhsT, class RhsT>
  std::tuple<LhsT, RhsT> coerce(LhsT *lv, RhsT *rv) {
    return {std::move(*lv), std::move(*rv)};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    std::nullptr_t) {
    return {nullptr, nullptr}; // two null pointers are equal
  }

  template <class LhsT>
  std::tuple<LhsT, std::nullptr_t> coerce(LhsT *lv, std::nullptr_t) {
    return {std::move(*lv), nullptr};
  }

  template <class RhsT>
  std::tuple<std::nullptr_t, RhsT> coerce(std::nullptr_t, RhsT *rv) {
    return {nullptr, std::move(*rv)};
  }
};

struct numeric_binary_operator_base {
  std::tuple<double, double> coerce(double *lv, double *rv) {
    return {*lv, *rv};
  }

  std::tuple<double, double> coerce(double *lv, std::int64_t *rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<double, double> coerce(double *lv, std::uint64_t *rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<double, double> coerce(std::int64_t *lv, double *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(std::int64_t *lv,
                                                std::int64_t *rv) {
    return {*lv, *rv};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(std::int64_t *lv,
                                                std::uint64_t *rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<double, double> coerce(std::uint64_t *lv, double *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(std::uint64_t *lv,
                                                std::int64_t *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(std::uint64_t *lv,
                                                  std::uint64_t *rv) {
    return {*lv, *rv};
  }
};

/// \brief an equality operator compares two values. if_expr the
///        values have a different type, type coercion is performed
///        on one of the operands.
struct relational_operator_base : numeric_binary_operator_base {
  using numeric_binary_operator_base::coerce;

  std::tuple<double, double> coerce(double *lv, json::string *rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<double, double> coerce(double *lv, bool *rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(std::int64_t *lv,
                                                json::string *rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(std::int64_t *lv, bool *rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(std::uint64_t *lv,
                                                  json::string *rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(std::uint64_t *lv, bool *rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<double, double> coerce(json::string *lv, double *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<double, double> coerce(bool *lv, double *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(json::string *lv,
                                                std::int64_t *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(bool *lv, std::int64_t *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(json::string *lv,
                                                  std::uint64_t *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(bool *lv, std::uint64_t *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<bool, bool> coerce(json::string *, bool *rv) {
    // strings and boolean are never equal
    return {!*rv, *rv};
  }

  std::tuple<bool, bool> coerce(bool *lv, json::string *) {
    // strings and boolean are never equal
    return {*lv, !*lv};
  }

  std::tuple<json::string, json::string> coerce(json::string *lv,
                                                json::string *rv) {
    return {std::move(*lv), std::move(*rv)};
  }

  std::tuple<bool, bool> coerce(bool *lv, bool *rv) { return {*lv, *rv}; }
};

struct equality_operator : relational_operator_base, comparison_operator_base {
  using relational_operator_base::coerce;

  // due to special conversion rules, the coercion function may just produce
  //   the result instead of just unpacking and coercing values.

  std::tuple<bool, bool> coerce(array *, array *) {
    return {true, false}; // arrays are never equal
  }

  template <class T> std::tuple<bool, bool> coerce(T *lv, array *rv) {
    // an array may be compared to a value_base
    //   (1) *lv == arr[0], iff the array has exactly one element
    if (rv->num_evaluated_operands() == 1)
      throw unpacked_array_req{};

    //   (2) or if [] and *lv converts to false
    if (rv->num_evaluated_operands() > 1)
      return {false, true};

    const bool convToFalse = to_concrete(*lv, false) == false;

    return {convToFalse, true /* zero elements */};
  }

  template <class T> std::tuple<bool, bool> coerce(array *lv, T *rv) {
    // see comments membership coerce(T*,array*)
    if (lv->num_evaluated_operands() == 1)
      throw unpacked_array_req{};

    if (lv->num_evaluated_operands() > 1)
      return {false, true};

    const bool convToFalse = to_concrete(*rv, false) == false;

    return {true /* zero elements */, convToFalse};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    std::nullptr_t) {
    return {nullptr, nullptr}; // two null pointers are equal
  }

  template <class T> std::tuple<bool, bool> coerce(T *, std::nullptr_t) {
    return {false, true}; // null pointer is only equal to itself
  }

  template <class T> std::tuple<bool, bool> coerce(std::nullptr_t, T *) {
    return {true, false}; // null pointer is only equal to itself
  }

  std::tuple<bool, bool> coerce(array *, std::nullptr_t) {
    return {true, false}; // null pointer is only equal to itself
  }

  std::tuple<bool, bool> coerce(std::nullptr_t, array *) {
    return {true, false}; // null pointer is only equal to itself
  }
};

struct relational_operator : relational_operator_base,
                             comparison_operator_base {
  using relational_operator_base::coerce;

  std::tuple<array *, array *> coerce(array *lv, array *rv) { return {lv, rv}; }

  template <class T> std::tuple<bool, bool> coerce(T *lv, array *rv) {
    // an array may be equal to another value_base if
    //   (1) *lv == arr[0], iff the array has exactly one element
    if (rv->num_evaluated_operands() == 1)
      throw unpacked_array_req{};

    //   (2) or if [] and *lv converts to false
    if (rv->num_evaluated_operands() > 1)
      return {false, true};

    const bool convToTrue = to_concrete(*lv, true) == true;

    return {convToTrue, false /* zero elements */};
  }

  template <class T> std::tuple<bool, bool> coerce(array *lv, T *rv) {
    // see comments membership coerce(T*,array*)
    if (lv->num_evaluated_operands() == 1)
      throw unpacked_array_req{};

    if (lv->num_evaluated_operands() > 1)
      return {false, true};

    const bool convToTrue = to_concrete(*rv, true) == true;

    return {false /* zero elements */, convToTrue};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    std::nullptr_t) {
    return {nullptr, nullptr}; // two null pointers are equal
  }

  std::tuple<bool, bool> coerce(bool *lv, std::nullptr_t) {
    return {*lv, false}; // null pointer -> false
  }

  std::tuple<std::int64_t, std::int64_t> coerce(std::int64_t *lv,
                                                std::nullptr_t) {
    return {*lv, 0}; // null pointer -> 0
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(std::uint64_t *lv,
                                                  std::nullptr_t) {
    return {*lv, 0}; // null pointer -> 0
  }

  std::tuple<double, double> coerce(double *lv, std::nullptr_t) {
    return {*lv, 0}; // null pointer -> 0.0
  }

  std::tuple<json::string, std::nullptr_t> coerce(json::string *lv,
                                                  std::nullptr_t) {
    return {std::move(*lv), nullptr}; // requires special handling
  }

  std::tuple<bool, bool> coerce(std::nullptr_t, bool *rv) {
    return {false, *rv}; // null pointer -> false
  }

  std::tuple<std::int64_t, std::int64_t> coerce(std::nullptr_t,
                                                std::int64_t *rv) {
    return {0, *rv}; // null pointer -> 0
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(std::nullptr_t,
                                                  std::uint64_t *rv) {
    return {0, *rv}; // null pointer -> 0
  }

  std::tuple<double, double> coerce(std::nullptr_t, double *rv) {
    return {0, *rv}; // null pointer -> 0
  }

  std::tuple<std::nullptr_t, json::string> coerce(std::nullptr_t,
                                                  json::string *rv) {
    return {nullptr, std::move(*rv)}; // requires special handling
  }
};
// @}

// Arith
struct arithmetic_operator : numeric_binary_operator_base {
  enum {
    defined_for_string = false,
    defined_for_real = true,
    defined_for_integer = true,
    defined_for_boolean = false,
    defined_for_null = true,
    defined_for_array = false
  };

  using result_type = any_expr;

  using numeric_binary_operator_base::coerce;

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(double *, std::nullptr_t) {
    return {nullptr, nullptr};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::int64_t *,
                                                    std::nullptr_t) {
    return {nullptr, nullptr};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::uint64_t *,
                                                    std::nullptr_t) {
    return {nullptr, nullptr};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t, double *) {
    return {nullptr, nullptr};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    std::int64_t *) {
    return {nullptr, nullptr};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    std::uint64_t *) {
    return {nullptr, nullptr};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    std::nullptr_t) {
    return {nullptr, nullptr};
  }
};

struct integer_arithmetic_operator : arithmetic_operator {
  enum {
    defined_for_string = false,
    defined_for_real = false,
    defined_for_integer = true,
    defined_for_boolean = false,
    defined_for_null = false,
    defined_for_array = false
  };

  using arithmetic_operator::coerce;
};

struct string_operator {
  enum {
    defined_for_string = true,
    defined_for_real = false,
    defined_for_integer = false,
    defined_for_boolean = false,
    defined_for_null = false,
    defined_for_array = false
  };

  using result_type = any_expr;

  std::tuple<json::string, json::string> coerce(json::string *lv,
                                                json::string *rv) {
    return {std::move(*lv), std::move(*rv)};
  }
};

struct array_operator {
  enum {
    defined_for_string = false,
    defined_for_real = false,
    defined_for_integer = false,
    defined_for_boolean = false,
    defined_for_null = false,
    defined_for_array = true
  };

  using result_type = any_expr;

  std::tuple<array *, array *> coerce(array *lv, array *rv) { return {lv, rv}; }
};

/*
  any_expr convert(any_expr val, ...)
  {
    return val;
  }
*/

any_expr convert(any_expr val, const arithmetic_operator &) {
  struct arithmetic_converter : forwarding_visitor {
    explicit arithmetic_converter(any_expr val) : res(std::move(val)) {}

    void visit(expr &) final { throw_type_error(); }

    // defined for the following types
    void visit(int_value &) final {}
    void visit(unsigned_int_value &) final {}
    void visit(real_value &) final {}
    void visit(null_value &) final {}

    // need to convert values
    void visit(string_value &el) final {
      double dd = to_concrete(el.value(), double{});
      std::int64_t ii = to_concrete(el.value(), std::int64_t{});
      // uint?

      res = (dd != ii) ? to_expr(dd) : to_expr(ii);
    }

    void visit(bool_value &) final {
      // \todo correct?
      res = to_expr(nullptr);
    }

    any_expr result() && { return std::move(res); }

  private:
    any_expr res;
  };

  expr *node = val.get();
  arithmetic_converter conv{std::move(val)};

  node->accept(conv);
  return std::move(conv).result();
}

/*
  any_expr convert(any_expr val, const integer_arithmetic_operator&)
  {
    struct integer_arithmetic_converter : forwarding_visitor
    {
        explicit
        integer_arithmetic_converter(any_expr val)
        : res(std::move(val))
        {}

        void visit(expr&)               final { throw_type_error(); }

        // defined for the following types
        void visit(int_value&)          final {}
        void visit(unsigned_int_value&) final {}

        // need to convert values
        void visit(string_value& el) final
        {
          res = to_expr(to_concrete(el.value(), std::int64_t{}));
        }

        void visit(bool_value& el) final
        {
          res = to_expr(to_concrete(el.value(), std::int64_t{}));
        }

        void visit(real_value& el) final
        {
          res = to_expr(to_concrete(el.value(), std::int64_t{}));
        }

        void visit(null_value&) final
        {
          res = to_expr(std::int64_t{0});
        }

        any_expr result() && { return std::move(res); }

      private:
        any_expr res;
    };

    expr*                        node = val.get();
    integer_arithmetic_converter conv{std::move(val)};

    node->accept(conv);
    return std::move(conv).result();
  }
*/

any_expr convert(any_expr val, const string_operator &) {
  struct string_converter : forwarding_visitor {
    explicit string_converter(any_expr val) : res(std::move(val)) {}

    void visit(expr &) final { throw_type_error(); }

    // defined for the following types
    void visit(string_value &) final {}

    // need to convert values
    void visit(bool_value &el) final {
      res = to_expr(to_concrete(el.value(), json::string{}));
    }

    void visit(int_value &el) final {
      res = to_expr(to_concrete(el.value(), json::string{}));
    }

    void visit(unsigned_int_value &el) final {
      res = to_expr(to_concrete(el.value(), json::string{}));
    }

    void visit(real_value &el) final {
      res = to_expr(to_concrete(el.value(), json::string{}));
    }

    void visit(null_value &el) final {
      res = to_expr(to_concrete(el.value(), json::string{}));
    }

    any_expr result() && { return std::move(res); }

  private:
    any_expr res;
  };

  expr *node = val.get();
  string_converter conv{std::move(val)};

  node->accept(conv);
  return std::move(conv).result();
}

any_expr convert(any_expr val, const array_operator &) {
  struct array_converter : forwarding_visitor {
    explicit array_converter(any_expr val) : val(std::move(val)) {}

    void visit(expr &) final { throw_type_error(); }

    // moves res into
    void to_array() {
      array &arr = deref(new array);
      oper::container_type operands;

      operands.emplace_back(&arr);

      // swap the operand and result
      std::swap(operands.back(), val);

      // then set the operands
      arr.set_operands(std::move(operands));
    }

    // defined for the following types
    void visit(array &) final {}

    // need to move value_base to new array
    void visit(string_value &) final { to_array(); }
    void visit(bool_value &) final { to_array(); }
    void visit(int_value &) final { to_array(); }
    void visit(unsigned_int_value &) final { to_array(); }
    void visit(real_value &) final { to_array(); }
    void visit(null_value &) final { to_array(); }

    any_expr result() && { return std::move(val); }

  private:
    any_expr val;
  };

  expr *node = val.get();
  array_converter conv{std::move(val)};

  node->accept(conv);
  return std::move(conv).result();
}

template <class value_t> struct unpacker : forwarding_visitor {
  void assign(value_t &lhs, const value_t &val) { lhs = val; }

  template <class U> void assign(value_t &lhs, const U &val) {
    lhs = to_concrete(val, lhs);
  }

  CXX_NORETURN
  void visit(expr &) final { throw_type_error(); }

  // defined for the following types
  void visit(string_value &el) final { assign(res, el.value()); }

  // need to convert values
  void visit(bool_value &el) final { assign(res, el.value()); }

  void visit(int_value &el) final { assign(res, el.value()); }

  void visit(unsigned_int_value &el) final { assign(res, el.value()); }

  void visit(real_value &el) final { assign(res, el.value()); }

  void visit(null_value &el) final { assign(res, el.value()); }

  void visit(array &el) final {
    if constexpr (std::is_same<value_t, bool>::value) {
      CXX_LIKELY;
      return assign(res, el);
    }

    throw_type_error();
  }

  value_t result() && { return std::move(res); }

private:
  value_t res;
};

template <class T> T unpack_value(expr &expr) {
  unpacker<T> unpack;

  expr.accept(unpack);
  return std::move(unpack).result();
}

template <class T> T unpack_value(const any_expr &el) {
  return unpack_value<T>(*el);
}

template <class T> T unpack_value(any_expr &&el) {
  return unpack_value<T>(*el);
}

//
// Json Logic - truthy/falsy
bool truthy(expr &el) { return unpack_value<bool>(el); }
bool truthy(any_expr &&el) { return unpack_value<bool>(std::move(el)); }
bool falsy(expr &el) { return !truthy(el); }
// bool falsy(any_expr&& el)  { return !truthy(std::move(el)); }
} // namespace

bool truthy(const any_expr &el) { return unpack_value<bool>(el); }
bool falsy(const any_expr &el) { return !truthy(el); }

//
// cloning

namespace {
any_expr clone_expr(const any_expr &expr);

struct expr_cloner {
  /// init functions that set up children
  /// \{
  expr &init(const oper &, oper &) const;
  expr &init(const object_value &, object_value &) const;
  /// \}

  /// function family for type specific cloning
  /// \param  n       the original node
  /// \param  unnamed a tag parameter to summarily handle groups of types
  /// \return the cloned node
  /// \{
  CXX_NORETURN
  expr &clone(const expr &, const expr &) const { unsupported(); }

  expr &clone(const error &, const error &) const { return deref(new error); }

  template <class value_t>
  expr &clone(const value_t &n, const value_base &) const {
    return deref(new value_t(n.value()));
  }

  template <class oper_t> expr &clone(const oper_t &n, const oper &) const {
    return init(n, deref(new oper_t));
  }

  expr &clone(const object_value &n, const object_value &) const {
    return init(n, deref(new object_value));
  }
  /// \}

  template <class expr_t> expr *operator()(expr_t &n) { return &clone(n, n); }
};

expr &expr_cloner::init(const oper &src, oper &tgt) const {
  oper::container_type children;

  std::transform(src.operands().begin(), src.operands().end(),
                 std::back_inserter(children),
                 [](const any_expr &e) -> any_expr { return clone_expr(e); });

  tgt.set_operands(std::move(children));
  return tgt;
}

expr &expr_cloner::init(const object_value &src, object_value &tgt) const {
  std::transform(
      src.begin(), src.end(), std::inserter(tgt.elements(), tgt.end()),
      [](const object_value::value_type &entry) -> object_value::value_type {
        return {entry.first, clone_expr(entry.second)};
      });

  return tgt;
}

any_expr clone_expr(const any_expr &exp) {
  return any_expr(generic_visit(expr_cloner{}, exp.get()));
}

template <class expr_t, class fn_t, class alt_fn_t>
auto with_type(expr *e, fn_t fn, alt_fn_t altfn) -> decltype(altfn()) {
  if (e == nullptr) {
    CXX_UNLIKELY;
    return altfn();
  }

  expr_t *casted = may_down_cast<expr_t>(*e);

  return casted ? fn(*casted) : altfn();
}

//
// binary operator - double dispatch pattern

template <class binary_op_t, class lhs_value_t>
struct binary_operator_visitor_2 : forwarding_visitor {
  using result_type = typename binary_op_t::result_type;

  binary_operator_visitor_2(lhs_value_t lval, binary_op_t oper)
      : lv(lval), op(oper), res() {}

  template <class rhs_value_t> void calc(rhs_value_t rv) {
    auto [ll, rr] = op.coerce(lv, rv);

    res = op(std::move(ll), std::move(rr));
  }

  void visit(expr &) final { throw_type_error(); }

  void visit(string_value &n) final {
    if constexpr (binary_op_t::defined_for_string)
      return calc(&n.value());

    throw_type_error();
  }

  void visit(null_value &) final {
    if constexpr (binary_op_t::defined_for_null)
      return calc(nullptr);

    throw_type_error();
  }

  void visit(bool_value &n) final {
    if constexpr (binary_op_t::defined_for_boolean)
      return calc(&n.value());

    throw_type_error();
  }

  void visit(int_value &n) final {
    if constexpr (binary_op_t::defined_for_integer) {
      try {
        return calc(&n.value());
      } catch (const not_int64_error &ex) {
        if (n.value() < 0) {
          CXX_UNLIKELY;
          throw std::range_error{
              "unable to consolidate uint>max(int) with int<0"};
        }
      }

      std::uint64_t alt = n.value();
      return calc(&alt);
    }

    throw_type_error();
  }

  void visit(unsigned_int_value &n) final {
    if constexpr (binary_op_t::defined_for_integer) {
      try {
        return calc(&n.value());
      } catch (const not_uint64_error &ex) {
        if (n.value() > std::uint64_t(std::numeric_limits<std::int64_t>::max)) {
          CXX_UNLIKELY;
          throw std::range_error{
              "unable to consolidate int<0 with uint>max(int)"};
        }
      }

      std::int64_t alt = n.value();
      return calc(&alt);
    }

    throw_type_error();
  }

  void visit(real_value &n) final {
    if constexpr (binary_op_t::defined_for_real)
      return calc(&n.value());

    throw_type_error();
  }

  void visit(array &n) final {
    if constexpr (binary_op_t::defined_for_array) {
      try {
        calc(&n);
      } catch (const unpacked_array_req &) {
        assert(n.num_evaluated_operands() == 1);
        n.operand(0).accept(*this);
      }

      return;
    }

    throw_type_error();
  }

  result_type result() && { return std::move(res); }

private:
  lhs_value_t lv;
  binary_op_t op;
  result_type res;
};

template <class binary_op_t>
struct binary_operator_visitor : forwarding_visitor {
  using result_type = typename binary_op_t::result_type;

  binary_operator_visitor(binary_op_t oper, any_expr &rhsarg)
      : op(oper), rhs(rhsarg), res() {}

  template <class LhsValue> void calc(LhsValue lv) {
    using rhs_visitor = binary_operator_visitor_2<binary_op_t, LhsValue>;

    rhs_visitor vis{lv, op};

    rhs->accept(vis);
    res = std::move(vis).result();
  }

  void visit(string_value &n) final {
    if constexpr (binary_op_t::defined_for_string)
      return calc(&n.value());

    throw_type_error();
  }

  void visit(null_value &) final {
    if constexpr (binary_op_t::defined_for_null)
      return calc(nullptr);

    throw_type_error();
  }

  void visit(bool_value &n) final {
    if constexpr (binary_op_t::defined_for_boolean)
      return calc(&n.value());

    throw_type_error();
  }

  void visit(int_value &n) final {
    if constexpr (binary_op_t::defined_for_integer) {
      try {
        return calc(&n.value());
      } catch (const not_int64_error &ex) {
        if (n.value() < 0) {
          CXX_UNLIKELY;
          throw std::range_error{
              "unable to consolidate int<0 with uint>max(int)"};
        }
      }

      std::uint64_t alt = n.value();
      return calc(&alt);
    }

    throw_type_error();
  }

  void visit(unsigned_int_value &n) final {
    if constexpr (binary_op_t::defined_for_integer) {
      try {
        return calc(&n.value());
      } catch (const not_uint64_error &ex) {
        if (n.value() > std::uint64_t(std::numeric_limits<std::int64_t>::max)) {
          CXX_UNLIKELY;
          throw std::range_error{
              "unable to consolidate uint>max(int) with int<0"};
        }
      }

      std::int64_t alt = n.value();
      return calc(&alt);
    }

    throw_type_error();
  }

  void visit(real_value &n) final {
    if constexpr (binary_op_t::defined_for_real)
      return calc(&n.value());

    throw_type_error();
  }

  void visit(array &n) final {
    if constexpr (binary_op_t::defined_for_array) {
      try {
        calc(&n);
      } catch (const unpacked_array_req &) {
        assert(n.num_evaluated_operands() == 1);
        n.operand(0).accept(*this);
      }

      return;
    }

    throw_type_error();
  }

  result_type result() && { return std::move(res); }

private:
  binary_op_t op;
  any_expr &rhs;
  result_type res;
};

//
// compute and sequence functions

template <class binary_op_t>
typename binary_op_t::result_type compute(any_expr &lhs, any_expr &rhs,
                                          binary_op_t op) {
  using lhs_visitor = binary_operator_visitor<binary_op_t>;

  assert(lhs.get() && rhs.get());

  lhs_visitor vis{op, rhs};

  lhs->accept(vis);
  return std::move(vis).result();
}

template <class binary_predicate_t>
bool compare_sequence(array &lv, array &rv, binary_predicate_t pred) {
  const std::size_t lsz = lv.num_evaluated_operands();
  const std::size_t rsz = rv.num_evaluated_operands();

  if (lsz == 0)
    return pred(false, rsz != 0);

  if (rsz == 0)
    return pred(true, false);

  std::size_t const len = std::min(lsz, rsz);
  std::size_t i = 0;
  bool res = false;
  bool found = false;

  while ((i < len) && !found) {
    res = compute(lv.at(i), rv.at(i), pred);

    // res is conclusive if the reverse test yields a different result
    found = res != compute(rv.at(i), lv.at(i), pred);

    ++i;
  }

  return found ? res : pred(lsz, rsz);
}

template <class binary_predicate_t>
bool compare_sequence(array *lv, array *rv, binary_predicate_t pred) {
  return compare_sequence(deref(lv), deref(rv), std::move(pred));
}

/// convenience template that only returns \p R
/// when the types \p U and \p V mismatch.
/// matching \p U and \p V trigger a SFINAE error to invoke
/// the regular operator.
/// \{
template <class U, class V, class R>
struct mismatched_types
{
  using type = R;
};

template <class T, class R>
struct mismatched_types<T,T,R>
{
  // using type // triggers SFINAE exclusion
};
/// \}

//
// the calc operator implementations

template <class> struct operator_impl {};

template <> struct operator_impl<equal> : equality_operator {
  using equality_operator::result_type;

  template <class U, class V>
  auto operator()(const U&, const V&) const -> typename mismatched_types<U, V, result_type>::type {
    return false;
  }

  template <class T> auto operator()(const T &lhs, const T &rhs) const -> result_type {
    return lhs == rhs;
  }
};

template <> struct operator_impl<not_equal> : equality_operator {
  using equality_operator::result_type;

  template <class U, class V>
  auto operator()(const U&, const V&) const -> typename mismatched_types<U, V, result_type>::type {
    return true;
  }

  template <class T> auto operator()(const T &lhs, const T &rhs) const -> result_type {
    return lhs != rhs;
  }
};

template <> struct operator_impl<strict_equal> : strict_equality_operator {
  using strict_equality_operator::result_type;

  template <class U, class V>
  auto operator()(const U&, const V&) const -> typename mismatched_types<U, V, result_type>::type {
    return false;
  }

  template <class T> auto operator()(const T &lhs, const T &rhs) const -> result_type {
    return lhs == rhs;
  }
};

template <> struct operator_impl<strict_not_equal> : strict_equality_operator {
  using strict_equality_operator::result_type;

  template <class U, class V>
  auto operator()(const U&, const V&) const -> typename mismatched_types<U, V, result_type>::type {
    return true;
  }

  template <class T> auto operator()(const T &lhs, const T &rhs) const -> result_type {
    return lhs != rhs;
  }
};

template <> struct operator_impl<less> : relational_operator {
  using relational_operator::result_type;

  result_type operator()(const std::nullptr_t, std::nullptr_t) const {
    return false;
  }

  result_type operator()(array *lv, array *rv) const {
    return compare_sequence(lv, rv, *this);
  }

  result_type operator()(const json::string &, std::nullptr_t) const {
    return false;
  }

  result_type operator()(std::nullptr_t, const json::string &) const {
    return false;
  }

  template <class T> result_type operator()(const T &lhs, const T &rhs) const {
    return lhs < rhs;
  }
};

template <> struct operator_impl<greater> : relational_operator {
  using relational_operator::result_type;

  result_type operator()(const std::nullptr_t, std::nullptr_t) const {
    return false;
  }

  result_type operator()(array *lv, array *rv) const {
    return compare_sequence(lv, rv, *this);
  }

  result_type operator()(const json::string &, std::nullptr_t) const {
    return false;
  }

  result_type operator()(std::nullptr_t, const json::string &) const {
    return false;
  }

  template <class T> result_type operator()(const T &lhs, const T &rhs) const {
    return rhs < lhs;
  }
};

template <> struct operator_impl<less_or_equal> : relational_operator {
  using relational_operator::result_type;

  result_type operator()(const std::nullptr_t, std::nullptr_t) const {
    return true;
  }

  result_type operator()(array *lv, array *rv) const {
    return compare_sequence(lv, rv, *this);
  }

  result_type operator()(const json::string &lhs, std::nullptr_t) const {
    return lhs.empty();
  }

  result_type operator()(std::nullptr_t, const json::string &rhs) const {
    return rhs.empty();
  }

  template <class T> result_type operator()(const T &lhs, const T &rhs) const {
    return lhs <= rhs;
  }
};

template <> struct operator_impl<greater_or_equal> : relational_operator {
  using relational_operator::result_type;

  result_type operator()(const std::nullptr_t, std::nullptr_t) const {
    return true;
  }

  result_type operator()(array *lv, array *rv) const {
    return compare_sequence(lv, rv, *this);
  }

  result_type operator()(const json::string &lhs, std::nullptr_t) const {
    return lhs.empty();
  }

  result_type operator()(std::nullptr_t, const json::string &rhs) const {
    return rhs.empty();
  }

  template <class T> result_type operator()(const T &lhs, const T &rhs) const {
    return rhs <= lhs;
  }
};

template <> struct operator_impl<add> : arithmetic_operator {
  using arithmetic_operator::result_type;

  result_type operator()(std::nullptr_t, std::nullptr_t) const {
    return to_expr(nullptr);
  }

  template <class T> result_type operator()(const T &lhs, const T &rhs) const {
    return to_expr(lhs + rhs);
  }
};

template <> struct operator_impl<subtract> : arithmetic_operator {
  using arithmetic_operator::result_type;

  result_type operator()(std::nullptr_t, std::nullptr_t) const {
    return to_expr(nullptr);
  }

  template <class T> result_type operator()(const T &lhs, const T &rhs) const {
    return to_expr(lhs - rhs);
  }
};

template <> struct operator_impl<multiply> : arithmetic_operator {
  using arithmetic_operator::result_type;

  result_type operator()(std::nullptr_t, std::nullptr_t) const {
    return to_expr(nullptr);
  }

  template <class T> result_type operator()(const T &lhs, const T &rhs) const {
    return to_expr(lhs * rhs);
  }
};

template <> struct operator_impl<divide> : arithmetic_operator {
  using arithmetic_operator::result_type;

  result_type operator()(std::nullptr_t, std::nullptr_t) const {
    return to_expr(nullptr);
  }

  result_type operator()(double lhs, double rhs) const {
    double res = lhs / rhs;

    // if (isInteger(res)) return toInt(res);
    return to_expr(res);
  }

  template <class Int_t> result_type operator()(Int_t lhs, Int_t rhs) const {
    if (lhs % rhs)
      return (*this)(double(lhs), double(rhs));

    return to_expr(lhs / rhs);
  }
};

template <> struct operator_impl<modulo> : integer_arithmetic_operator {
  using integer_arithmetic_operator::result_type;

  std::nullptr_t operator()(std::nullptr_t, std::nullptr_t) const {
    return nullptr;
  }

  template <class T> result_type operator()(const T &lhs, const T &rhs) const {
    if (rhs == 0)
      return to_expr(nullptr);

    return to_expr(lhs % rhs);
  }
};

template <> struct operator_impl<min> : arithmetic_operator {
  using arithmetic_operator::result_type;

  result_type operator()(const std::nullptr_t, std::nullptr_t) const {
    return nullptr;
  }

  template <class T> result_type operator()(const T &lhs, const T &rhs) const {
    return to_expr(std::min(lhs, rhs));
  }
};

template <> struct operator_impl<max> : arithmetic_operator {
  using arithmetic_operator::result_type;

  result_type operator()(const std::nullptr_t, std::nullptr_t) const {
    return nullptr;
  }

  template <class T> result_type operator()(const T &lhs, const T &rhs) const {
    return to_expr(std::max(lhs, rhs));
  }
};

template <> struct operator_impl<logical_not> {
  using result_type = bool;

  result_type operator()(expr &val) const { return falsy(val); }
};

template <> struct operator_impl<logical_not_not> {
  using result_type = bool;

  result_type operator()(expr &val) const { return truthy(val); }
};

template <> struct operator_impl<cat> : string_operator {
  using string_operator::result_type;

  result_type operator()(const json::string &lhs,
                         const json::string &rhs) const {
    json::string tmp;

    tmp.reserve(lhs.size() + rhs.size());

    tmp.append(lhs.begin(), lhs.end());
    tmp.append(rhs.begin(), rhs.end());

    return to_expr(std::move(tmp));
  }
};

template <>
struct operator_impl<membership>
    : string_operator // \todo the conversion rules differ
{
  using string_operator::result_type;

  result_type operator()(const json::string &lhs,
                         const json::string &rhs) const {
    const bool res = (rhs.find(lhs) != json::string::npos);

    return to_expr(res);
  }
};

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
template <>
struct operator_impl<regex_match>
    : string_operator // \todo the conversion rules differ
{
  using string_operator::result_type;

  result_type operator()(const json::string &lhs,
                         const json::string &rhs) const {
    std::regex rgx(lhs.c_str(), lhs.size());

    return to_expr(std::regex_search(rhs.begin(), rhs.end(), rgx));
  }
};
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */

template <> struct operator_impl<merge> : array_operator {
  using array_operator::result_type;

  result_type operator()(array *lhs, array *rhs) const {
    // note, to use the lhs entirely, it would need to be released
    //   from its any_expr
    array &res = deref(new array);

    {
      oper::container_type &opers = res.operands();

      opers.swap(lhs->operands());

      oper::container_type &ropers = rhs->operands();

      opers.insert(opers.end(), std::make_move_iterator(ropers.begin()),
                   std::make_move_iterator(ropers.end()));
    }

    return any_expr(&res);
  }
};

struct evaluator : forwarding_visitor {
  evaluator(variable_accessor varAccess, std::ostream &out)
      : vars(std::move(varAccess)), logger(out), calcres(nullptr) {}

  void visit(equal &) final;
  void visit(strict_equal &) final;
  void visit(not_equal &) final;
  void visit(strict_not_equal &) final;
  void visit(less &) final;
  void visit(greater &) final;
  void visit(less_or_equal &) final;
  void visit(greater_or_equal &) final;
  void visit(logical_and &) final;
  void visit(logical_or &) final;
  void visit(logical_not &) final;
  void visit(logical_not_not &) final;
  void visit(add &) final;
  void visit(subtract &) final;
  void visit(multiply &) final;
  void visit(divide &) final;
  void visit(modulo &) final;
  void visit(min &) final;
  void visit(max &) final;
  void visit(array &) final;
  void visit(map &) final;
  void visit(reduce &) final;
  void visit(filter &) final;
  void visit(all &) final;
  void visit(none &) final;
  void visit(some &) final;
  void visit(merge &) final;
  void visit(cat &) final;
  void visit(substr &) final;
  void visit(membership &) final;
  void visit(var &) final;
  void visit(missing &) final;
  void visit(missing_some &) final;
  void visit(log &) final;

  void visit(if_expr &) final;

  void visit(null_value &n) final;
  void visit(bool_value &n) final;
  void visit(int_value &n) final;
  void visit(unsigned_int_value &n) final;
  void visit(real_value &n) final;
  void visit(string_value &n) final;

  void visit(error &n) final;

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
  void visit(regex_match &n) final;
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */

  any_expr eval(expr &n);

private:
  variable_accessor vars;
  std::ostream &logger;
  any_expr calcres;

  evaluator(const evaluator &) = delete;
  evaluator(evaluator &&) = delete;
  evaluator &operator=(const evaluator &) = delete;
  evaluator &operator=(evaluator &&) = delete;

  //
  // opers

  /// implements relop : [1, 2, 3, whatever] as 1 relop 2 relop 3
  template <class binary_predicate_t>
  void eval_pair_short_circuit(oper &n, binary_predicate_t pred);

  /// returns the first expression membership [ e1, e2, e3 ] that evaluates to
  /// val,
  ///   or the last expression otherwise
  void eval_short_circuit(oper &n, bool val);

  /// reduction operation on all elements
  template <class binary_op_t> void reduce_sequence(oper &n, binary_op_t op);

  /// computes unary operation on n[0]
  template <class UnaryOperator> void unary(oper &n, UnaryOperator calc);

  /// binary operation on all elements (invents an element if none is present)
  template <class binary_op_t> void binary(oper &n, binary_op_t binop);

  /// evaluates and unpacks n[argpos] to a fundamental value_base
  template <class value_t>
  value_t unpack_optional_arg(oper &n, int argpos, const value_t &defaultVal);

  /// auxiliary missing method
  std::size_t missing_aux(array &elems);

  template <class ValueNode> void _value(const ValueNode &val) {
    calcres = to_expr(val.value());
  }
};

struct sequence_function {
  sequence_function(expr &e, std::ostream &logstream)
      : exp(e), logger(logstream) {}

  any_expr operator()(any_expr &&elem) const {
    any_expr *elptr = &elem; // workaround, b/c unique_ptr cannot be captured

    evaluator sub{[elptr](const json::value &keyval, int) -> any_expr {
                    if (const json::string *pkey = keyval.if_string()) {
                      const json::string &key = *pkey;

                      if (key.size() == 0)
                        return clone_expr(*elptr);

                      try {
                        object_value &o = down_cast<object_value>(**elptr);

                        if (auto pos = o.find(key); pos != o.end())
                          return clone_expr(pos->second);
                      } catch (const type_error &) {
                      }
                    }

                    return to_expr(nullptr);
                  },
                  logger};

    return sub.eval(exp);
  }

private:
  expr &exp;
  std::ostream &logger;
};

struct sequence_predicate : sequence_function {
  using sequence_function::sequence_function;

  bool operator()(any_expr &&elem) const {
    return truthy(sequence_function::operator()(std::move(elem)));
  }
};

struct sequence_predicate_nondestructive : sequence_function {
  using sequence_function::sequence_function;

  bool operator()(any_expr &&elem) const {
    return truthy(sequence_function::operator()(clone_expr(elem)));
  }
};

/*
  template <class InputIterator, class BinaryOperation>
  any_expr
  accumulate_move(InputIterator pos, InputIterator lim, any_expr accu,
  BinaryOperation binop)
  {
    for ( ; pos != lim; ++pos)
      accu = binop(std::move(accu), std::move(*first));

    return accu;
  }
*/

struct sequence_reduction {
  sequence_reduction(expr &e, std::ostream &logstream)
      : exp(e), logger(logstream) {}

  // for compatibility reasons, the first argument is passed membership as
  // templated && ref.
  //   g++ -std=c++17 passes membership a & ref, while g++ -std=c++20 passes
  //   membership a &&.
  // the templated && together with reference collapsing make the code portable
  // across standard versions.
  // \note const ValueExprT& would also work, but currently the
  //       visitor calls membership clone_expr require a non-const reference.
  template <class ValueExprT>
  any_expr operator()(ValueExprT &&accu, any_expr elem) const {
    any_expr *acptr = &accu; // workaround, b/c unique_ptr cannot be captured
    any_expr *elptr = &elem; // workaround, b/c unique_ptr cannot be captured

    evaluator sub{[acptr, elptr](const json::value &keyval, int) -> any_expr {
                    if (const json::string *pkey = keyval.if_string()) {
                      if (*pkey == "current")
                        return clone_expr(*elptr);

                      if (*pkey == "accumulator")
                        return clone_expr(*acptr);
                    }

                    return to_expr(nullptr);
                  },
                  logger};

    return sub.eval(exp);
  }

private:
  expr &exp;
  std::ostream &logger;
};

template <class value_t>
value_t evaluator::unpack_optional_arg(oper &n, int argpos,
                                       const value_t &defaultVal) {
  if (std::size_t(argpos) >= n.size()) {
    CXX_UNLIKELY;
    return defaultVal;
  }

  return unpack_value<value_t>(*eval(n.operand(argpos)));
}

template <class unary_predicate_t>
void evaluator::unary(oper &n, unary_predicate_t pred) {
  CXX_MAYBE_UNUSED
  const int num = n.num_evaluated_operands();
  assert(num == 1);

  const bool res = pred(*eval(n.operand(0)));

  calcres = to_expr(res);
}

template <class binary_op_t>
void evaluator::binary(oper &n, binary_op_t binop) {
  const int num = n.num_evaluated_operands();
  assert(num == 1 || num == 2);

  int idx = -1;
  any_expr lhs;

  if (num == 2) {
    CXX_LIKELY;
    lhs = eval(n.operand(++idx));
  } else {
    lhs = to_expr(std::int64_t(0));
  }

  any_expr rhs = eval(n.operand(++idx));

  calcres = compute(lhs, rhs, binop);
}

template <class binary_op_t>
void evaluator::reduce_sequence(oper &n, binary_op_t op) {
  const int num = n.num_evaluated_operands();
  assert(num >= 1);

  int idx = -1;
  any_expr res = eval(n.operand(++idx));

  res = convert(std::move(res), op);

  while (idx != (num - 1)) {
    any_expr rhs = eval(n.operand(++idx));

    rhs = convert(std::move(rhs), op);
    res = compute(res, rhs, op);
  }

  calcres = std::move(res);
}

template <class binary_predicate_t>
void evaluator::eval_pair_short_circuit(oper &n, binary_predicate_t pred) {
  const int num = n.num_evaluated_operands();
  assert(num >= 2);

  bool res = true;
  int idx = -1;
  any_expr rhs = eval(n.operand(++idx));
  assert(rhs.get());

  while (res && (idx != (num - 1))) {
    any_expr lhs = std::move(rhs);
    assert(lhs.get());

    rhs = eval(n.operand(++idx));
    assert(rhs.get());

    res = compute(lhs, rhs, pred);
  }

  calcres = to_expr(res);
}

void evaluator::eval_short_circuit(oper &n, bool val) {
  const int num = n.num_evaluated_operands();

  if (num == 0) {
    CXX_UNLIKELY;
    throw_type_error();
  }

  int idx = -1;
  any_expr oper = eval(n.operand(++idx));
  //~ std::cerr << idx << ") " << oper << std::endl;

  bool found = (idx == num - 1) || (truthy(*oper) == val);

  // loop until *aa == val or when *aa is the last valid element
  while (!found) {
    oper = eval(n.operand(++idx));
    //~ std::cerr << idx << ") " << oper << std::endl;

    found = (idx == (num - 1)) || (truthy(*oper) == val);
  }

  calcres = std::move(oper);
}

any_expr evaluator::eval(expr &n) {
  any_expr res;

  n.accept(*this);
  res.swap(calcres);

  return res;
}

void evaluator::visit(equal &n) {
  eval_pair_short_circuit(n, operator_impl<equal>{});
}

void evaluator::visit(strict_equal &n) {
  eval_pair_short_circuit(n, operator_impl<strict_equal>{});
}

void evaluator::visit(not_equal &n) {
  eval_pair_short_circuit(n, operator_impl<not_equal>{});
}

void evaluator::visit(strict_not_equal &n) {
  eval_pair_short_circuit(n, operator_impl<strict_not_equal>{});
}

void evaluator::visit(less &n) {
  eval_pair_short_circuit(n, operator_impl<less>{});
}

void evaluator::visit(greater &n) {
  eval_pair_short_circuit(n, operator_impl<greater>{});
}

void evaluator::visit(less_or_equal &n) {
  eval_pair_short_circuit(n, operator_impl<less_or_equal>{});
}

void evaluator::visit(greater_or_equal &n) {
  eval_pair_short_circuit(n, operator_impl<greater_or_equal>{});
}

void evaluator::visit(logical_and &n) { eval_short_circuit(n, false); }

void evaluator::visit(logical_or &n) { eval_short_circuit(n, true); }

void evaluator::visit(logical_not &n) {
  unary(n, operator_impl<logical_not>{});
}

void evaluator::visit(logical_not_not &n) {
  unary(n, operator_impl<logical_not_not>{});
}

void evaluator::visit(add &n) { reduce_sequence(n, operator_impl<add>{}); }

void evaluator::visit(subtract &n) { binary(n, operator_impl<subtract>{}); }

void evaluator::visit(multiply &n) {
  reduce_sequence(n, operator_impl<multiply>{});
}

void evaluator::visit(divide &n) { binary(n, operator_impl<divide>{}); }

void evaluator::visit(modulo &n) { binary(n, operator_impl<modulo>{}); }

void evaluator::visit(min &n) { reduce_sequence(n, operator_impl<min>{}); }

void evaluator::visit(max &n) { reduce_sequence(n, operator_impl<max>{}); }

void evaluator::visit(cat &n) { reduce_sequence(n, operator_impl<cat>{}); }

void evaluator::visit(membership &n) { binary(n, operator_impl<membership>{}); }

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
void evaluator::visit(regex_match &n) {
  binary(n, operator_impl<membership>{});
}
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */

void evaluator::visit(substr &n) {
  assert(n.num_evaluated_operands() >= 1);

  json::string str = unpack_value<json::string>(*eval(n.operand(0)));
  std::int64_t ofs = unpack_optional_arg<std::int64_t>(n, 1, 0);
  std::int64_t cnt = unpack_optional_arg<std::int64_t>(n, 2, 0);

  if (ofs < 0) {
    CXX_UNLIKELY;
    ofs = std::max(std::int64_t(str.size()) + ofs, std::int64_t(0));
  }

  if (cnt < 0) {
    CXX_UNLIKELY;
    cnt = std::max(std::int64_t(str.size()) - ofs + cnt, std::int64_t(0));
  }

  calcres = to_expr(json::string{str.subview(ofs, cnt)});
}

void evaluator::visit(array &n) {
  oper::container_type elems;
  evaluator *self = this;

  // \todo consider making arrays lazy
  std::transform(
      std::make_move_iterator(n.begin()), std::make_move_iterator(n.end()),
      std::back_inserter(elems),
      [self](any_expr &&exp) -> any_expr { return self->eval(*exp); });

  array &res = deref(new array);

  res.set_operands(std::move(elems));

  calcres = any_expr(&res);
}

void evaluator::visit(merge &n) { reduce_sequence(n, operator_impl<merge>{}); }

void evaluator::visit(reduce &n) {
  any_expr arr = eval(n.operand(0));
  expr &expr = n.operand(1);
  any_expr accu = eval(n.operand(2));
  any_expr *acptr = &accu;

  auto op = [&expr, acptr,
             calclogger = &this->logger](array &arrop) -> any_expr {
    // non destructive predicate is required for evaluating and copying
    return std::accumulate(std::make_move_iterator(arrop.begin()),
                           std::make_move_iterator(arrop.end()),
                           std::move(*acptr),
                           sequence_reduction{expr, *calclogger});
  };

  calcres =
      with_type<array>(arr.get(), op, []() -> any_expr { return nullptr; });
}

void evaluator::visit(map &n) {
  any_expr arr = eval(n.operand(0));
  auto mapper = [&n, &arr,
                 calclogger = &this->logger](array &arrop) -> any_expr {
    expr &expr = n.operand(1);
    oper::container_type mapped_elements;

    std::transform(std::make_move_iterator(arrop.begin()),
                   std::make_move_iterator(arrop.end()),
                   std::back_inserter(mapped_elements),
                   sequence_function{expr, *calclogger});

    arrop.set_operands(std::move(mapped_elements));
    return std::move(arr);
  };

  calcres = with_type<array>(arr.get(), mapper,
                             []() -> any_expr { return any_expr{new array}; });
}

void evaluator::visit(filter &n) {
  any_expr arr = eval(n.operand(0));
  auto filter = [&n, &arr,
                 calclogger = &this->logger](array &arrop) -> any_expr {
    expr &expr = n.operand(1);
    oper::container_type filtered_elements;

    // non destructive predicate is required for evaluating and copying
    std::copy_if(std::make_move_iterator(arrop.begin()),
                 std::make_move_iterator(arrop.end()),
                 std::back_inserter(filtered_elements),
                 sequence_predicate_nondestructive{expr, *calclogger});

    arrop.set_operands(std::move(filtered_elements));
    return std::move(arr);
  };

  calcres = with_type<array>(arr.get(), filter,
                             []() -> any_expr { return any_expr{new array}; });
}

void evaluator::visit(all &n) {
  any_expr arr = eval(n.operand(0));
  array &elems = down_cast<array>(*arr); // evaluated elements
  expr &expr = n.operand(1);
  const bool res = std::all_of(std::make_move_iterator(elems.begin()),
                               std::make_move_iterator(elems.end()),
                               sequence_predicate{expr, logger});

  calcres = to_expr(res);
}

void evaluator::visit(none &n) {
  any_expr arr = eval(n.operand(0));
  array &elems = down_cast<array>(*arr); // evaluated elements
  expr &expr = n.operand(1);
  const bool res = std::none_of(std::make_move_iterator(elems.begin()),
                                std::make_move_iterator(elems.end()),
                                sequence_predicate{expr, logger});

  calcres = to_expr(res);
}

void evaluator::visit(some &n) {
  any_expr arr = eval(n.operand(0));
  array &elems = down_cast<array>(*arr); // evaluated elements
  expr &expr = n.operand(1);
  const bool res = std::any_of(std::make_move_iterator(elems.begin()),
                               std::make_move_iterator(elems.end()),
                               sequence_predicate{expr, logger});

  calcres = to_expr(res);
}

void evaluator::visit(error &) { unsupported(); }

void evaluator::visit(var &n) {
  assert(n.num_evaluated_operands() >= 1);

  any_expr elm = eval(n.operand(0));
  value_base &val = down_cast<value_base>(*elm);

  try {
    calcres = vars(val.to_json(), n.num());
  } catch (...) {
    calcres = (n.num_evaluated_operands() > 1) ? eval(n.operand(1))
                                               : to_expr(nullptr);
  }
}

std::size_t evaluator::missing_aux(array &elems) {
  auto avail = [calc = this](any_expr &v) -> bool {
    try {
      value_base &val = down_cast<value_base>(*v);

      calc->vars(val.to_json(), -1 /* logical_not membership varmap */);
    } catch (...) {
      return false;
    }

    return true;
  };

  array::iterator beg = elems.begin();
  array::iterator lim = elems.end();
  array::iterator pos = std::remove_if(beg, lim, avail);
  std::size_t res = std::distance(pos, lim);

  elems.operands().erase(pos, lim);
  return res;
}

void evaluator::visit(missing &n) {
  any_expr arg = eval(n.operand(0));
  auto non_array_alt = [&arg, &n, calc = this]() -> array & {
    array &res = deref(new array);

    res.set_operands(std::move(n).move_operands());
    res.operands().front().swap(arg);
    arg.reset(&res);
    calc->visit(res);

    return res;
  };

  array &elems = with_type<array>(
      arg.get(), [](array &arr) -> array & { return arr; }, // ignore other args
      non_array_alt);

  missing_aux(elems);
  calcres = std::move(arg);
}

void evaluator::visit(missing_some &n) {
  const std::uint64_t minreq = unpack_value<std::uint64_t>(eval(n.operand(0)));
  any_expr arr = eval(n.operand(1));
  array &elems = down_cast<array>(*arr); // evaluated elements
  std::size_t avail = missing_aux(elems);

  if (avail >= minreq)
    elems.operands().clear();

  calcres = std::move(arr);
}

void evaluator::visit(if_expr &n) {
  const int num = n.num_evaluated_operands();

  if (num == 0) {
    calcres = to_expr(nullptr);
    return;
  }

  const int lim = num - 1;
  int pos = 0;

  while (pos < lim) {
    if (truthy(eval(n.operand(pos)))) {
      calcres = eval(n.operand(pos + 1));
      return;
    }

    pos += 2;
  }

  calcres = (pos < num) ? eval(n.operand(pos)) : to_expr(nullptr);
}

void evaluator::visit(log &n) {
  assert(n.num_evaluated_operands() == 1);

  calcres = eval(n.operand(0));

  logger << calcres << std::endl;
}

void evaluator::visit(null_value &n) { _value(n); }
void evaluator::visit(bool_value &n) { _value(n); }
void evaluator::visit(int_value &n) { _value(n); }
void evaluator::visit(unsigned_int_value &n) { _value(n); }
void evaluator::visit(real_value &n) { _value(n); }
void evaluator::visit(string_value &n) { _value(n); }

any_expr apply(expr &exp, const variable_accessor &vars) {
  evaluator ev{vars, std::cerr};

  return ev.eval(exp);
}

any_expr eval_path(const json::string &path, const json::object &obj) {
  if (auto pos = obj.find(path); pos != obj.end())
    return jsonlogic::to_expr(pos->value());

  if (std::size_t pos = path.find('.'); pos != json::string::npos) {
    json::string selector = path.subview(0, pos);
    json::string suffix = path.subview(pos + 1);

    return eval_path(suffix, obj.at(selector).as_object());
  }

  throw std::out_of_range("jsonlogic - unable to locate path");
}

template <class IntT> any_expr eval_index(IntT idx, const json::array &arr) {
  return jsonlogic::to_expr(arr[idx]);
}

} // namespace

any_expr apply(const any_expr &exp, const variable_accessor &vars) {
  assert(exp.get());
  return apply(*exp, vars);
}

any_expr apply(const any_expr &exp) {
  return jsonlogic::apply(exp, [](const json::value &, int) -> any_expr {
    throw std::runtime_error{"variable not available"};
  });
}

variable_accessor data_accessor(json::value data) {
  return [data = std::move(data)](const json::value &keyval, int) -> any_expr {
    if (const json::string *ppath = keyval.if_string()) {
      //~ std::cerr << *ppath << std::endl;
      return ppath->size() ? eval_path(*ppath, data.as_object())
                           : to_expr(data);
    }

    if (const std::int64_t *pidx = keyval.if_int64())
      return eval_index(*pidx, data.as_array());

    if (const std::uint64_t *pidx = keyval.if_uint64())
      return eval_index(*pidx, data.as_array());

    throw std::logic_error{"jsonlogic - unsupported var access"};
  };
}

any_expr apply(json::value rule, json::value data) {
  logic_details logic = create_logic(rule);

  return jsonlogic::apply(logic.synatx_tree(), data_accessor(std::move(data)));
}

namespace {

struct value_printer : forwarding_visitor {
  explicit value_printer(std::ostream &stream) : os(stream) {}

  void prn(json::value val) { os << val; }

  void visit(expr &) final { unsupported(); }

  void visit(value_base &n) final { prn(n.to_json()); }

  void visit(array &n) final {
    bool first = true;

    os << "[";
    for (any_expr &el : n) {
      if (first)
        first = false;
      else
        os << ",";

      deref(el).accept(*this);
    }

    os << "]";
  }

private:
  std::ostream &os;
};

} // namespace

std::ostream &operator<<(std::ostream &os, any_expr &n) {
  value_printer prn{os};

  deref(n).accept(prn);
  return os;
}

expr &oper::operand(int n) const { return deref(this->at(n).get()); }
} // namespace jsonlogic

#if UNSUPPORTED_SUPPLEMENTAL

/// traverses the children of a node; does logical_not traverse grandchildren
void traverseChildren(visitor &v, const oper &node);
void traverseAllChildren(visitor &v, const oper &node);
void traverseChildrenReverse(visitor &v, const oper &node);

// only operators have children
void _traverseChildren(visitor &v, oper::const_iterator aa,
                       oper::const_iterator zz) {
  std::for_each(aa, zz, [&v](const any_expr &e) -> void { e->accept(v); });
}

void traverseChildren(visitor &v, const oper &node) {
  oper::const_iterator aa = node.begin();

  _traverseChildren(v, aa, aa + node.num_evaluated_operands());
}

void traverseAllChildren(visitor &v, const oper &node) {
  _traverseChildren(v, node.begin(), node.end());
}

void traverseChildrenReverse(visitor &v, const oper &node) {
  oper::const_reverse_iterator zz = node.crend();
  oper::const_reverse_iterator aa = zz - node.num_evaluated_operands();

  std::for_each(aa, zz, [&v](const any_expr &e) -> void { e->accept(v); });
}

namespace {
struct SAttributeTraversal : visitor {
  explicit SAttributeTraversal(visitor &client) : sub(client) {}

  void visit(expr &) final;
  void visit(oper &) final;
  void visit(equal &) final;
  void visit(strict_equal &) final;
  void visit(not_equal &) final;
  void visit(strict_not_equal &) final;
  void visit(less &) final;
  void visit(greater &) final;
  void visit(less_or_equal &) final;
  void visit(greater_or_equal &) final;
  void visit(logical_and &) final;
  void visit(logical_or &) final;
  void visit(logical_not &) final;
  void visit(logical_not_not &) final;
  void visit(add &) final;
  void visit(subtract &) final;
  void visit(multiply &) final;
  void visit(divide &) final;
  void visit(modulo &) final;
  void visit(min &) final;
  void visit(max &) final;
  void visit(map &) final;
  void visit(reduce &) final;
  void visit(filter &) final;
  void visit(all &) final;
  void visit(none &) final;
  void visit(some &) final;
  void visit(merge &) final;
  void visit(cat &) final;
  void visit(substr &) final;
  void visit(membership &) final;
  void visit(array &n) final;
  void visit(var &) final;
  void visit(log &) final;

  void visit(if_expr &) final;

  void visit(null_value &n) final;
  void visit(bool_value &n) final;
  void visit(int_value &n) final;
  void visit(unsigned_int_value &n) final;
  void visit(real_value &n) final;
  void visit(string_value &n) final;

  void visit(error &n) final;

private:
  visitor &sub;

  template <class OperatorNode> inline void _visit(OperatorNode &n) {
    traverseChildren(*this, n);
    sub.visit(n);
  }

  template <class ValueNode> inline void _value(ValueNode &n) { sub.visit(n); }
};

void SAttributeTraversal::visit(expr &) { throw_type_error(); }
void SAttributeTraversal::visit(oper &) { throw_type_error(); }
void SAttributeTraversal::visit(equal &n) { _visit(n); }
void SAttributeTraversal::visit(strict_equal &n) { _visit(n); }
void SAttributeTraversal::visit(not_equal &n) { _visit(n); }
void SAttributeTraversal::visit(strict_not_equal &n) { _visit(n); }
void SAttributeTraversal::visit(less &n) { _visit(n); }
void SAttributeTraversal::visit(greater &n) { _visit(n); }
void SAttributeTraversal::visit(less_or_equal &n) { _visit(n); }
void SAttributeTraversal::visit(greater_or_equal &n) { _visit(n); }
void SAttributeTraversal::visit(logical_and &n) { _visit(n); }
void SAttributeTraversal::visit(logical_or &n) { _visit(n); }
void SAttributeTraversal::visit(logical_not &n) { _visit(n); }
void SAttributeTraversal::visit(logical_not_not &n) { _visit(n); }
void SAttributeTraversal::visit(add &n) { _visit(n); }
void SAttributeTraversal::visit(subtract &n) { _visit(n); }
void SAttributeTraversal::visit(multiply &n) { _visit(n); }
void SAttributeTraversal::visit(divide &n) { _visit(n); }
void SAttributeTraversal::visit(modulo &n) { _visit(n); }
void SAttributeTraversal::visit(min &n) { _visit(n); }
void SAttributeTraversal::visit(max &n) { _visit(n); }
void SAttributeTraversal::visit(array &n) { _visit(n); }
void SAttributeTraversal::visit(map &n) { _visit(n); }
void SAttributeTraversal::visit(reduce &n) { _visit(n); }
void SAttributeTraversal::visit(filter &n) { _visit(n); }
void SAttributeTraversal::visit(all &n) { _visit(n); }
void SAttributeTraversal::visit(none &n) { _visit(n); }
void SAttributeTraversal::visit(some &n) { _visit(n); }
void SAttributeTraversal::visit(merge &n) { _visit(n); }
void SAttributeTraversal::visit(cat &n) { _visit(n); }
void SAttributeTraversal::visit(substr &n) { _visit(n); }
void SAttributeTraversal::visit(membership &n) { _visit(n); }
void SAttributeTraversal::visit(missing &n) { _visit(n); }
void SAttributeTraversal::visit(missing_some &n) { _visit(n); }
void SAttributeTraversal::visit(var &n) { _visit(n); }
void SAttributeTraversal::visit(log &n) { _visit(n); }

void SAttributeTraversal::visit(if_expr &n) { _visit(n); }

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
void SAttributeTraversal::visit(regex_match &n) { _visit(n); }
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */

void SAttributeTraversal::visit(null_value &n) { _value(n); }
void SAttributeTraversal::visit(bool_value &n) { _value(n); }
void SAttributeTraversal::visit(int_value &n) { _value(n); }
void SAttributeTraversal::visit(unsigned_int_value &n) { _value(n); }
void SAttributeTraversal::visit(real_value &n) { _value(n); }
void SAttributeTraversal::visit(string_value &n) { _value(n); }

void SAttributeTraversal::visit(error &n) { sub.visit(n); }
} // namespace

/// AST traversal function that calls v's visit methods membership post-fix
/// order
void traverseInSAttributeOrder(expr &e, visitor &vis);

void traverseInSAttributeOrder(expr &e, visitor &vis) {
  SAttributeTraversal trav{vis};

  e.accept(trav);
}
#endif /* SUPPLEMENTAL */
