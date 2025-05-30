/// implements jsonlogic functions

#include "jsonlogic/logic.hpp"

// standard headers
#include <algorithm>
#include <exception>
#include <iostream>
#include <limits>
#include <numeric>
#include <string>
#include <unordered_map>
#include <charconv>
#include <set> // alternatives are <deque>, <forward_list>

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
#include <regex>
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */

// 3rd party headers
#include <boost/json.hpp>

// jsonlogic complete headers
#include "jsonlogic/details/ast-full.hpp"
#include "jsonlogic/details/cxx-compat.hpp"

namespace {
constexpr bool DEBUG_OUTPUT = false;
constexpr int COMPUTED_VARIABLE_NAME = -1;
}  // namespace


namespace jsonlogic {

namespace json = boost::json;

/// type for string storage
/// \note if the compiler uses small string optimization, we cannot
///       use an std::vector because resizing invalidates the
///       string_views.
///       alternatives include forward_list or deque
using string_table_base = std::set<std::string>;

/// string table ensures lifetime of string exceeds lifetime of string_views
struct string_table : private string_table_base
{
  std::string_view safe_string(std::string v)
  {
    auto res = this->emplace(std::move(v));
    return *res.first;
  }

  template <class iterator>
  std::string_view safe_string(iterator aa, iterator zz)
  {
    return safe_string(std::string(aa, zz));
  }
};

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

//~ template <class T> T &up_cast(T &n) { return n; }
template <class T>
const T &up_cast(const T &n) {
  return n;
}

template <class T>
struct down_caster_internal {
  T *operator()(const expr &) const { return nullptr; }

  T *operator()(const T &o) const { return &o; }
};

template <class T>
const T *may_down_cast(const expr &e) {
  return generic_visit(down_caster_internal<T>{}, &e);
}

template <class T>
T *may_down_cast(expr &e) {
  return dynamic_cast<T *>(&e);  // \todo replace dynamic_cast
}

template <class T>
const T &down_cast(const expr &e) {
  if (T *casted = may_down_cast<T>(e)) {
    CXX_LIKELY;
    return *casted;
  }

  throw_type_error();
}

template <class T>
T &down_cast(expr &e) {
  if (T *casted = may_down_cast<T>(e)) {
    CXX_LIKELY;
    return *casted;
  }

  throw_type_error();
}
}  // namespace

//
// foundation classes
// \{

// accept implementations
void equal::accept(visitor &v) const { v.visit(*this); }
void strict_equal::accept(visitor &v) const { v.visit(*this); }
void not_equal::accept(visitor &v) const { v.visit(*this); }
void strict_not_equal::accept(visitor &v) const { v.visit(*this); }
void less::accept(visitor &v) const { v.visit(*this); }
void greater::accept(visitor &v) const { v.visit(*this); }
void less_or_equal::accept(visitor &v) const { v.visit(*this); }
void greater_or_equal::accept(visitor &v) const { v.visit(*this); }
void logical_and::accept(visitor &v) const { v.visit(*this); }
void logical_or::accept(visitor &v) const { v.visit(*this); }
void logical_not::accept(visitor &v) const { v.visit(*this); }
void logical_not_not::accept(visitor &v) const { v.visit(*this); }
void add::accept(visitor &v) const { v.visit(*this); }
void subtract::accept(visitor &v) const { v.visit(*this); }
void multiply::accept(visitor &v) const { v.visit(*this); }
void divide::accept(visitor &v) const { v.visit(*this); }
void modulo::accept(visitor &v) const { v.visit(*this); }
void min::accept(visitor &v) const { v.visit(*this); }
void max::accept(visitor &v) const { v.visit(*this); }
void map::accept(visitor &v) const { v.visit(*this); }
void reduce::accept(visitor &v) const { v.visit(*this); }
void filter::accept(visitor &v) const { v.visit(*this); }
void all::accept(visitor &v) const { v.visit(*this); }
void none::accept(visitor &v) const { v.visit(*this); }
void some::accept(visitor &v) const { v.visit(*this); }
void array::accept(visitor &v) const { v.visit(*this); }
void merge::accept(visitor &v) const { v.visit(*this); }
void cat::accept(visitor &v) const { v.visit(*this); }
void substr::accept(visitor &v) const { v.visit(*this); }
void membership::accept(visitor &v) const { v.visit(*this); }
void var::accept(visitor &v) const { v.visit(*this); }
void missing::accept(visitor &v) const { v.visit(*this); }
void missing_some::accept(visitor &v) const { v.visit(*this); }
void log::accept(visitor &v) const { v.visit(*this); }
void if_expr::accept(visitor &v) const { v.visit(*this); }

void null_value::accept(visitor &v) const { v.visit(*this); }
void bool_value::accept(visitor &v) const { v.visit(*this); }
void int_value::accept(visitor &v) const { v.visit(*this); }
void unsigned_int_value::accept(visitor &v) const { v.visit(*this); }
void real_value::accept(visitor &v) const { v.visit(*this); }
void string_value::accept(visitor &v) const { v.visit(*this); }
void object_value::accept(visitor &v) const { v.visit(*this); }

void error::accept(visitor &v) const { v.visit(*this); }

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
void regex_match::accept(visitor &v) const { v.visit(*this); }
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */

// to_json implementations
template <class T>
value_variant value_generic<T>::to_variant() const {
  return value();
}

value_variant null_value::to_variant() const { return value(); }


// num_evaluated_operands implementations
int oper::num_evaluated_operands() const { return size(); }

template <int MaxArity>
int oper_n<MaxArity>::num_evaluated_operands() const {
  return std::min(MaxArity, oper::num_evaluated_operands());
}

struct forwarding_visitor : visitor {
  void visit(const expr &) override {}  // error
  void visit(const oper &n) override { visit(up_cast<expr>(n)); }
  void visit(const equal &n) override { visit(up_cast<oper>(n)); }
  void visit(const strict_equal &n) override { visit(up_cast<oper>(n)); }
  void visit(const not_equal &n) override { visit(up_cast<oper>(n)); }
  void visit(const strict_not_equal &n) override { visit(up_cast<oper>(n)); }
  void visit(const less &n) override { visit(up_cast<oper>(n)); }
  void visit(const greater &n) override { visit(up_cast<oper>(n)); }
  void visit(const less_or_equal &n) override { visit(up_cast<oper>(n)); }
  void visit(const greater_or_equal &n) override { visit(up_cast<oper>(n)); }
  void visit(const logical_and &n) override { visit(up_cast<oper>(n)); }
  void visit(const logical_or &n) override { visit(up_cast<oper>(n)); }
  void visit(const logical_not &n) override { visit(up_cast<oper>(n)); }
  void visit(const logical_not_not &n) override { visit(up_cast<oper>(n)); }
  void visit(const add &n) override { visit(up_cast<oper>(n)); }
  void visit(const subtract &n) override { visit(up_cast<oper>(n)); }
  void visit(const multiply &n) override { visit(up_cast<oper>(n)); }
  void visit(const divide &n) override { visit(up_cast<oper>(n)); }
  void visit(const modulo &n) override { visit(up_cast<oper>(n)); }
  void visit(const min &n) override { visit(up_cast<oper>(n)); }
  void visit(const max &n) override { visit(up_cast<oper>(n)); }
  void visit(const map &n) override { visit(up_cast<oper>(n)); }
  void visit(const reduce &n) override { visit(up_cast<oper>(n)); }
  void visit(const filter &n) override { visit(up_cast<oper>(n)); }
  void visit(const all &n) override { visit(up_cast<oper>(n)); }
  void visit(const none &n) override { visit(up_cast<oper>(n)); }
  void visit(const some &n) override { visit(up_cast<oper>(n)); }
  void visit(const merge &n) override { visit(up_cast<oper>(n)); }
  void visit(const cat &n) override { visit(up_cast<oper>(n)); }
  void visit(const substr &n) override { visit(up_cast<oper>(n)); }
  void visit(const membership &n) override { visit(up_cast<oper>(n)); }
  void visit(const var &n) override { visit(up_cast<oper>(n)); }
  void visit(const missing &n) override { visit(up_cast<oper>(n)); }
  void visit(const missing_some &n) override { visit(up_cast<oper>(n)); }
  void visit(const log &n) override { visit(up_cast<oper>(n)); }

  void visit(const if_expr &n) override { visit(up_cast<expr>(n)); }

  void visit(const value_base &n) override { visit(up_cast<expr>(n)); }
  void visit(const null_value &n) override { visit(up_cast<value_base>(n)); }
  void visit(const bool_value &n) override { visit(up_cast<value_base>(n)); }
  void visit(const int_value &n) override { visit(up_cast<value_base>(n)); }
  void visit(const unsigned_int_value &n) override {
    visit(up_cast<value_base>(n));
  }
  void visit(const real_value &n) override { visit(up_cast<value_base>(n)); }
  void visit(const string_value &n) override { visit(up_cast<value_base>(n)); }

  void visit(const array &n) override { visit(up_cast<oper>(n)); }
  void visit(const object_value &n) override { visit(up_cast<expr>(n)); }

  void visit(const error &n) override { visit(up_cast<expr>(n)); }

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
  // extensions
  void visit(const regex_match &n) override { visit(up_cast<expr>(n)); }
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */
};

namespace {

struct variable_map {
  void insert(var &el);
  std::vector<json::string> to_vector() const;

  /// accessors for withComputedNames
  /// \{
  bool hasComputedVariables() const { return withComputedNames; }
  void setComputedVariables(bool b) { withComputedNames = b; }
  /// \}

 private:
  using container_type = std::map<json::string, int>;

  container_type mapping = {};
  bool withComputedNames = false;
};

void variable_map::insert(var &var) {
  try {
    any_expr &arg = var.back();
    string_value &str = down_cast<string_value>(*arg);
    const bool comp = (str.value().find('.') != json::string::npos &&
                       str.value().find('[') != json::string::npos);

    if (comp) {
      setComputedVariables(true);
    } else if (str.value() != "") {
      // do nothing for free variables in "lambdas"
      auto [pos, success] = mapping.emplace(str.value(), mapping.size());

      var.num(pos->second);
    }
  } catch (const type_error &) {
    setComputedVariables(true);
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
oper::container_type translate_children(const json::array &children, variable_map &, string_table&);

oper::container_type translate_children(const json::value &n, variable_map &, string_table&);
/// \}

template <class ExprT>
ExprT &mkOperator_(const json::object &n, variable_map &m, string_table& strings) {
  assert(n.size() == 1);

  ExprT &res = deref(new ExprT);

  res.set_operands(translate_children(n.begin()->value(), m, strings));
  return res;
}

template <class ExprT>
expr &mk_operator(const json::object &n, variable_map &m, string_table& strings) {
  return mkOperator_<ExprT>(n, m, strings);
}

template <class ExprT>
expr &mk_missing(const json::object &n, variable_map &m, string_table& strings) {
  // \todo * extract variables from array and only setComputedVariables when
  // needed.
  //       * check reference implementation when missing is null
  m.setComputedVariables(true);
  return mkOperator_<ExprT>(n, m, strings);
}

expr &mk_variable(const json::object &n, variable_map &m, string_table& strings) {
  var &v = mkOperator_<var>(n, m, strings);

  m.insert(v);
  return v;
}

array &mk_array(const json::array &children, variable_map &m, string_table& strings) {
  array &res = deref(new array);

  res.set_operands(translate_children(children, m, strings));
  return res;
}

template <class value_t>
value_t &mk_value(typename value_t::value_type n) {
  return deref(new value_t(std::move(n)));
}

null_value &mk_null_value() { return deref(new null_value); }

using dispatch_table =
    std::map<std::string_view, expr &(*)(const json::object &, variable_map &, string_table&)>;

dispatch_table::const_iterator lookup(const dispatch_table &m,
                                      const json::object &op) {
  if (op.size() != 1) return m.end();

  const json::string& key = op.begin()->key();
  std::string_view    keyvw(&*key.begin(), key.size());

  return m.find(keyvw);
}

any_expr translate_internal(const json::value& n, variable_map &varmap, string_table &strings) {
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
      {"in", &mk_operator<membership>},
      {"cat", &mk_operator<cat>},
      {"log", &mk_operator<log>},
      {"var", &mk_variable},
      {"missing", &mk_missing<missing>},
      {"missing_some", &mk_missing<missing_some>},
#if WITH_JSON_LOGIC_CPP_EXTENSIONS
      /// extensions
      {"regex", &mk_operator<regex_match>},
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */
  };

  expr *res = nullptr;

  switch (n.kind()) {
    case json::kind::object: {
      const json::object &obj = n.get_object();
      dispatch_table::const_iterator pos = lookup(dt, obj);

      if (pos != dt.end()) {
        CXX_LIKELY;
        res = &pos->second(obj, varmap, strings);
      } else {
        // does jsonlogic support value objects?
        unsupported();
      }

      break;
    }

    case json::kind::array: {
      // array is an operator that combines its subexpressions into an array
      res = &mk_array(n.get_array(), varmap, strings);
      break;
    }

    case json::kind::string: {
      const json::string& str = n.get_string();
      res = &mk_value<string_value>(strings.safe_string(str.begin(), str.end()));
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

oper::container_type translate_children(const json::array &children,
                                        variable_map &varmap,
                                        string_table& strings) {
  oper::container_type res;

  res.reserve(children.size());

  for (const json::value &elem : children)
    res.emplace_back(translate_internal(elem, varmap, strings));

  return res;
}

oper::container_type translate_children(const json::value &n, variable_map &varmap, string_table& strings) {
  if (const json::array *arr = n.if_array()) {
    CXX_LIKELY;
    return translate_children(*arr, varmap, strings);
  }

  oper::container_type res;

  res.emplace_back(translate_internal(n, varmap, strings));
  return res;
}
}  // namespace

logic_rule_base create_logic(const json::value& n) {
  static string_table strings;
  variable_map varmap;
  any_expr node = translate_internal(n, varmap, strings);
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
//~ no longer supported
//~ any_expr to_expr(json::string val) {
  //~ return any_expr(new string_value(safe_string(val.begin(), val.end())));
//~ }
any_expr to_expr(std::string_view val) {
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
      const json::string& str = n.get_string(); // \todo this may be unsafe..
      res = to_expr(std::string_view(&*str.begin(), str.size()));
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

any_expr to_expr(value_variant val) {
  // guard against accidental variant modification
  enum { mono_variant = 0,
         null_variant = 1,
         bool_variant = 2,
         int_variant  = 3,
         uint_variant = 4,
         real_variant = 5,
         strv_variant = 6,
         json_variant = 7
       };

  static_assert(std::variant_size_v<value_variant> == 8);
  static_assert(std::is_same_v<std::monostate,
                               std::variant_alternative_t<mono_variant, value_variant> >);
  static_assert(std::is_same_v<std::nullptr_t,
                               std::variant_alternative_t<null_variant, value_variant> >);
  static_assert(std::is_same_v<bool,
                               std::variant_alternative_t<bool_variant, value_variant> >);
  static_assert(std::is_same_v<std::int64_t,
                               std::variant_alternative_t<int_variant, value_variant> >);
  static_assert(std::is_same_v<std::uint64_t,
                               std::variant_alternative_t<uint_variant, value_variant> >);
  static_assert(std::is_same_v<double,
                               std::variant_alternative_t<real_variant, value_variant> >);
  static_assert(std::is_same_v<std::string_view,
                               std::variant_alternative_t<strv_variant, value_variant> >);
  static_assert(std::is_same_v<boost::json::value,
                               std::variant_alternative_t<json_variant, value_variant> >);

  any_expr res;

  switch (val.index()) {
    case mono_variant: {
      throw variable_resolution_error{"in logic.c::to_expr(value_variant)"};
    }

    case null_variant: {
      res = to_expr(nullptr);
      break;
    }

    case bool_variant: {
      res = to_expr(std::get<bool>(val));
      break;
    }

    case int_variant: {
      res = to_expr(std::get<std::int64_t>(val));
      break;
    }

    case uint_variant: {
      res = to_expr(std::get<std::uint64_t>(val));
      break;
    }

    case real_variant: {
      res = to_expr(std::get<double>(val));
      break;
    }

    case strv_variant: {
      res = to_expr(std::get<std::string_view>(val));
      break;
    }

    case json_variant: {
      res = to_expr(std::get<boost::json::value>(val));
      break;
    }

    default:
      // did val hold a valid value?
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


template <class T>
T from_string(std::string_view str, T el) {
  auto [ptr, err] = std::from_chars(str.data(), str.data() + str.size(), el);

  if (err != std::errc{}) {
    CXX_UNLIKELY;
    throw std::runtime_error{"logic.cc: in to_concrete(string_view, int64_t)"};
  }

  return el;
}

/// conversion to int64
/// \{
inline std::int64_t to_concrete(std::int64_t v, const std::int64_t &) {
  return v;
}
inline std::int64_t to_concrete(std::string_view str, const std::int64_t &el) {
  return from_string(str, el);
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
inline std::uint64_t to_concrete(std::string_view str, const std::uint64_t &el) {
  return from_string(str, el);
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
inline double to_concrete(std::string_view str, const double &el) {
  return from_string(str, el);
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
inline std::string_view to_concrete(Val v, const std::string_view &, string_table& strings) {
  return strings.safe_string(std::to_string(v));
}
inline std::string_view to_concrete(bool v, const std::string_view &, string_table&) {
  static constexpr const char* bool_string[] = {"false", "true"};

  return bool_string[v];
}
inline std::string_view to_concrete(const std::string_view &s, const std::string_view &, string_table&) {
  return s;
}
inline std::string_view to_concrete(std::nullptr_t, const std::string_view &, string_table&) {
  static constexpr const char* null_string = "null";

  return null_string;
}
/// \}

/// conversion to boolean
///   implements truthy, falsy as described by https://jsonlogic.com/truthy.html
/// \{
inline bool to_concrete(bool v, const bool &) { return v; }
inline bool to_concrete(std::int64_t v, const bool &) { return v; }
inline bool to_concrete(std::uint64_t v, const bool &) { return v; }
inline bool to_concrete(double v, const bool &) { return v; }
inline bool to_concrete(const std::string_view &v, const bool &) {
  return v.size() != 0;
}
inline bool to_concrete(std::nullptr_t, const bool &) { return false; }

// \todo logical_not sure if conversions from arrays to values should be
// supported like this
inline bool to_concrete(const array &v, const bool &) {
  return v.num_evaluated_operands();
}
/// \}


template <typename T, typename U, typename = void>
struct to_concrete_nostrings : std::false_type {};

template <typename T, typename U>
struct to_concrete_nostrings<T, U, std::void_t<decltype(to_concrete(std::declval<T>(), std::declval<const U&>())) >>
       : std::true_type {};

//~ template< class T, class U> inline constexpr bool to_concrete_nostrings_v =
          //~ to_concrete_nostrings<T, U>::value;

template <class T, class U>
U to_concrete_(T&& v, const U& u, string_table& strings)
{
  if constexpr (to_concrete_nostrings<T, U>::value)
    return to_concrete(std::forward<T>(v), u);
  else
    return to_concrete(std::forward<T>(v), u, strings);
}



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

template <class T>
T to_calc_type(const T *val) {
  return *val;
}

std::string_view to_calc_type(const std::string_view *val) {
  return *val;
}
/*
std::nullptr_t
to_calc_type(std::nullptr_t el)
{
  return el;
}
*/
const array *to_calc_type(const array *arr) { return arr; }

/// \brief a strict equality operator operates on operands of the same
///        type. The operation on two different types returns false.
///        NO type coercion is performed.
struct strict_equality_operator : comparison_operator_base {
  std::tuple<bool, bool> coerce(const array *, const array *) {
    // arrays are never equal
    // \todo arrays may be equal if they have the same reference
    //       i.e., if they originate from the same variable
    // return {true, lhs==rhs};
    return {true, false};
  }

  template <class LhsT, class RhsT>
  std::tuple<decltype(to_calc_type(std::declval<const LhsT *>())),
             decltype(to_calc_type(std::declval<const RhsT *>()))>
  coerce(const LhsT *lv, const RhsT *rv) {
    return {to_calc_type(lv), to_calc_type(rv)};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    std::nullptr_t) {
    return {nullptr, nullptr};  // two null pointers are equal
  }

  template <class LhsT>
  std::tuple<decltype(to_calc_type(std::declval<const LhsT *>())),
             std::nullptr_t>
  coerce(const LhsT *lv, std::nullptr_t) {
    return {to_calc_type(lv), nullptr};
  }

  template <class RhsT>
  std::tuple<std::nullptr_t,
             decltype(to_calc_type(std::declval<const RhsT *>()))>
  coerce(std::nullptr_t, const RhsT *rv) {
    return {nullptr, to_calc_type(rv)};
  }
};

struct numeric_binary_operator_base {
  std::tuple<double, double> coerce(const double *lv, const double *rv) {
    return {*lv, *rv};
  }

  std::tuple<double, double> coerce(const double *lv, const std::int64_t *rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<double, double> coerce(const double *lv, const std::uint64_t *rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<double, double> coerce(const std::int64_t *lv, const double *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(const std::int64_t *lv,
                                                const std::int64_t *rv) {
    return {*lv, *rv};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(const std::int64_t *lv,
                                                const std::uint64_t *rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<double, double> coerce(const std::uint64_t *lv, const double *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(const std::uint64_t *lv,
                                                const std::int64_t *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(const std::uint64_t *lv,
                                                  const std::uint64_t *rv) {
    return {*lv, *rv};
  }
};

/// \brief an equality operator compares two values. if_expr the
///        values have a different type, type coercion is performed
///        on one of the operands.
struct relational_operator_base : numeric_binary_operator_base {
  using numeric_binary_operator_base::coerce;

  std::tuple<double, double> coerce(const double *lv, const std::string_view* rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<double, double> coerce(const double *lv, const bool *rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(const std::int64_t *lv,
                                                const std::string_view* rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(const std::int64_t *lv,
                                                const bool *rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(const std::uint64_t *lv,
                                                  const std::string_view* rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(const std::uint64_t *lv,
                                                  const bool *rv) {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<double, double> coerce(const std::string_view* lv, const double *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<double, double> coerce(const bool *lv, const double *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(const std::string_view* lv,
                                                const std::int64_t *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(const bool *lv,
                                                const std::int64_t *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(const std::string_view* lv,
                                                  const std::uint64_t *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(const bool *lv,
                                                  const std::uint64_t *rv) {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<bool, bool> coerce(const std::string_view *, const bool *) {
    // strings and boolean are never equal
    return {true, false};
  }

  std::tuple<bool, bool> coerce(const bool *, const std::string_view *) {
    // strings and boolean are never equal
    return {true, false};
  }

  std::tuple<std::string_view, std::string_view> coerce(
      const std::string_view *lv, const std::string_view *rv) {
    return {*lv, *rv};
  }

  std::tuple<bool, bool> coerce(const bool *lv, const bool *rv) {
    return {*lv, *rv};
  }
};

struct equality_operator : relational_operator_base, comparison_operator_base {
  using relational_operator_base::coerce;

  // due to special conversion rules, the coercion function may just produce
  //   the result instead of just unpacking and coercing values.

  std::tuple<bool, bool> coerce(const array *, const array *) {
    return {true, false};  // arrays are never equal
  }

  template <class T>
  std::tuple<bool, bool> coerce(const T *lv, const array *rv) {
    // an array may be compared to a value_base
    //   (1) *lv == arr[0], iff the array has exactly one element
    if (rv->num_evaluated_operands() == 1) throw unpacked_array_req{};

    //   (2) or if [] and *lv converts to false
    if (rv->num_evaluated_operands() > 1) return {false, true};

    const bool convToFalse = to_concrete(*lv, false) == false;

    return {convToFalse, true /* zero elements */};
  }

  template <class T>
  std::tuple<bool, bool> coerce(const array *lv, const T *rv) {
    // see comments in coerce(T*,array*)
    if (lv->num_evaluated_operands() == 1) throw unpacked_array_req{};

    if (lv->num_evaluated_operands() > 1) return {false, true};

    const bool convToFalse = to_concrete(*rv, false) == false;

    return {true /* zero elements */, convToFalse};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    std::nullptr_t) {
    return {nullptr, nullptr};  // two null pointers are equal
  }

  template <class T>
  std::tuple<bool, bool> coerce(const T *, std::nullptr_t) {
    return {false, true};  // null pointer is only equal to itself
  }

  template <class T>
  std::tuple<bool, bool> coerce(std::nullptr_t, const T *) {
    return {true, false};  // null pointer is only equal to itself
  }

  std::tuple<bool, bool> coerce(const array *, std::nullptr_t) {
    return {true, false};  // null pointer is only equal to itself
  }

  std::tuple<bool, bool> coerce(std::nullptr_t, const array *) {
    return {true, false};  // null pointer is only equal to itself
  }
};

struct relational_operator : relational_operator_base,
                             comparison_operator_base {
  using relational_operator_base::coerce;

  std::tuple<const array *, const array *> coerce(const array *lv,
                                                  const array *rv) {
    return {lv, rv};
  }

  template <class T>
  std::tuple<bool, bool> coerce(const T *lv, const array *rv) {
    // an array may be equal to another value_base if
    //   (1) *lv == arr[0], iff the array has exactly one element
    if (rv->num_evaluated_operands() == 1) throw unpacked_array_req{};

    //   (2) or if [] and *lv converts to false
    if (rv->num_evaluated_operands() > 1) return {false, true};

    const bool convToTrue = to_concrete(*lv, true) == true;

    return {convToTrue, false /* zero elements */};
  }

  template <class T>
  std::tuple<bool, bool> coerce(const array *lv, const T *rv) {
    // see comments in coerce(T*,array*)
    if (lv->num_evaluated_operands() == 1) throw unpacked_array_req{};

    if (lv->num_evaluated_operands() > 1) return {false, true};

    const bool convToTrue = to_concrete(*rv, true) == true;

    return {false /* zero elements */, convToTrue};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    std::nullptr_t) {
    return {nullptr, nullptr};  // two null pointers are equal
  }

  std::tuple<bool, bool> coerce(const bool *lv, std::nullptr_t) {
    return {*lv, false};  // null pointer -> false
  }

  std::tuple<std::int64_t, std::int64_t> coerce(const std::int64_t *lv,
                                                std::nullptr_t) {
    return {*lv, 0};  // null pointer -> 0
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(const std::uint64_t *lv,
                                                  std::nullptr_t) {
    return {*lv, 0};  // null pointer -> 0
  }

  std::tuple<double, double> coerce(const double *lv, std::nullptr_t) {
    return {*lv, 0};  // null pointer -> 0.0
  }

  std::tuple<std::string_view, std::nullptr_t> coerce(const std::string_view *lv,
                                                      std::nullptr_t) {
    return {to_calc_type(lv), nullptr};  // requires special handling
  }

  std::tuple<bool, bool> coerce(std::nullptr_t, const bool *rv) {
    return {false, *rv};  // null pointer -> false
  }

  std::tuple<std::int64_t, std::int64_t> coerce(std::nullptr_t,
                                                const std::int64_t *rv) {
    return {0, *rv};  // null pointer -> 0
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(std::nullptr_t,
                                                  const std::uint64_t *rv) {
    return {0, *rv};  // null pointer -> 0
  }

  std::tuple<double, double> coerce(std::nullptr_t, const double *rv) {
    return {0, *rv};  // null pointer -> 0
  }

  std::tuple<std::nullptr_t, std::string_view> coerce(std::nullptr_t,
                                                      const std::string_view *rv) {
    return {nullptr, to_calc_type(rv)};  // requires special handling
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

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(const double *,
                                                    std::nullptr_t) {
    return {nullptr, nullptr};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(const std::int64_t *,
                                                    std::nullptr_t) {
    return {nullptr, nullptr};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(const std::uint64_t *,
                                                    std::nullptr_t) {
    return {nullptr, nullptr};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    const double *) {
    return {nullptr, nullptr};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    const std::int64_t *) {
    return {nullptr, nullptr};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    const std::uint64_t *) {
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

struct string_operator_non_destructive {
  enum {
    defined_for_string = true,
    defined_for_real = false,
    defined_for_integer = false,
    defined_for_boolean = false,
    defined_for_null = false,
    defined_for_array = false
  };

  using result_type = any_expr;

  std::tuple<std::string_view, std::string_view> coerce(
      const std::string_view *lv, const std::string_view *rv) {
    return {*lv, *rv};
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

  std::tuple<const array *, const array *> coerce(const array *lv,
                                                  const array *rv) {
    return {lv, rv};
  }
};

/*
  any_expr convert(any_expr val, ...)
  {
    return val;
  }
*/

any_expr convert(any_expr val, string_table&, const arithmetic_operator &) {
  struct arithmetic_converter : forwarding_visitor {
    explicit arithmetic_converter(any_expr val) : res(std::move(val)) {}

    void visit(const expr &) final { throw_type_error(); }

    // defined for the following types
    void visit(const int_value &) final {}
    void visit(const unsigned_int_value &) final {}
    void visit(const real_value &) final {}
    void visit(const null_value &) final {}

    // need to convert values
    void visit(const string_value &el) final {
      double dd = to_concrete(el.value(), double{});
      std::int64_t ii = to_concrete(el.value(), std::int64_t{});
      // uint?

      res = (dd != ii) ? to_expr(dd) : to_expr(ii);
    }

    void visit(const bool_value &) final {
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

any_expr convert(any_expr val, string_table& strings, const string_operator_non_destructive &) {
  struct string_converter : forwarding_visitor {
    string_converter(any_expr val, string_table& strtab) : res(std::move(val)), strings(strtab) {}

    void visit(const expr &) final { throw_type_error(); }

    // defined for the following types
    void visit(const string_value &) final {}

    // need to convert values
    void visit(const bool_value &el) final {
      res = to_expr(to_concrete(el.value(), std::string_view{}, strings));
    }

    void visit(const int_value &el) final {
      res = to_expr(to_concrete(el.value(), std::string_view{}, strings));
    }

    void visit(const unsigned_int_value &el) final {
      res = to_expr(to_concrete(el.value(), std::string_view{}, strings));
    }

    void visit(const real_value &el) final {
      res = to_expr(to_concrete(el.value(), std::string_view{}, strings));
    }

    void visit(const null_value &el) final {
      res = to_expr(to_concrete(el.value(), std::string_view{}, strings));
    }

    any_expr result() && { return std::move(res); }

   private:
    any_expr res;
    string_table& strings;
  };

  expr *node = val.get();
  string_converter conv{std::move(val), strings};

  node->accept(conv);
  return std::move(conv).result();
}

any_expr convert(any_expr val, string_table&, const array_operator &) {
  struct array_converter : forwarding_visitor {
    explicit array_converter(any_expr val) : val(std::move(val)) {}

    void visit(const expr &) final { throw_type_error(); }

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
    void visit(const array &) final {}

    // need to move value_base to new array
    void visit(const string_value &) final { to_array(); }
    void visit(const bool_value &) final { to_array(); }
    void visit(const int_value &) final { to_array(); }
    void visit(const unsigned_int_value &) final { to_array(); }
    void visit(const real_value &) final { to_array(); }
    void visit(const null_value &) final { to_array(); }

    any_expr result() && { return std::move(val); }

   private:
    any_expr val;
  };

  expr *node = val.get();
  array_converter conv{std::move(val)};

  node->accept(conv);
  return std::move(conv).result();
}

template <class value_t>
struct unpacker : forwarding_visitor {

  explicit
  unpacker(string_table& strtab)
  : strings(strtab), res()
  {}

  void assign(value_t &lhs, const value_t &val) { lhs = val; }

  template <class U>
  void assign(value_t &lhs, const U &val) {
    lhs = to_concrete_(val, lhs, strings);
  }

  CXX_NORETURN
  void visit(const expr &) final { throw_type_error(); }

  // defined for the following types
  void visit(const string_value &el) final { assign(res, el.value()); }

  // need to convert values
  void visit(const bool_value &el) final { assign(res, el.value()); }

  void visit(const int_value &el) final { assign(res, el.value()); }

  void visit(const unsigned_int_value &el) final { assign(res, el.value()); }

  void visit(const real_value &el) final { assign(res, el.value()); }

  void visit(const null_value &el) final { assign(res, el.value()); }

  void visit(const array &el) final {
    if constexpr (std::is_same<value_t, bool>::value) {
      CXX_LIKELY;
      return assign(res, el);
    }

    throw_type_error();
  }

  value_t result() && { return std::move(res); }

 private:
  string_table& strings;
  value_t res;
};

template <class T>
T unpack_value(const expr &expr, string_table& strings) {
  unpacker<T> unpack{strings};

  expr.accept(unpack);
  return std::move(unpack).result();
}

template <class T>
T unpack_value(const any_expr &el, string_table& strings) {
  return unpack_value<T>(*el, strings);
}


//
// Json Logic - truthy/falsy
bool truthy(const expr &el)
{
  string_table tmp;

  return unpack_value<bool>(el, tmp);
}

bool falsy(const expr &el) { return !truthy(el); }
// bool falsy(any_expr&& el)  { return !truthy(std::move(el)); }
}  // namespace

bool truthy(const any_expr &el) { return truthy(*el); }
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

  template <class oper_t>
  expr &clone(const oper_t &n, const oper &) const {
    return init(n, deref(new oper_t));
  }

  expr &clone(const object_value &n, const object_value &) const {
    return init(n, deref(new object_value));
  }
  /// \}

  template <class expr_t>
  expr *operator()(expr_t &n) {
    return &clone(n, n);
  }
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

  template <class rhs_value_t>
  void calc(rhs_value_t rv) {
    auto [ll, rr] = op.coerce(lv, rv);

    //~ std::cerr << lv << " -- " << rv << " " << typeid(ll).name()
              //~ << std::endl;
    res = op(std::move(ll), std::move(rr));
    //~ std::cerr << lv << " == " << rv << std::endl;
  }

  void visit(const expr &) final { throw_type_error(); }

  void visit(const string_value &n) final {
    if constexpr (binary_op_t::defined_for_string) return calc(&n.value());

    throw_type_error();
  }

  void visit(const null_value &) final {
    if constexpr (binary_op_t::defined_for_null) return calc(nullptr);

    throw_type_error();
  }

  void visit(const bool_value &n) final {
    if constexpr (binary_op_t::defined_for_boolean) return calc(&n.value());

    throw_type_error();
  }

  void visit(const int_value &n) final {
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

  void visit(const unsigned_int_value &n) final {
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

  void visit(const real_value &n) final {
    if constexpr (binary_op_t::defined_for_real) return calc(&n.value());

    throw_type_error();
  }

  void visit(const array &n) final {
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

  binary_operator_visitor(binary_op_t oper, const any_expr &rhsarg)
      : op(oper), rhs(rhsarg), res() {}

  template <class LhsValue>
  void calc(LhsValue lv) {
    using rhs_visitor = binary_operator_visitor_2<binary_op_t, LhsValue>;

    rhs_visitor vis{lv, op};

    rhs->accept(vis);
    res = std::move(vis).result();
  }

  void visit(const string_value &n) final {
    if constexpr (binary_op_t::defined_for_string) return calc(&n.value());

    throw_type_error();
  }

  void visit(const null_value &) final {
    if constexpr (binary_op_t::defined_for_null) return calc(nullptr);

    throw_type_error();
  }

  void visit(const bool_value &n) final {
    if constexpr (binary_op_t::defined_for_boolean) return calc(&n.value());

    throw_type_error();
  }

  void visit(const int_value &n) final {
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

  void visit(const unsigned_int_value &n) final {
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

  void visit(const real_value &n) final {
    if constexpr (binary_op_t::defined_for_real) return calc(&n.value());

    throw_type_error();
  }

  void visit(const array &n) final {
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
  const any_expr &rhs;
  result_type res;
};

//
// compute and sequence functions

template <class binary_op_t>
typename binary_op_t::result_type compute(const any_expr &lhs,
                                          const any_expr &rhs, binary_op_t op) {
  using lhs_visitor = binary_operator_visitor<binary_op_t>;

  assert(lhs.get() && rhs.get());

  lhs_visitor vis{op, rhs};

  lhs->accept(vis);
  return std::move(vis).result();
}

template <class binary_predicate_t>
bool compare_sequence(const array &lv, const array &rv,
                      binary_predicate_t pred) {
  const std::size_t lsz = lv.num_evaluated_operands();
  const std::size_t rsz = rv.num_evaluated_operands();

  if (lsz == 0) return pred(false, rsz != 0);

  if (rsz == 0) return pred(true, false);

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
bool compare_sequence(const array *lv, const array *rv,
                      binary_predicate_t pred) {
  return compare_sequence(deref(lv), deref(rv), std::move(pred));
}

/// convenience template that only returns \p R
/// when the types \p U and \p V mismatch.
/// matching \p U and \p V trigger a SFINAE error to invoke
/// the regular operator.
/// \tparam U first operator type
/// \tparam V second operator type
/// \tparam R operator result type
/// \{
template <class U, class V, class R>
struct mismatched_types {
  using type = R;
};

template <class T, class R>
struct mismatched_types<T, T, R> {
  // missing "using type" triggers SFINAE exclusion for matching types
};
/// \}

//
// the calc operator implementations

template <class>
struct operator_impl {};

template <>
struct operator_impl<equal> : equality_operator {
  using equality_operator::result_type;

  template <class U, class V>
  auto operator()(const U &, const V &) const ->
      typename mismatched_types<U, V, result_type>::type {
    return false;
  }

  template <class T>
  auto operator()(const T &lhs, const T &rhs) const -> result_type {
    return lhs == rhs;
  }
};

template <>
struct operator_impl<not_equal> : equality_operator {
  using equality_operator::result_type;

  template <class U, class V>
  auto operator()(const U &, const V &) const ->
      typename mismatched_types<U, V, result_type>::type {
    return true;
  }

  template <class T>
  auto operator()(const T &lhs, const T &rhs) const -> result_type {
    return lhs != rhs;
  }
};

template <>
struct operator_impl<strict_equal> : strict_equality_operator {
  using strict_equality_operator::result_type;

  template <class U, class V>
  auto operator()(const U &, const V &) const ->
      typename mismatched_types<U, V, result_type>::type {
    return false;
  }

  template <class T>
  auto operator()(const T &lhs, const T &rhs) const -> result_type {
    // std::cerr << lhs << " <> " << rhs << std::endl;
    return lhs == rhs;
  }
};

template <>
struct operator_impl<strict_not_equal> : strict_equality_operator {
  using strict_equality_operator::result_type;

  template <class U, class V>
  auto operator()(const U &, const V &) const ->
      typename mismatched_types<U, V, result_type>::type {
    return true;
  }

  template <class T>
  auto operator()(const T &lhs, const T &rhs) const -> result_type {
    return lhs != rhs;
  }
};

template <>
struct operator_impl<less> : relational_operator {
  using relational_operator::result_type;

  result_type operator()(const std::nullptr_t, std::nullptr_t) const {
    return false;
  }

  result_type operator()(const array *lv, const array *rv) const {
    return compare_sequence(lv, rv, *this);
  }

  result_type operator()(std::string_view, std::nullptr_t) const {
    return false;
  }

  result_type operator()(std::nullptr_t, std::string_view) const {
    return false;
  }

  template <class T>
  result_type operator()(const T &lhs, const T &rhs) const {
    return lhs < rhs;
  }
};

template <>
struct operator_impl<greater> : relational_operator {
  using relational_operator::result_type;

  result_type operator()(const std::nullptr_t, std::nullptr_t) const {
    return false;
  }

  result_type operator()(const array *lv, const array *rv) const {
    return compare_sequence(lv, rv, *this);
  }

  result_type operator()(std::string_view, std::nullptr_t) const {
    return false;
  }

  result_type operator()(std::nullptr_t, std::string_view) const {
    return false;
  }

  template <class T>
  result_type operator()(const T &lhs, const T &rhs) const {
    return rhs < lhs;
  }
};

template <>
struct operator_impl<less_or_equal> : relational_operator {
  using relational_operator::result_type;

  result_type operator()(const std::nullptr_t, std::nullptr_t) const {
    return true;
  }

  result_type operator()(const array *lv, const array *rv) const {
    return compare_sequence(lv, rv, *this);
  }

  result_type operator()(std::string_view lhs, std::nullptr_t) const {
    return lhs.empty();
  }

  result_type operator()(std::nullptr_t, std::string_view rhs) const {
    return rhs.empty();
  }

  template <class T>
  result_type operator()(const T &lhs, const T &rhs) const {
    return lhs <= rhs;
  }
};

template <>
struct operator_impl<greater_or_equal> : relational_operator {
  using relational_operator::result_type;

  result_type operator()(const std::nullptr_t, std::nullptr_t) const {
    return true;
  }

  result_type operator()(const array *lv, const array *rv) const {
    return compare_sequence(lv, rv, *this);
  }

  result_type operator()(std::string_view lhs, std::nullptr_t) const {
    return lhs.empty();
  }

  result_type operator()(std::nullptr_t, std::string_view rhs) const {
    return rhs.empty();
  }

  template <class T>
  result_type operator()(const T &lhs, const T &rhs) const {
    return rhs <= lhs;
  }
};

template <>
struct operator_impl<add> : arithmetic_operator {
  using arithmetic_operator::result_type;

  result_type operator()(std::nullptr_t, std::nullptr_t) const {
    return to_expr(nullptr);
  }

  template <class T>
  result_type operator()(const T &lhs, const T &rhs) const {
    return to_expr(lhs + rhs);
  }
};

template <>
struct operator_impl<subtract> : arithmetic_operator {
  using arithmetic_operator::result_type;

  result_type operator()(std::nullptr_t, std::nullptr_t) const {
    return to_expr(nullptr);
  }

  template <class T>
  result_type operator()(const T &lhs, const T &rhs) const {
    return to_expr(lhs - rhs);
  }
};

template <>
struct operator_impl<multiply> : arithmetic_operator {
  using arithmetic_operator::result_type;

  result_type operator()(std::nullptr_t, std::nullptr_t) const {
    return to_expr(nullptr);
  }

  template <class T>
  result_type operator()(const T &lhs, const T &rhs) const {
    return to_expr(lhs * rhs);
  }
};

template <>
struct operator_impl<divide> : arithmetic_operator {
  using arithmetic_operator::result_type;

  result_type operator()(std::nullptr_t, std::nullptr_t) const {
    return to_expr(nullptr);
  }

  result_type operator()(double lhs, double rhs) const {
    double res = lhs / rhs;

    // if (isInteger(res)) return toInt(res);
    return to_expr(res);
  }

  template <class Int_t>
  result_type operator()(Int_t lhs, Int_t rhs) const {
    if (lhs % rhs) return (*this)(double(lhs), double(rhs));

    return to_expr(lhs / rhs);
  }
};

template <>
struct operator_impl<modulo> : integer_arithmetic_operator {
  using integer_arithmetic_operator::result_type;

  std::nullptr_t operator()(std::nullptr_t, std::nullptr_t) const {
    return nullptr;
  }

  template <class T>
  result_type operator()(const T &lhs, const T &rhs) const {
    if (rhs == 0) return to_expr(nullptr);

    return to_expr(lhs % rhs);
  }
};

template <>
struct operator_impl<min> : arithmetic_operator {
  using arithmetic_operator::result_type;

  result_type operator()(const std::nullptr_t, std::nullptr_t) const {
    return nullptr;
  }

  template <class T>
  result_type operator()(const T &lhs, const T &rhs) const {
    return to_expr(std::min(lhs, rhs));
  }
};

template <>
struct operator_impl<max> : arithmetic_operator {
  using arithmetic_operator::result_type;

  result_type operator()(const std::nullptr_t, std::nullptr_t) const {
    return nullptr;
  }

  template <class T>
  result_type operator()(const T &lhs, const T &rhs) const {
    return to_expr(std::max(lhs, rhs));
  }
};

template <>
struct operator_impl<logical_not> {
  using result_type = bool;

  result_type operator()(expr &val) const { return falsy(val); }
};

template <>
struct operator_impl<logical_not_not> {
  using result_type = bool;

  result_type operator()(expr &val) const { return truthy(val); }
};

template <>
struct operator_impl<cat> : string_operator_non_destructive {
  using string_operator_non_destructive::result_type;

  explicit
  operator_impl(string_table& strtab)
  : string_operator_non_destructive(), strings(strtab)
  {}

  result_type operator()(std::string_view lhs, std::string_view rhs) const {
    std::string tmp;

    tmp.reserve(lhs.size() + rhs.size());

    tmp.append(lhs.begin(), lhs.end());
    tmp.append(rhs.begin(), rhs.end());

    return to_expr(strings.safe_string(std::move(tmp)));
  }

  string_table& strings;
};

/// implements the string mode of the membership operator
///    the mode testing element in an array is implemented
///    in the evaluator::visit function.
template <>
struct operator_impl<membership> : string_operator_non_destructive {
  using string_operator_non_destructive::result_type;

  result_type operator()(std::string_view lhs, std::string_view rhs) const {
    const bool res = (rhs.find(lhs) != json::string::npos);

    return to_expr(res);
  }
};

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
template <>
struct operator_impl<regex_match>
    : string_operator_non_destructive  // \todo the conversion rules differ
{
  using string_operator_non_destructive::result_type;

  result_type operator()(std::string_view lhs, std::string_view rhs) const {
    std::regex rgx(lhs.c_str(), lhs.size());

    return to_expr(std::regex_search(rhs.begin(), rhs.end(), rgx));
  }
};
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */

template <>
struct operator_impl<merge> : array_operator {
  using array_operator::result_type;

  result_type operator()(const array *ll, const array *rr) const {
    // note, to use the lhs directly, it would need to be released
    //   from its any_expr
    array *lhs = const_cast<array *>(ll);
    array *rhs = const_cast<array *>(rr);
    array &res = deref(new array);

    {
      oper::container_type &opers = res.operands();

      opers.swap(lhs->operands());

      oper::container_type &ropers = rhs->operands();

      auto beg = std::make_move_iterator(ropers.begin());
      auto lim = std::make_move_iterator(ropers.end());

      opers.insert(opers.end(), beg, lim);
    }

    return any_expr(&res);
  }
};

struct evaluator : forwarding_visitor {
  evaluator(variable_accessor varAccess, std::ostream &out)
      : vars(std::move(varAccess)), logger(out), calcres(nullptr), strings() {}

  void visit(const equal &) final;
  void visit(const strict_equal &) final;
  void visit(const not_equal &) final;
  void visit(const strict_not_equal &) final;
  void visit(const less &) final;
  void visit(const greater &) final;
  void visit(const less_or_equal &) final;
  void visit(const greater_or_equal &) final;
  void visit(const logical_and &) final;
  void visit(const logical_or &) final;
  void visit(const logical_not &) final;
  void visit(const logical_not_not &) final;
  void visit(const add &) final;
  void visit(const subtract &) final;
  void visit(const multiply &) final;
  void visit(const divide &) final;
  void visit(const modulo &) final;
  void visit(const min &) final;
  void visit(const max &) final;
  void visit(const array &) final;
  void visit(const map &) final;
  void visit(const reduce &) final;
  void visit(const filter &) final;
  void visit(const all &) final;
  void visit(const none &) final;
  void visit(const some &) final;
  void visit(const merge &) final;
  void visit(const cat &) final;
  void visit(const substr &) final;
  void visit(const membership &) final;
  void visit(const var &) final;
  void visit(const missing &) final;
  void visit(const missing_some &) final;
  void visit(const log &) final;

  void visit(const if_expr &) final;

  void visit(const null_value &n) final;
  void visit(const bool_value &n) final;
  void visit(const int_value &n) final;
  void visit(const unsigned_int_value &n) final;
  void visit(const real_value &n) final;
  void visit(const string_value &n) final;

  void visit(const error &n) final;

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
  void visit(const regex_match &n) final;
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */

  any_expr eval(const expr &n);

 private:
  variable_accessor vars;
  std::ostream &logger;
  any_expr calcres;
  string_table strings;

  evaluator(const evaluator &) = delete;
  evaluator(evaluator &&) = delete;
  evaluator &operator=(const evaluator &) = delete;
  evaluator &operator=(evaluator &&) = delete;

  //
  // opers

  /// implements relop : [1, 2, 3, whatever] as 1 relop 2 relop 3
  template <class binary_predicate_t>
  void eval_pair_short_circuit(const oper &n, binary_predicate_t pred);

  /// returns the first expression in [ e1, e2, e3 ] that evaluates to
  /// val,
  ///   or the last expression otherwise
  void eval_short_circuit(const oper &n, bool val);

  /// reduction operation on all elements
  template <class binary_op_t>
  void reduce_sequence(const oper &n, binary_op_t op);

  /// computes unary operation on n[0]
  template <class UnaryOperator>
  void unary(const oper &n, UnaryOperator calc);

  /// binary operation on all elements (invents an element if none is present)
  template <class binary_op_t>
  void binary(const oper &n, binary_op_t binop);

  /// evaluates and unpacks n[argpos] to a fundamental value_base
  template <class value_t>
  value_t unpack_optional_arg(const oper &n, int argpos,
                              const value_t &defaultVal);

  /// auxiliary missing method
  std::size_t missing_aux(array &elems);

  template <class ValueNode>
  void _value(const ValueNode &val) {
    calcres = to_expr(val.value());
  }
};

struct sequence_function {
  sequence_function(const expr &e, std::ostream &logstream)
      : exp(e), logger(logstream) {}

  any_expr operator()(any_expr &&elem) const {
    any_expr *elptr = &elem;  // workaround, b/c unique_ptr cannot be captured

    evaluator sub{[elptr](value_variant keyval, int) -> any_expr {
                    if (std::string_view* pkey = std::get_if<std::string_view>(&keyval)) {
                      if (pkey->size() == 0)
                        return clone_expr(*elptr);

                      try {
                        object_value &o = down_cast<object_value>(**elptr);

                        if (auto pos = o.find(*pkey); pos != o.end())
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
  const expr &exp;
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

  // for compatibility reasons, the first argument is passed in as
  // templated && ref.
  //   g++ -std=c++17 passes in a & ref, while g++ -std=c++20 passes
  //   in a &&.
  // the templated && together with reference collapsing make the code portable
  // across standard versions.
  // \note const ValueExprT& would also work, but currently the
  //       visitor calls in clone_expr require a non-const reference.
  template <class ValueExprT>
  any_expr operator()(ValueExprT &&accu, any_expr elem) const {
    any_expr *acptr = &accu;  // workaround, b/c unique_ptr cannot be captured
    any_expr *elptr = &elem;  // workaround, b/c unique_ptr cannot be captured

    evaluator sub{[acptr, elptr](value_variant keyval, int) -> any_expr {
                    if (const std::string_view *pkey = std::get_if<std::string_view>(&keyval)) {
                      if (*pkey == "current") return clone_expr(*elptr);

                      if (*pkey == "accumulator") return clone_expr(*acptr);
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
value_t evaluator::unpack_optional_arg(const oper &n, int argpos,
                                       const value_t &defaultVal) {
  if (std::size_t(argpos) >= n.size()) {
    CXX_UNLIKELY;
    return defaultVal;
  }

  return unpack_value<value_t>(*eval(n.operand(argpos)), strings);
}

template <class unary_predicate_t>
void evaluator::unary(const oper &n, unary_predicate_t pred) {
  CXX_MAYBE_UNUSED
  const int num = n.num_evaluated_operands();
  assert(num == 1);

  const bool res = pred(*eval(n.operand(0)));

  calcres = to_expr(res);
}

template <class binary_op_t>
void evaluator::binary(const oper &n, binary_op_t binop) {
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
void evaluator::reduce_sequence(const oper &n, binary_op_t op) {
  const int num = n.num_evaluated_operands();
  assert(num >= 1);

  int idx = -1;
  any_expr res = eval(n.operand(++idx));

  res = convert(std::move(res), strings, op);

  while (idx != (num - 1)) {
    any_expr rhs = eval(n.operand(++idx));

    rhs = convert(std::move(rhs), strings, op);
    res = compute(res, rhs, op);
  }

  calcres = std::move(res);
}

template <class binary_predicate_t>
void evaluator::eval_pair_short_circuit(const oper &n,
                                        binary_predicate_t pred) {
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

void evaluator::eval_short_circuit(const oper &n, bool val) {
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

any_expr evaluator::eval(const expr &n) {
  any_expr res;

  n.accept(*this);
  res.swap(calcres);

  return res;
}

void evaluator::visit(const equal &n) {
  eval_pair_short_circuit(n, operator_impl<equal>{});
}

void evaluator::visit(const strict_equal &n) {
  eval_pair_short_circuit(n, operator_impl<strict_equal>{});
}

void evaluator::visit(const not_equal &n) {
  eval_pair_short_circuit(n, operator_impl<not_equal>{});
}

void evaluator::visit(const strict_not_equal &n) {
  eval_pair_short_circuit(n, operator_impl<strict_not_equal>{});
}

void evaluator::visit(const less &n) {
  eval_pair_short_circuit(n, operator_impl<less>{});
}

void evaluator::visit(const greater &n) {
  eval_pair_short_circuit(n, operator_impl<greater>{});
}

void evaluator::visit(const less_or_equal &n) {
  eval_pair_short_circuit(n, operator_impl<less_or_equal>{});
}

void evaluator::visit(const greater_or_equal &n) {
  eval_pair_short_circuit(n, operator_impl<greater_or_equal>{});
}

void evaluator::visit(const logical_and &n) { eval_short_circuit(n, false); }

void evaluator::visit(const logical_or &n) { eval_short_circuit(n, true); }

void evaluator::visit(const logical_not &n) {
  unary(n, operator_impl<logical_not>{});
}

void evaluator::visit(const logical_not_not &n) {
  unary(n, operator_impl<logical_not_not>{});
}

void evaluator::visit(const add &n) {
  reduce_sequence(n, operator_impl<add>{});
}

void evaluator::visit(const subtract &n) {
  binary(n, operator_impl<subtract>{});
}

void evaluator::visit(const multiply &n) {
  reduce_sequence(n, operator_impl<multiply>{});
}

void evaluator::visit(const divide &n) { binary(n, operator_impl<divide>{}); }

void evaluator::visit(const modulo &n) { binary(n, operator_impl<modulo>{}); }

void evaluator::visit(const min &n) {
  reduce_sequence(n, operator_impl<min>{});
}

void evaluator::visit(const max &n) {
  reduce_sequence(n, operator_impl<max>{});
}

void evaluator::visit(const cat &n) {
  reduce_sequence(n, operator_impl<cat>{strings});
}

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
void evaluator::visit(const regex_match &n) {
  binary(n, operator_impl<regex_match>{});
}
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */

void evaluator::visit(const membership &n) {
  assert(n.num_evaluated_operands() >= 1);

  // lhs in rhs - rhs is a possibly large set.
  any_expr lhs = eval(n.operand(0));
  any_expr rhs = eval(n.operand(1));

  // std::cout << "in membership: lhs = " << lhs << ", rhs = " << rhs <<
  // std::endl;

  auto array_op = [&lhs](array &arrop) -> any_expr {
    auto beg = arrop.begin();
    auto lim = arrop.end();
    auto isEqual = [&lhs](any_expr &rhs) -> bool {
      return compute(lhs, rhs, operator_impl<strict_equal>{});
    };

    return to_expr(std::find_if(beg, lim, isEqual) != lim);
  };

  auto string_op = [&lhs, &rhs]() -> any_expr {
    try {
      return compute(lhs, rhs, operator_impl<membership>{});
    } catch (const type_error &) {
    }

    return to_expr(false);
  };

  calcres = with_type<array>(rhs.get(), array_op, string_op);
}

void evaluator::visit(const substr &n) {
  assert(n.num_evaluated_operands() >= 1);

  std::string_view str = unpack_value<std::string_view>(*eval(n.operand(0)), strings);
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

  calcres = to_expr(str.substr(ofs, cnt));
}

void evaluator::visit(const array &n) {
  oper::container_type elems;
  evaluator *self = this;

  // \todo consider making arrays lazy
  std::transform(
      n.begin(), n.end(), std::back_inserter(elems),
      [self](const any_expr &exp) -> any_expr { return self->eval(*exp); });

  array &res = deref(new array);

  res.set_operands(std::move(elems));

  calcres = any_expr(&res);
}

void evaluator::visit(const merge &n) {
  reduce_sequence(n, operator_impl<merge>{});
}

void evaluator::visit(const reduce &n) {
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

  auto mkInvalid = []() -> any_expr { return nullptr; };

  calcres = with_type<array>(arr.get(), op, mkInvalid);
}

void evaluator::visit(const map &n) {
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

void evaluator::visit(const filter &n) {
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

void evaluator::visit(const all &n) {
  any_expr arr = eval(n.operand(0));
  array &elems = down_cast<array>(*arr);  // evaluated elements
  expr &expr = n.operand(1);
  const bool res = std::all_of(std::make_move_iterator(elems.begin()),
                               std::make_move_iterator(elems.end()),
                               sequence_predicate{expr, logger});

  calcres = to_expr(res);
}

void evaluator::visit(const none &n) {
  any_expr arr = eval(n.operand(0));
  array &elems = down_cast<array>(*arr);  // evaluated elements
  expr &expr = n.operand(1);
  const bool res = std::none_of(std::make_move_iterator(elems.begin()),
                                std::make_move_iterator(elems.end()),
                                sequence_predicate{expr, logger});

  calcres = to_expr(res);
}

void evaluator::visit(const some &n) {
  any_expr arr = eval(n.operand(0));
  array &elems = down_cast<array>(*arr);  // evaluated elements
  expr &expr = n.operand(1);
  const bool res = std::any_of(std::make_move_iterator(elems.begin()),
                               std::make_move_iterator(elems.end()),
                               sequence_predicate{expr, logger});

  calcres = to_expr(res);
}

void evaluator::visit(const error &) { unsupported(); }

void evaluator::visit(const var &n) {
  assert(n.num_evaluated_operands() >= 1);

  any_expr elm = eval(n.operand(0));
  value_base &val = down_cast<value_base>(*elm);

  try {
    calcres = vars(val.to_variant(), n.num());
  } catch (const variable_resolution_error &) {
    calcres = (n.num_evaluated_operands() > 1) ? eval(n.operand(1))
                                               : to_expr(nullptr);
  }
}

std::size_t evaluator::missing_aux(array &elems) {
  auto avail = [calc = this](const any_expr &v) -> bool {
    try {
      const value_base &val = down_cast<value_base>(*v);
      any_expr res = calc->vars(val.to_variant(), COMPUTED_VARIABLE_NAME);

      // value-missing := res == 0 || *res is a null_value
      // return !value-missing
      return res && (may_down_cast<null_value>(*res) == nullptr);
    } catch (const jsonlogic::variable_resolution_error &) {
      return false;
    }
  };

  array::iterator beg = elems.begin();
  array::iterator lim = elems.end();
  array::iterator pos = std::remove_if(beg, lim, avail);
  std::size_t res = std::distance(pos, lim);

  elems.operands().erase(pos, lim);
  return res;
}

void evaluator::visit(const missing &n) {
  any_expr arg = eval(n.operand(0));
  auto non_array_alt = [&arg, &n, calc = this]() -> array & {
    array &res = deref(new array);

    res.operands().emplace_back(std::move(arg));
    arg.reset(&res);

    std::transform(
        std::next(n.begin()), n.end(), std::back_inserter(res.operands()),
        [&calc](const any_expr &e) -> any_expr { return calc->eval(*e); });

    return res;
  };

  array &elems = with_type<array>(
      arg.get(),
      [](array &arr) -> array & { return arr; },  // ignore other args
      non_array_alt);

  missing_aux(elems);
  calcres = std::move(arg);
}

void evaluator::visit(const missing_some &n) {
  const std::uint64_t minreq = unpack_value<std::uint64_t>(eval(n.operand(0)), strings);
  any_expr arr = eval(n.operand(1));
  array &elems = down_cast<array>(*arr);  // evaluated elements
  std::size_t avail = missing_aux(elems);

  if (avail >= minreq) elems.operands().clear();

  calcres = std::move(arr);
}

void evaluator::visit(const if_expr &n) {
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

void evaluator::visit(const log &n) {
  assert(n.num_evaluated_operands() == 1);

  calcres = eval(n.operand(0));

  logger << calcres << std::endl;
}

void evaluator::visit(const null_value &n) { _value(n); }
void evaluator::visit(const bool_value &n) { _value(n); }
void evaluator::visit(const int_value &n) { _value(n); }
void evaluator::visit(const unsigned_int_value &n) { _value(n); }
void evaluator::visit(const real_value &n) { _value(n); }
void evaluator::visit(const string_value &n) { _value(n); }

any_expr eval_path(std::string_view path, const json::object *obj) {
  if (obj) {
    if (auto pos = obj->find(path); pos != obj->end())
      return jsonlogic::to_expr(pos->value());

    if (std::size_t pos = path.find('.'); pos != json::string::npos) {
      std::string_view selector = path.substr(0, pos);
      std::string_view suffix   = path.substr(pos + 1);

      return eval_path(suffix, obj->at(selector).if_object());
    }
  }

  throw variable_resolution_error{"in logic.cc::eval_path."};
}

template <class IntT>
any_expr eval_index(IntT idx, const json::array &arr) {
  return jsonlogic::to_expr(arr[idx]);
}

}  // namespace

any_expr apply(expr &exp, const variable_accessor &vars) {
  evaluator ev{vars, std::cerr};

  return ev.eval(exp);
}

any_expr apply(const any_expr &exp, const variable_accessor &vars) {
  assert(exp.get());
  return apply(*exp, vars);
}

any_expr apply(const any_expr &exp) {
  return jsonlogic::apply(exp, [](value_variant, int) -> any_expr {
    throw std::logic_error{"variable accessor not available"};
  });
}

variable_accessor data_accessor(json::value data) {
  return [data = std::move(data)](value_variant keyval, int) -> any_expr {
    if (const std::string_view *ppath = std::get_if<std::string_view>(&keyval)) {
      //~ std::cerr << *ppath << std::endl;
      return ppath->size() ? eval_path(*ppath, data.if_object())
                           : to_expr(data);
    }

    if (const std::int64_t *pidx = std::get_if<std::int64_t>(&keyval))
      return eval_index(*pidx, data.as_array());

    if (const std::uint64_t *puidx = std::get_if<std::uint64_t>(&keyval))
      return eval_index(*puidx, data.as_array());

    throw std::logic_error{"jsonlogic - unsupported var access"};
  };
}

any_expr apply(json::value rule, json::value data) {
  logic_rule logic(create_logic(rule));

  return jsonlogic::apply(logic.syntax_tree(), data_accessor(std::move(data)));
}

variable_accessor data_accessor(std::vector<value_variant> vars) {
  return [vars = std::move(vars)](value_variant, int idx) -> any_expr {
    if ((idx >= 0) && (std::size_t(idx) < vars.size())) {
      CXX_LIKELY;
      return to_expr(vars[idx]);
    }

    throw std::logic_error{"unable to access (computed) variable"};
  };
}

any_expr apply(const any_expr &exp, std::vector<value_variant> vars) {
  return jsonlogic::apply(exp, data_accessor(std::move(vars)));
}


std::ostream& operator<<(std::ostream& os, const value_variant& val)
{
    // \todo this is copied, and should be made more broadly available..
  enum { mono_variant = 0,
         null_variant = 1,
         bool_variant = 2,
         int_variant  = 3,
         uint_variant = 4,
         real_variant = 5,
         strv_variant = 6,
         json_variant = 7
       };


  switch (val.index()) {
    case mono_variant: {
      os << "<mono/unavail>";
      break;
    }

    case null_variant: {
      os << "null";
      break;
    }

    case bool_variant: {
      os << (std::get<bool>(val) ? "true" : "false");
      break;
    }

    case int_variant: {
      os << std::get<std::int64_t>(val);
      break;
    }

    case uint_variant: {
      os << std::get<std::uint64_t>(val);
      break;
    }

    case real_variant: {
      // print the double as json would print it so that strings
      //   are comparable.
      os << json::value(std::get<double>(val));
      break;
    }

    case strv_variant: {
      os << '"' << std::get<std::string_view>(val) << '"';
      break;
    }

    case json_variant: {
      os << std::get<boost::json::value>(val);
      break;
    }

    default:
      // did val hold a valid value?
      os << "<error>";
  }

  return os;
}

namespace {

struct value_printer : forwarding_visitor {
  explicit value_printer(std::ostream &stream) : os(stream) {}

  void prn(value_variant val) { os << val; }

  void visit(const expr &) final { unsupported(); }

  void visit(const value_base &n) final { prn(n.to_variant()); }

  void visit(const array &n) final {
    bool first = true;

    os << "[";
    for (const any_expr &el : n) {
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

}  // namespace

std::ostream &operator<<(std::ostream &os, any_expr &n) {
  value_printer prn{os};

  deref(n).accept(prn);
  return os;
}

expr &oper::operand(int n) const { return deref(this->at(n).get()); }

//
// json_logic_rule

any_expr const &logic_rule::syntax_tree() const { return std::get<0>(*this); }

std::vector<boost::json::string> const &logic_rule::variable_names() const {
  return std::get<1>(*this);
}

bool logic_rule::has_computed_variable_names() const {
  return std::get<2>(*this);
}

any_expr logic_rule::apply() const { return jsonlogic::apply(syntax_tree()); }

any_expr logic_rule::apply(const variable_accessor &var_accessor) const {
  return jsonlogic::apply(syntax_tree(), var_accessor);
}

any_expr logic_rule::apply(std::vector<value_variant> vars) const {
  return jsonlogic::apply(syntax_tree(), std::move(vars));
}

}  // namespace jsonlogic

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

  template <class OperatorNode>
  inline void _visit(OperatorNode &n) {
    traverseChildren(*this, n);
    sub.visit(n);
  }

  template <class ValueNode>
  inline void _value(ValueNode &n) {
    sub.visit(n);
  }
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
}  // namespace

/// AST traversal function that calls v's visit methods in post-fix
/// order
void traverseInSAttributeOrder(expr &e, visitor &vis);

void traverseInSAttributeOrder(expr &e, visitor &vis) {
  SAttributeTraversal trav{vis};

  e.accept(trav);
}
#endif /* SUPPLEMENTAL */
