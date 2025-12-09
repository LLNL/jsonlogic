/// implements jsonlogic functions

#include <jsonlogic/logic.hpp>

// standard headers
#include <algorithm>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <numeric>
#include <string>
#include <unordered_map>
#include <charconv>
#include <span>
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
[[maybe_unused]] constexpr bool DEBUG_OUTPUT = false;
constexpr int COMPUTED_VARIABLE_NAME = -1;

#if DEBUGGING_SUPPORT
struct DebugGuard
{
  DebugGuard(const char* m) : msg(m) { std::cerr << "enter " << msg << std::endl; }
  ~DebugGuard() { std::cerr << "exit  " << msg << std::endl; }

  const char* msg = "none";
};
#endif /* DEBUGGING_SUPPORT */

}  // namespace


namespace jsonlogic {



namespace {
CXX_NORETURN
inline
void unsupported() {
  throw std::logic_error("functionality not yet implemented");
}

CXX_NORETURN
inline
void throw_type_error() { throw type_error("typing error"); }

CXX_NORETURN
inline
void implementation_error() { throw std::logic_error("implementation error"); }

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
} // namespace


namespace json = boost::json;

using any_value = value_variant;

enum /* anonymous */ {
  mono_variant = 0,
  null_variant = 1,
  bool_variant = 2,
  sint_variant = 3,
  uint_variant = 4,
  real_variant = 5,
  strv_variant = 6,
  sequ_variant = 7,
  //~ json_variant = 8
};

static_assert(std::variant_size_v<value_variant_base> == 8);
static_assert(std::is_same_v<std::monostate,
                             std::variant_alternative_t<mono_variant, value_variant_base> >);
static_assert(std::is_same_v<std::nullptr_t,
                             std::variant_alternative_t<null_variant, value_variant_base> >);
static_assert(std::is_same_v<bool,
                             std::variant_alternative_t<bool_variant, value_variant_base> >);
static_assert(std::is_same_v<std::int64_t,
                             std::variant_alternative_t<sint_variant, value_variant_base> >);
static_assert(std::is_same_v<std::uint64_t,
                             std::variant_alternative_t<uint_variant, value_variant_base> >);
static_assert(std::is_same_v<double,
                             std::variant_alternative_t<real_variant, value_variant_base> >);
static_assert(std::is_same_v<managed_string_view,
                             std::variant_alternative_t<strv_variant, value_variant_base> >);
static_assert(std::is_same_v<array_value const*,
                             std::variant_alternative_t<sequ_variant, value_variant_base> >);

bool null_equivalent(value_variant val)
{
  bool res = false;

  switch (val.index())
  {
    case mono_variant:
    case null_variant:
      res = true;
      break;

    default:
      CXX_LIKELY;
  }

  return res;
}


template <class T>
bool
stricteq(const value_variant& lhs, const value_variant& rhs)
try
{
  return std::get<T>(lhs) == std::get<T>(rhs);
}
catch (...)
{
  return false;
}

// using variant_span = std::span<value_variant>;

using variant_span_base = std::tuple<std::vector<value_variant>::const_iterator, std::vector<value_variant>::const_iterator>;

struct variant_span : variant_span_base
{
  using base = variant_span_base;
  using base::base;

  std::vector<value_variant>::const_iterator begin() const { return std::get<0>(*this); }
  std::vector<value_variant>::const_iterator end()   const { return std::get<1>(*this); }
  std::size_t                                size()  const { return std::distance(begin(), end()); }
};


managed_string_view
element_range(const value_variant& val, const std::string_view&)
{
  return std::get<managed_string_view>(val);
}

variant_span
element_range(const std::vector<value_variant>& vec)
{
  return { vec.begin(), vec.end() };
}

variant_span
element_range(const array_value* arr)
{
  return element_range(deref(arr).value());
}

variant_span
element_range(const value_variant& val, const array_value*)
{
  return element_range(std::get<array_value const*>(val));
}

template <class RangeT>
bool
stricteq_range(const value_variant& lhs, const value_variant& rhs)
{
  if (rhs.index() != lhs.index())
    return false;

  auto ll = element_range(lhs, RangeT{});
  auto rr = element_range(rhs, RangeT{});

  if (ll.size() != rr.size())
    return false;

  auto const lhslim = ll.end();

  return std::mismatch(ll.begin(), lhslim, rr.begin()).first == lhslim;
}

bool inteq(std::int64_t x, std::uint64_t y)
{
  bool const y_within_int_range = (y <= std::uint64_t(std::numeric_limits<std::int64_t>::max()));

  return y_within_int_range & (x == std::int64_t(y));
}

template <class T>
bool
inteq(const value_variant& lhs, const value_variant& rhs)
{
  if (rhs.index() == lhs.index())
  {
    CXX_LIKELY;
    return std::get<T>(lhs) == std::get<T>(rhs);
  }

  if (rhs.index() == uint_variant)
    return inteq(std::get<std::int64_t>(lhs), std::get<std::uint64_t>(rhs));

  if (rhs.index() == sint_variant)
    return inteq(std::get<std::int64_t>(rhs), std::get<std::uint64_t>(lhs));

  return false;
}


bool operator==(const value_variant& lhs, const value_variant& rhs)
{
  bool       res = false;

  switch (lhs.index())
  {
    case mono_variant:
    case null_variant:
      res = lhs.index() == rhs.index();
      break;

    case bool_variant:
      res = stricteq<bool>(lhs, rhs);
      break;

    case sint_variant:
      res = inteq<std::int64_t>(lhs, rhs);
      break;

    case uint_variant:
      res = inteq<std::uint64_t>(lhs, rhs);
      break;

    case real_variant:
      res = stricteq<double>(lhs, rhs);
      break;

    case strv_variant:
      res = stricteq_range<std::string_view>(lhs, rhs);
      break;

    case sequ_variant:
      res = stricteq_range<array_value const*>(lhs, rhs);
      break;

    default:
      throw_type_error();
  }

  return res;
}

namespace
{
  array_value& mk_array_value(std::vector<any_value> elems = {});
  //~ array_view& mk_array_view(const array_value& elems);

  void delete_array(value_variant_base& val)
  {
    if (val.index() == sequ_variant)
    {
      CXX_UNLIKELY;
      delete std::get<array_value const*>(val);
    }
  }

  void copy_array(value_variant_base& val)
  {
    if (val.index() == sequ_variant)
    {
      CXX_UNLIKELY;
      val = std::get<array_value const*>(val)->copy();
    }
  }

  void invalidate_array(value_variant_base& val)
  {
    if (val.index() == sequ_variant)
    {
      CXX_UNLIKELY;
      val = std::monostate{};
    }
  }
}


// implement variants destructor to possibly free memory if needed.
value_variant::~value_variant()
{
  delete_array(*this);
}

value_variant::value_variant(const value_variant& other)
: base(other)
{
  assert(this != &other);
  copy_array(*this);
}

value_variant::value_variant(value_variant&& other)
: base(std::move(other))
{
  assert(this != &other);
  invalidate_array(other);
}

value_variant& value_variant::operator=(const value_variant& other)
{
  if (&other != this)
  {
    delete_array(*this);
    static_cast<value_variant_base&>(*this) = other;
    copy_array(*this);
  }

  return *this;
}

value_variant& value_variant::operator=(value_variant&& other)
{
  if (&other != this)
  {
    delete_array(*this);
    static_cast<value_variant_base&>(*this) = std::move(other);
    invalidate_array(other);
  }

  return *this;
}




//
// foundation classes
// \{

array::array(array &&other) : oper() {
  set_operands(std::move(other).move_operands());
}

array&
array::operator=(array &&other) {
  set_operands(std::move(other).move_operands());
  return *this;
}


array_value::container_type const&
array_value::value() const
{
  assert(vec.get());

  return *vec;
}

const array_value*
array_value::copy() const
{
  return new array_value(*this);
}

value_variant
array_value::to_variant() const
{
  return this;
}



#if 0


array_value::container_type const&
array_view::value() const
{
  return arr.value();
}

const array_view*
array_view::copy() const
{
  return &mk_array_view(arr);
}
#endif

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
void array_value::accept(visitor &v) const { v.visit(*this); }
//~ void array_view::accept(visitor &v) const { v.visit(*this); }

void error::accept(visitor &v) const { v.visit(*this); }

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
void regex_match::accept(visitor &v) const { v.visit(*this); }
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */

// to_variant conversion
template <class T>
value_variant value_generic<T>::to_variant() const {
  return value();
}

value_variant null_value::to_variant() const { return value(); }

// num_evaluated_operands implementations
int oper::num_evaluated_operands() const { return static_cast<int>(size()); }

template <int MaxArity> int oper_n<MaxArity>::num_evaluated_operands() const {
  return std::min(MaxArity, oper::num_evaluated_operands());
}

struct forwarding_visitor : visitor {
  virtual ~forwarding_visitor() = default;

  void visit(const expr &) override {} // error
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
  void visit(const array &n) override { visit(up_cast<oper>(n)); }

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
  void visit(const array_value &n) override { visit(up_cast<value_base>(n)); }

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
  std::vector<std::string_view> to_vector() const;

  /// accessors for withComputedNames
  /// \{
  bool hasComputedVariables() const { return withComputedNames; }
  void set_computed_variables(bool b) { withComputedNames = b; }
  /// \}

 private:
  using container_type = std::map<std::string_view, int>;

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
      set_computed_variables(true);
    } else if (str.value() != "") {
      auto [pos, success] = mapping.emplace(str.value(), mapping.size());

      var.num(pos->second);
    }
    else
    {
      // do nothing for free variables in "lambdas"
    }
  } catch (const type_error &) {
    set_computed_variables(true);
  }
}

std::vector<std::string_view> variable_map::to_vector() const {
  std::vector<std::string_view> res;

  res.resize(mapping.size());

  for (const container_type::value_type &el : mapping)
    res.at(el.second) = el.first;

  return res;
}

/// translates all children
/// \{
oper::container_type translate_children(const json::array &children, variable_map &);

oper::container_type translate_children(const json::value &n, variable_map &);
/// \}

array& mk_array()     { return deref(new array); }

array_value& mk_array_value(std::vector<any_value> elems)
{
  return deref(new array_value{std::move(elems)});
}

//~ array_view& mk_array_view(const array_value& arr)
//~ {
  //~ return deref(new array_view{arr});
//~ }

template <class ExprT>
ExprT &mk_operator_(const json::object &n, variable_map &m) {
  assert(n.size() == 1);

  ExprT &res = deref(new ExprT);

  res.set_operands(translate_children(n.begin()->value(), m));
  return res;
}

template <class ExprT>
expr &mk_operator(const json::object &n, variable_map &m) {
  return mk_operator_<ExprT>(n, m);
}

template <class ExprT>
expr &mk_missing(const json::object &n, variable_map &m) {
  // \todo * extract variables from array and only set_computed_variables when
  // needed.
  //       * check reference implementation when missing is null
  m.set_computed_variables(true);
  return mk_operator_<ExprT>(n, m);
}

expr &mk_variable(const json::object &n, variable_map &m) {
  var &v = mk_operator_<var>(n, m);

  m.insert(v);
  return v;
}

array &mk_array(const json::array &children, variable_map &m) {
  array &res = mk_array();

  res.set_operands(translate_children(children, m));
  return res;
}

template <class value_t>
value_t &mk_value(typename value_t::value_type n) {
  return deref(new value_t(std::move(n)));
}

null_value &mk_null_value() { return deref(new null_value); }

using dispatch_table =
    std::map<std::string_view, expr &(*)(const json::object &, variable_map &)>;

dispatch_table::const_iterator lookup(const dispatch_table &m,
                                      const json::object &op) {
  if (op.size() != 1) return m.end();

  const json::string& key = op.begin()->key();
  std::string_view    keyvw(&*key.begin(), key.size());

  return m.find(keyvw);
}

any_expr translate_internal(const json::value& n, variable_map &varmap) {
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
      const json::string& str = n.get_string();
      res = &mk_value<string_value>(managed_string_view(str.begin(), str.size()));
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
                                        variable_map &varmap) {
  oper::container_type res;

  res.reserve(children.size());

  for (const json::value &elem : children)
    res.emplace_back(translate_internal(elem, varmap));

  return res;
}

oper::container_type translate_children(const json::value &n, variable_map &varmap) {
  if (const json::array *arr = n.if_array()) {
    CXX_LIKELY;
    return translate_children(*arr, varmap);
  }

  oper::container_type res;

  res.emplace_back(translate_internal(n, varmap));
  return res;
}
} // namespace


logic_rule create_logic(const json::value& n) {
  variable_map varmap;
  any_expr node = translate_internal(n, varmap);
  bool const hasComputedVariables = varmap.hasComputedVariables();

  return logic_rule(std::make_unique<logic_data>(std::move(node), varmap.to_vector(), hasComputedVariables));
}



//
// to value_base conversion

any_value to_value(std::nullptr_t) { return nullptr; }
any_value to_value(bool val) { return val; }
any_value to_value(std::int64_t val) { return val; }
any_value to_value(std::uint64_t val) { return val; }
any_value to_value(double val) { return val; }
any_value to_value(managed_string_view val) { return val; }
// any_value to_value(const array_value& val) { return &mk_array_view(val); }

//~ no longer supported
//~ any_expr to_value(json::string val) {
  //~ return any_expr(new string_value(safe_string(val.begin(), val.end())));
//~ }

any_value to_value(const json::value &n); // \todo remove after moving to logic.hpp

/*
any_value to_value(const json::array &) {
  oper::container_type elems;
/ *** TODO
  std::transform(val.begin(), val.end(), std::back_inserter(elems),
                 [](const json::value &el) -> any_value { return to_value(el); });
  array &arr = mk_array();

  arr.set_operands(std::move(elems));
  return &arr;
}
*/

any_value to_value(const json::value &n) {
  any_value res;

  switch (n.kind()) {
    case json::kind::string: {
      const json::string& str = n.get_string(); // \todo this may be unsafe..
      res = to_value(managed_string_view(&*str.begin(), str.size()));
      break;
    }

    case json::kind::int64: {
      res = to_value(n.get_int64());
      break;
    }

    case json::kind::uint64: {
      res = to_value(n.get_uint64());
      break;
    }

    case json::kind::double_: {
      res = to_value(n.get_double());
      break;
    }

    case json::kind::bool_: {
      res = to_value(n.get_bool());
      break;
    }

    case json::kind::null: {
      res = to_value(nullptr);
      break;
    }

    case json::kind::array: {
      std::vector<value_variant> values;
      const json::array&         arr = n.get_array();

      for (const json::value& val : arr)
        values.emplace_back(to_value(val));

      res = &mk_array_value(std::move(values));
      break;
    }

    default:
      unsupported();
  }

  return res;
}


any_value to_value(const any_expr& n)
{
  return down_cast<value_base>(*n).to_variant();
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
[[maybe_unused]] inline std::int64_t to_concrete(std::int64_t v,
                                                 const std::int64_t &) {
  return v;
}
inline std::int64_t to_concrete(const managed_string_view& str, const std::int64_t &el) {
  return from_string(str, el);
}
inline std::int64_t to_concrete(double v, const std::int64_t &) {
  return static_cast<std::int64_t>(v);
}
inline std::int64_t to_concrete(bool v, const std::int64_t &) {
  return static_cast<std::int64_t>(v);
}
inline std::int64_t to_concrete(std::nullptr_t, const std::int64_t &) {
  return 0;
}

inline std::int64_t to_concrete(std::uint64_t v, const std::int64_t &) {
  if (v > std::uint64_t(std::numeric_limits<std::int64_t>::max())) {
    CXX_UNLIKELY;
    throw not_int64_error{};
  }

  return static_cast<int64_t>(v);
}
/// \}

/// conversion to uint64
/// \{
[[maybe_unused]] inline std::uint64_t to_concrete(std::uint64_t v,
                                                  const std::uint64_t &) {
  return v;
}
inline std::uint64_t to_concrete(const managed_string_view& str, const std::uint64_t &el) {
  return from_string(str, el);
}
inline std::uint64_t to_concrete(double v, const std::uint64_t &) {
  return static_cast<std::uint64_t>(v);
}
inline std::uint64_t to_concrete(bool v, const std::uint64_t &) {
  return static_cast<std::uint64_t>(v);
}
inline std::uint64_t to_concrete(std::nullptr_t, const std::uint64_t &) {
  return 0;
}

inline std::uint64_t to_concrete(std::int64_t v, const std::uint64_t &) {
  if (v < 0) {
    CXX_UNLIKELY;
    throw not_uint64_error{};
  }

  return static_cast<std::uint64_t>(v);
}
/// \}

/// conversion to double
/// \{
inline double to_concrete(const managed_string_view& str, const double &el) {
  return from_string(str, el);
}
inline double to_concrete(std::int64_t v, const double &) {
  return static_cast<double>(v);
}
inline double to_concrete(std::uint64_t v, const double &) {
  return static_cast<double>(v);
}
[[maybe_unused]] inline double to_concrete(double v, const double &) {
  return v;
}

inline double to_concrete(bool v, const double &) {
  return static_cast<double>(v);
}
[[maybe_unused]] inline double to_concrete(std::nullptr_t, const double &) {
  return 0;
}
/// \}




/// conversion to string
/// \{
template <class Val>
inline managed_string_view to_concrete(Val v, const std::string_view &) {
  return managed_string_view(std::to_string(v));
}
inline managed_string_view to_concrete(bool v, const std::string_view &) {
  static constexpr const char* bool_string[] = {"false", "true"};

  return managed_string_view(std::string_view(bool_string[v]));
}
[[maybe_unused]] inline managed_string_view to_concrete(const managed_string_view &s, const std::string_view &) {
  return s;
}
inline managed_string_view to_concrete(std::nullptr_t, const std::string_view &) {
  return managed_string_view(std::string_view("null"));
}
/// \}

/// conversion to boolean
///   implements truthy, falsy as described by https://jsonlogic.com/truthy.html
/// \{
inline bool to_concrete(bool v, const bool &) { return v; }
inline bool to_concrete(std::int64_t v, const bool &) {
  return static_cast<bool>(v);
}
inline bool to_concrete(std::uint64_t v, const bool &) {
  return static_cast<bool>(v);
}
inline bool to_concrete(double v, const bool &) { return static_cast<bool>(v); }
inline bool to_concrete(const managed_string_view &v, const bool &) {
  return v.size() != 0;
}
inline bool to_concrete(std::nullptr_t, const bool &) { return false; }

// \todo not sure if conversions from arrays to values should be
// supported like this
inline bool to_concrete(array_value const* arr, const bool &) {
  return arr->value().size();
}
/// \}

}


/*
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
*/


struct comparison_operator_base {
  enum : bool {
    defined_for_string = true,
    defined_for_real = true,
    defined_for_integer = true,
    defined_for_boolean = true,
    defined_for_null = true,
    defined_for_array = true
  };

  using result_type = bool;
};

template <class T> T to_calc_type(const T *val) { return *val; }

const managed_string_view& to_calc_type(const managed_string_view *val) {
  return *val;
}
/*
std::nullptr_t
to_calc_type(std::nullptr_t el)
{
  return el;
}
*/
variant_span to_calc_type(array_value const *n) { return element_range(n); }

/// \brief a strict equality operator operates on operands of the same
///        type. The operation on two different types returns false.
///        NO type coercion is performed.
struct strict_equality_operator : comparison_operator_base {
  std::tuple<bool, bool> coerce(const array *, const array *) const {
    // arrays are never equal
    // \todo arrays may be equal if they have the same reference
    //       i.e., if they originate from the same variable
    // return {true, lhs==rhs};
    return {true, false};
  }

  template <class LhsT, class RhsT>
  std::tuple<decltype(to_calc_type(std::declval<const LhsT *>())),
             decltype(to_calc_type(std::declval<const RhsT *>()))>
  coerce(const LhsT *lv, const RhsT *rv) const {
    return {to_calc_type(lv), to_calc_type(rv)};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    std::nullptr_t) const  {
    return {nullptr, nullptr};  // two null pointers are equal
  }

  template <class LhsT>
  std::tuple<decltype(to_calc_type(std::declval<const LhsT *>())),
             std::nullptr_t>
  coerce(const LhsT *lv, std::nullptr_t) const {
    return {to_calc_type(lv), nullptr};
  }

  template <class RhsT>
  std::tuple<std::nullptr_t,
             decltype(to_calc_type(std::declval<const RhsT *>()))>
  coerce(std::nullptr_t, const RhsT *rv) const {
    return {nullptr, to_calc_type(rv)};
  }
};

struct numeric_binary_operator_base {
  std::tuple<double, double> coerce(const double *lv, const double *rv) const {
    return {*lv, *rv};
  }

  std::tuple<double, double> coerce(const double *lv, const std::int64_t *rv) const {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<double, double> coerce(const double *lv, const std::uint64_t *rv) const {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<double, double> coerce(const std::int64_t *lv, const double *rv) const {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(const std::int64_t *lv,
                                                const std::int64_t *rv) const {
    return {*lv, *rv};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(const std::int64_t *lv,
                                                const std::uint64_t *rv) const {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<double, double> coerce(const std::uint64_t *lv, const double *rv) const {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(const std::uint64_t *lv,
                                                const std::int64_t *rv) const {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(const std::uint64_t *lv,
                                                  const std::uint64_t *rv) const {
    return {*lv, *rv};
  }
};

/// \brief an equality operator compares two values. if_expr the
///        values have a different type, type coercion is performed
///        on one of the operands.
struct relational_operator_base : numeric_binary_operator_base {
  using numeric_binary_operator_base::coerce;

  std::tuple<double, double> coerce(const double *lv, const managed_string_view* rv) const {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<double, double> coerce(const double *lv, const bool *rv) const {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(const std::int64_t *lv,
                                                const managed_string_view* rv) const {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(const std::int64_t *lv,
                                                const bool *rv) const {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(const std::uint64_t *lv,
                                                  const managed_string_view* rv) const {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(const std::uint64_t *lv,
                                                  const bool *rv) const {
    return {*lv, to_concrete(*rv, *lv)};
  }

  std::tuple<double, double> coerce(const managed_string_view* lv, const double *rv) const {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<double, double> coerce(const bool *lv, const double *rv) const {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(const managed_string_view* lv,
                                                const std::int64_t *rv) const {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::int64_t, std::int64_t> coerce(const bool *lv,
                                                const std::int64_t *rv) const {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(const managed_string_view* lv,
                                                  const std::uint64_t *rv) const {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(const bool *lv,
                                                  const std::uint64_t *rv) const {
    return {to_concrete(*lv, *rv), *rv};
  }

  std::tuple<bool, bool> coerce(const managed_string_view *, const bool *) const {
    // strings and boolean are never equal
    return {true, false};
  }

  std::tuple<bool, bool> coerce(const bool *, const managed_string_view *) const {
    // strings and boolean are never equal
    return {true, false};
  }

  std::tuple<const managed_string_view&, const managed_string_view&> coerce(
      const managed_string_view *lv, const managed_string_view *rv) const {
    return {*lv, *rv};
  }

  std::tuple<bool, bool> coerce(const bool *lv, const bool *rv) const {
    return {*lv, *rv};
  }
};

struct equality_operator : relational_operator_base, comparison_operator_base {
  using relational_operator_base::coerce;

  // due to special conversion rules, the coercion function may just produce
  //   the result instead of just unpacking and coercing values.

  std::tuple<bool, bool> coerce(const array_value *, const array_value *) const {
    return {true, false};  // arrays are never equal
  }

  template <class T>
  std::tuple<bool, bool> coerce(const T *lv, const array_value *rv) const {
    // an array may be compared to a value_base
    //   (1) *lv == arr[0], iff the array has exactly one element
    if (rv->value().size() == 1) throw unpacked_array_req{};

    //   (2) or if [] and *lv converts to false
    if (rv->value().size() > 1) return {false, true};

    const bool convToFalse = to_concrete(*lv, false) == false;

    return {convToFalse, true /* zero elements */};
  }

  template <class T>
  std::tuple<bool, bool> coerce(const array_value *lv, const T *rv) const {
    // see comments in coerce(T*,array*)
    if (lv->value().size() == 1) throw unpacked_array_req{};

    if (lv->value().size() > 1) return {false, true};

    const bool convToFalse = to_concrete(*rv, false) == false;

    return {true /* zero elements */, convToFalse};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    std::nullptr_t) const {
    return {nullptr, nullptr};  // two null pointers are equal
  }

  template <class T>
  std::tuple<bool, bool> coerce(const T *, std::nullptr_t) const {
    return {false, true};  // null pointer is only equal to itself
  }

  template <class T>
  std::tuple<bool, bool> coerce(std::nullptr_t, const T *) const {
    return {true, false};  // null pointer is only equal to itself
  }

  std::tuple<bool, bool> coerce(const array_value *, std::nullptr_t) const {
    return {true, false};  // null pointer is only equal to itself
  }

  std::tuple<bool, bool> coerce(std::nullptr_t, const array_value *) const {
    return {true, false};  // null pointer is only equal to itself
  }
};

struct relational_operator : relational_operator_base,
                             comparison_operator_base {
  using relational_operator_base::coerce;

  std::tuple<variant_span, variant_span>
  coerce(const array_value *lv, const array_value *rv) const {
    return {element_range(lv), element_range(rv)};
  }

  template <class T>
  std::tuple<bool, bool> coerce(const T *lv, array_value const* rv) const {
    // an array may be equal to another value_base if
    //   (1) *lv == arr[0], iff the array has exactly one element
    if (rv->value().size() == 1) throw unpacked_array_req{};

    //   (2) or if [] and *lv converts to false
    if (rv->value().size() > 1) return {false, true};

    const bool convToTrue = to_concrete(*lv, true) == true;

    return {convToTrue, false /* zero elements */};
  }

  template <class T>
  std::tuple<bool, bool> coerce(array_value const*lv, const T *rv) const {
    // see comments in coerce(T*,array*)
    if (lv->value().size() == 1) throw unpacked_array_req{};

    if (lv->value().size() > 1) return {false, true};

    const bool convToTrue = to_concrete(*rv, true) == true;

    return {false /* zero elements */, convToTrue};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    std::nullptr_t) const {
    return {nullptr, nullptr};  // two null pointers are equal
  }

  std::tuple<bool, bool> coerce(const bool *lv, std::nullptr_t) const {
    return {*lv, false};  // null pointer -> false
  }

  std::tuple<std::int64_t, std::int64_t> coerce(const std::int64_t *lv,
                                                std::nullptr_t) const {
    return {*lv, 0};  // null pointer -> 0
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(const std::uint64_t *lv,
                                                  std::nullptr_t) const {
    return {*lv, 0};  // null pointer -> 0
  }

  std::tuple<double, double> coerce(const double *lv, std::nullptr_t) const {
    return {*lv, 0};  // null pointer -> 0.0
  }

  std::tuple<const managed_string_view&, std::nullptr_t> coerce(const managed_string_view *lv,
                                                      std::nullptr_t) const {
    return {to_calc_type(lv), nullptr};  // requires special handling
  }

  std::tuple<bool, bool> coerce(std::nullptr_t, const bool *rv) const {
    return {false, *rv};  // null pointer -> false
  }

  std::tuple<std::int64_t, std::int64_t> coerce(std::nullptr_t,
                                                const std::int64_t *rv) const {
    return {0, *rv};  // null pointer -> 0
  }

  std::tuple<std::uint64_t, std::uint64_t> coerce(std::nullptr_t,
                                                  const std::uint64_t *rv) const {
    return {0, *rv};  // null pointer -> 0
  }

  std::tuple<double, double> coerce(std::nullptr_t, const double *rv) const {
    return {0, *rv};  // null pointer -> 0
  }

  std::tuple<std::nullptr_t, const managed_string_view&> coerce(std::nullptr_t,
                                                      const managed_string_view *rv) const {
    return {nullptr, to_calc_type(rv)};  // requires special handling
  }
};
// @}

// Arith
struct arithmetic_operator : numeric_binary_operator_base {
  enum : bool {
    defined_for_string = false,
    defined_for_real = true,
    defined_for_integer = true,
    defined_for_boolean = false,
    defined_for_null = true,
    defined_for_array = false
  };

  using result_type = any_value;

  using numeric_binary_operator_base::coerce;

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(const double *,
                                                    std::nullptr_t) const {
    return {nullptr, nullptr};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(const std::int64_t *,
                                                    std::nullptr_t) const {
    return {nullptr, nullptr};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(const std::uint64_t *,
                                                    std::nullptr_t) const {
    return {nullptr, nullptr};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    const double *) const {
    return {nullptr, nullptr};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    const std::int64_t *) const {
    return {nullptr, nullptr};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    const std::uint64_t *) const {
    return {nullptr, nullptr};
  }

  std::tuple<std::nullptr_t, std::nullptr_t> coerce(std::nullptr_t,
                                                    std::nullptr_t) const {
    return {nullptr, nullptr};
  }
};

struct integer_arithmetic_operator : arithmetic_operator {
  enum : bool {
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
  enum : bool {
    defined_for_string = true,
    defined_for_real = false,
    defined_for_integer = false,
    defined_for_boolean = false,
    defined_for_null = false,
    defined_for_array = false
  };

  using result_type = any_value;

  std::tuple<const managed_string_view&, const managed_string_view&> coerce(
      const managed_string_view *lv, const managed_string_view *rv) const {
    return {*lv, *rv};
  }
};

struct array_operator {
  enum : bool {
    defined_for_string = false,
    defined_for_real = false,
    defined_for_integer = false,
    defined_for_boolean = false,
    defined_for_null = false,
    defined_for_array = true
  };

  using result_type = array_value const*;

  std::tuple<variant_span, variant_span>
  coerce(const array_value *lv, const array_value *rv) const {
    return {element_range(lv), element_range(rv)};
  }
};


struct arithmetic_converter {
  template <class T>
  CXX_NORETURN
  any_value operator()(T) const { throw_type_error(); }

  any_value operator()(std::nullptr_t)  const { return nullptr; }

  // defined for the following types
  any_value operator()(std::int64_t v)  const { return v; }
  any_value operator()(std::uint64_t v) const { return v; }
  any_value operator()(double v)        const { return v; }

  any_value operator()(bool)            const { return nullptr; /* correct? */ }

  // need to convert values
  any_value operator()(const managed_string_view& v) const {
    const double        dblval = to_concrete(v, double{});
    const std::int64_t  intval = dblval;
    const bool          intval_is_precise = dblval == intval;

    if (intval_is_precise)
      return intval;

    const std::uint64_t uintval = dblval;
    const bool          uint_is_precise = uintval == dblval;

    if (uint_is_precise)
      return uintval;

    return dblval;
  }
};


any_value convert(any_value val, const arithmetic_operator &) {
  return std::visit(arithmetic_converter{}, val);
}


struct string_converter {
    template <class T>
    CXX_NORETURN
    managed_string_view operator()(T) const { throw_type_error(); }

    // defined for the following types
    managed_string_view operator()(managed_string_view val) const { return val; }
    // need to convert values
    managed_string_view operator()(bool val) const { return to_concrete(val, std::string_view{}); }
    managed_string_view operator()(std::int64_t val) const { return to_concrete(val, std::string_view{}); }
    managed_string_view operator()(std::uint64_t val) const { return to_concrete(val, std::string_view{}); }
    managed_string_view operator()(double val) const { return to_concrete(val, std::string_view{}); }
    managed_string_view operator()(std::nullptr_t) const { return to_concrete(nullptr, std::string_view{}); }
};


managed_string_view convert(any_value val, const string_operator_non_destructive &) {
  return std::visit(string_converter{}, val);
}

  struct array_converter {
    using result_type = array_value const*;

    // moves res into
    result_type to_array(any_value val) const {
      std::vector<value_variant> values;

      values.emplace_back(std::move(val));
      return &mk_array_value(std::move(values));
    }

    // defined for the following types
    template <class T>
    CXX_NORETURN
    result_type operator()(T&&) const                   { throw_type_error(); }

    result_type operator()(array_value const* v) const
    {
      return v->copy();
    }

    // need to move value_base to new array
    result_type operator()(const managed_string_view& v) const { return to_array(to_value(v)); }
    result_type operator()(bool v) const                       { return to_array(to_value(v)); }
    result_type operator()(std::int64_t v) const               { return to_array(to_value(v)); }
    result_type operator()(std::uint64_t v) const              { return to_array(to_value(v)); }
    result_type operator()(double v) const                     { return to_array(to_value(v)); }
    result_type operator()(std::nullptr_t) const               { return to_array(to_value(nullptr)); }
  };


any_value convert(any_value val, const array_operator &) {
  return std::visit(array_converter{}, val);
}
template <class value_t>
struct tag_type_traits
{
  using type = value_t;
};

template <>
struct tag_type_traits<managed_string_view>
{
  using type = std::string_view;
};

template <class value_t>
struct unpacker {
    using tag_type = tag_type_traits<value_t>::type;

    value_t conv(const value_t& val, const tag_type&) { return val; }

    template <class U>
    value_t conv(const U &val, const tag_type &tag) const {
      return to_concrete(val, tag);
    }

    template <class T>
    CXX_NORETURN
    value_t operator()(T) const { throw_type_error(); }

    // defined for the following types
    value_t operator()(const managed_string_view& val) const { return conv(val, tag_type{}); }

    // need to convert values
    value_t operator()(bool val) const { return conv(val, tag_type{}); }
    value_t operator()(std::int64_t val) const { return conv(val, tag_type{}); }
    value_t operator()(std::uint64_t val) const { return conv(val, tag_type{}); }
    value_t operator()(double val) const { return conv(val, tag_type{}); }
    value_t operator()(std::nullptr_t) const { return conv(nullptr, tag_type{}); }

    value_t operator()(array_value const* val) const {
      if constexpr (std::is_same<value_t, bool>::value) {
        CXX_LIKELY;
        return conv(val, tag_type{});
      }

      throw_type_error();
    }

};

/*
template <class T>
T unpack_value(const expr &expr, string_table& strings) {
  unpacker<T> unpack{strings};

  expr.accept(unpack);
  return std::move(unpack).result();
}
*/

template <class T>
T unpack_value(any_value el) {
  return std::visit(unpacker<T>{}, el);
}


//
// Json Logic - truthy/falsy


bool truthy(const any_value &el)
{
  return unpack_value<bool>(el);
}

bool falsy(const any_value &el) { return !truthy(el); }



namespace {


template <class T, class fn_t, class alt_fn_t>
auto with_type(any_value& v, fn_t fn, alt_fn_t altfn) -> decltype(altfn()) {
  if (T* casted = std::get_if<T>(&v))
    return fn(*casted);

  return altfn();
}

//
// binary operator - double dispatch pattern

template <class binary_op_t, class lhs_value_t>
struct binary_operator_visitor_2 { // : forwarding_visitor {
  using result_type = typename binary_op_t::result_type;

  binary_operator_visitor_2(lhs_value_t lval, binary_op_t oper)
      : lv(lval), op(oper) {}

  template <class rhs_value_t>
  result_type calc(rhs_value_t rv) const {
    auto [ll, rr] = op.coerce(lv, rv);
    //~ std::cerr << lv << " -- " << rv << " " << typeid(ll).name()
              //~ << std::endl;
    return op(std::move(ll), std::move(rr));
    //~ std::cerr << lv << " == " << rv << std::endl;
  }

  template <class T>
  CXX_NORETURN
  result_type operator()(T) const { throw_type_error(); }

  result_type operator()(const managed_string_view& n) const {
    if constexpr (binary_op_t::defined_for_string) return calc(&n);

    throw_type_error();
  }

  result_type operator()(std::nullptr_t) const {
    if constexpr (binary_op_t::defined_for_null) return calc(nullptr);

    throw_type_error();
  }

  result_type operator()(bool n) const {
    if constexpr (binary_op_t::defined_for_boolean) return calc(&n);

    throw_type_error();
  }

  result_type operator()(std::int64_t n) const {
    if constexpr (binary_op_t::defined_for_integer) {
      try {
        return calc(&n);
      } catch (const not_int64_error &ex) {
        if (n < 0) {
          CXX_UNLIKELY;
          throw std::range_error{
              "unable to consolidate uint>max(int) with int<0"};
        }
      }

      std::uint64_t alt = n;
      return calc(&alt);
    }

    throw_type_error();
  }

  result_type operator()(std::uint64_t n) const {
    if constexpr (binary_op_t::defined_for_integer) {
      try {
        return calc(&n);
      } catch (const not_uint64_error &ex) {
        if (n > std::uint64_t(std::numeric_limits<std::int64_t>::max)) {
          CXX_UNLIKELY;
          throw std::range_error{
              "unable to consolidate int<0 with uint>max(int)"};
        }
      }

      std::int64_t alt = n;
      return calc(&alt);
    }

    throw_type_error();
  }

  result_type operator()(double n) const {
    if constexpr (binary_op_t::defined_for_real) return calc(&n);

    throw_type_error();
  }

  result_type operator()(array_value const* n) const {
    result_type res;
    if constexpr (binary_op_t::defined_for_array) {
      try {
        res = calc(n);
      } catch (const unpacked_array_req &) {
        assert(deref(n).value().size() == 1);

        res = std::visit(*this, n->value().front());
      }

      return res;
    }

    throw_type_error();
  }

 private:
  lhs_value_t lv;
  mutable binary_op_t op;
};

template <class binary_op_t>
struct binary_operator_visitor  {
  using result_type = typename binary_op_t::result_type;

  binary_operator_visitor(binary_op_t oper, const any_value &rhsarg)
      : op(oper), rhs(rhsarg) {}

  template <class LhsValue>
  result_type calc(LhsValue lv) const {
    using rhs_visitor = binary_operator_visitor_2<binary_op_t, LhsValue>;

    return std::visit(rhs_visitor{lv, op}, rhs);
  }

  template <class T>
  CXX_NORETURN
  result_type operator()(T) const { throw_type_error(); }

  result_type operator()(const managed_string_view& n) const {
    if constexpr (binary_op_t::defined_for_string) return calc(&n);

    throw_type_error();
  }

  result_type operator()(std::nullptr_t) const  {
    if constexpr (binary_op_t::defined_for_null) return calc(nullptr);

    throw_type_error();
  }

  result_type operator()(bool n) const {
    if constexpr (binary_op_t::defined_for_boolean) return calc(&n);

    throw_type_error();
  }

  result_type operator()(std::int64_t n) const {
    if constexpr (binary_op_t::defined_for_integer) {
      try {
        return calc(&n);
      } catch (const not_int64_error &ex) {
        if (n < 0) {
          CXX_UNLIKELY;
          throw std::range_error{
              "unable to consolidate int<0 with uint>max(int)"};
        }
      }

      std::uint64_t alt = n;

      return calc(&alt); // should we rethrow range_error
    }

    throw_type_error();
  }

  result_type operator()(std::uint64_t n) const {
    if constexpr (binary_op_t::defined_for_integer) {
      try {
        return calc(&n);
      } catch (const not_uint64_error &ex) {
        if (n > std::uint64_t(std::numeric_limits<std::int64_t>::max)) {
          CXX_UNLIKELY;
          throw std::range_error{
              "unable to consolidate uint>max(int) with int<0"};
        }
      }

      std::int64_t alt = n;
      return calc(&alt); // should we rethrow range_error
    }

    throw_type_error();
  }

  result_type operator()(double n) const {
    if constexpr (binary_op_t::defined_for_real) return calc(&n);

    throw_type_error();
  }

  result_type operator()(array_value const* n) const {
    if constexpr (binary_op_t::defined_for_array) {
      try {
        return calc(n);
      } catch (const unpacked_array_req &) {
        assert(deref(n).value().size() == 1);

        return std::visit(*this, n->value().front());
      }

      return result_type{}; // \TODO was: return;
    }

    throw_type_error();
  }


 private:
  binary_op_t      op;
  const any_value& rhs;
};

//
// compute and sequence functions

template <class binary_op_t>
typename binary_op_t::result_type compute(const any_value &lhs,
                                          const any_value &rhs, binary_op_t op) {
  using lhs_visitor = binary_operator_visitor<binary_op_t>;

  return std::visit(lhs_visitor{op, rhs}, lhs);
}

template <class binary_predicate_t>
bool compare_sequence(variant_span lv, variant_span rv, binary_predicate_t pred) {
  const std::size_t lsz = lv.size();
  const std::size_t rsz = rv.size();

  if (lsz == 0) return pred(false, rsz != 0);

  if (rsz == 0) return pred(true, false);

  std::size_t const len = std::min(lsz, rsz);
  auto const        lbeg = lv.begin();
  bool              res = false;
  auto cmpElems = [&pred,&res](const any_value& lhs, const any_value& rhs) -> bool
                  {
                    res = compute(lhs, rhs, pred);
                    return res == compute(rhs, lhs, pred);
                  };

  auto const [lpos, rpos] = std::mismatch(lbeg, lbeg + len, rv.begin(), cmpElems);
  bool const matching_elems = lbeg + len == lpos;

  if (matching_elems)
    res = pred(lsz, rsz);

  return res;
}


template <class binary_predicate_t>
bool compare_sequence(array_value const *lv, array_value const *rv, binary_predicate_t pred) {
  return compare_sequence(element_range(lv), element_range(rv), std::move(pred));
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

  result_type operator()(variant_span lv, variant_span rv) const {
    return compare_sequence(lv, rv, *this);
  }

  result_type operator()(const managed_string_view&, std::nullptr_t) const {
    return false;
  }

  result_type operator()(std::nullptr_t, const managed_string_view&) const {
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

  result_type operator()(variant_span lv, variant_span rv) const {
    return compare_sequence(lv, rv, *this);
  }

  result_type operator()(const managed_string_view&, std::nullptr_t) const {
    return false;
  }

  result_type operator()(std::nullptr_t, const managed_string_view&) const {
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

  result_type operator()(variant_span lv, variant_span rv) const {
    return compare_sequence(lv, rv, *this);
  }

  result_type operator()(const managed_string_view& lhs, std::nullptr_t) const {
    return lhs.empty();
  }

  result_type operator()(std::nullptr_t, const managed_string_view& rhs) const {
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

  result_type operator()(variant_span lv, variant_span rv) const {
    return compare_sequence(lv, rv, *this);
  }

  result_type operator()(const managed_string_view& lhs, std::nullptr_t) const {
    return lhs.empty();
  }

  result_type operator()(std::nullptr_t, const managed_string_view& rhs) const {
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
    return to_value(nullptr);
  }

  template <class T>
  result_type operator()(const T &lhs, const T &rhs) const {
    return to_value(lhs + rhs);
  }
};

template <>
struct operator_impl<subtract> : arithmetic_operator {
  using arithmetic_operator::result_type;

  result_type operator()(std::nullptr_t, std::nullptr_t) const {
    return to_value(nullptr);
  }

  template <class T>
  result_type operator()(const T &lhs, const T &rhs) const {
    return to_value(lhs - rhs);
  }
};

template <>
struct operator_impl<multiply> : arithmetic_operator {
  using arithmetic_operator::result_type;

  result_type operator()(std::nullptr_t, std::nullptr_t) const {
    return to_value(nullptr);
  }

  template <class T>
  result_type operator()(const T &lhs, const T &rhs) const {
    return to_value(lhs * rhs);
  }
};

template <>
struct operator_impl<divide> : arithmetic_operator {
  using arithmetic_operator::result_type;

  result_type operator()(std::nullptr_t, std::nullptr_t) const {
    return to_value(nullptr);
  }

  result_type operator()(double lhs, double rhs) const {
    double res = lhs / rhs;

    // if (isInteger(res)) return toInt(res);
    return to_value(res);
  }

  template <class Int_t>
  result_type operator()(Int_t lhs, Int_t rhs) const {
    if (lhs % rhs) return (*this)(double(lhs), double(rhs));

    return to_value(lhs / rhs);
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
    if (rhs == 0) return to_value(nullptr);

    return to_value(lhs % rhs);
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
    return to_value(std::min(lhs, rhs));
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
    return to_value(std::max(lhs, rhs));
  }
};

template <>
struct operator_impl<logical_not> {
  using result_type = bool;

  result_type operator()(any_value val) const { return falsy(val); }
};

template <>
struct operator_impl<logical_not_not> {
  using result_type = bool;

  result_type operator()(any_value val) const { return truthy(val); }
};

template <>
struct operator_impl<cat> : string_operator_non_destructive {
  using string_operator_non_destructive::result_type;

  result_type operator()(const managed_string_view& lhs, const managed_string_view& rhs) const {
    std::string tmp;

    tmp.reserve(lhs.size() + rhs.size());

    tmp.append(lhs.begin(), lhs.end());
    tmp.append(rhs.begin(), rhs.end());

    return to_value(managed_string_view(std::move(tmp)));
  }

};

/// implements the string mode of the membership operator
///    the mode testing element in an array is implemented
///    in the evaluator::visit function.
template <>
struct operator_impl<membership> : string_operator_non_destructive {
  using string_operator_non_destructive::result_type;

  result_type operator()(const managed_string_view& lhs, const managed_string_view& rhs) const {
    const bool res = (rhs.find(lhs) != json::string::npos);

    return to_value(res);
  }
};

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
template <>
struct operator_impl<regex_match>
    : string_operator_non_destructive  // \todo the conversion rules differ
{
  using string_operator_non_destructive::result_type;

  result_type operator()(const managed_string_view& lhs, const managed_string_view& rhs) const {
    std::regex rgx(lhs.c_str(), lhs.size());

    return to_value(std::regex_search(rhs.begin(), rhs.end(), rgx));
  }
};
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */

template <>
struct operator_impl<merge> : array_operator {
  using array_operator::result_type;

  result_type operator()(const variant_span ll, const variant_span rr) const {
    std::vector<value_variant> values;

    values.reserve(ll.size() + rr.size());

    std::copy(ll.begin(), ll.end(), std::back_inserter(values));
    std::copy(rr.begin(), rr.end(), std::back_inserter(values));

    return &mk_array_value(std::move(values));
  }
};

using variant_logger = std::function<void(const value_variant&)>;

struct evaluator : forwarding_visitor {
  evaluator(variable_accessor varAccess, variant_logger& out)
      : vars(std::move(varAccess)), logger(out), calcres(nullptr) {}

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

  void visit(const null_value &) final;
  void visit(const bool_value &) final;
  void visit(const int_value &) final;
  void visit(const unsigned_int_value &) final;
  void visit(const real_value &) final;
  void visit(const string_value &) final;
  void visit(const array_value &) final;

  void visit(const error &) final;

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
  void visit(const regex_match &) final;
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */

  any_value eval(const expr &);

 private:
  variable_accessor vars;
  variant_logger& logger;
  any_value calcres;

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
  std::int64_t unpack_optional_int_arg(const oper &n, int argpos,
                                       std::int64_t defaultVal);

  /// auxiliary missing method
  std::tuple<std::vector<value_variant>, std::size_t>
  missing_aux(const oper& n, std::size_t arrpos);

  template <class ValueNode>
  void _value(const ValueNode &val) {
    calcres = to_value(val.value());
  }
};

struct sequence_function {
  sequence_function(const expr &e, variant_logger &logstream)
      : exp(e), logger(logstream) {}

  any_value operator()(const any_value &elem) const {
    evaluator sub{[&elem](value_variant keyval, int) -> any_value {
                    if (managed_string_view* pkey = std::get_if<managed_string_view>(&keyval)) {
                      if (pkey->size() == 0)
                        return elem;
#if OBSOLETE
                      try {
                        object_value &o = down_cast<object_value>(**elptr);

                        if (auto pos = o.find(*pkey); pos != o.end())
                          return to_value(pos->second);
                      } catch (const type_error &) {
                      }
#endif /*OBSOLETE*/
                    }

                    return nullptr;
                  },
                  logger};

    return sub.eval(exp);
  }

 private:
  const expr &exp;
  variant_logger& logger;
};

struct sequence_predicate : sequence_function {
  using sequence_function::sequence_function;

  bool operator()(const any_value &elem) const {
    return truthy(sequence_function::operator()(elem));
  }
};

// \todo do we still need this class??
struct sequence_predicate_nondestructive : sequence_function {
  using sequence_function::sequence_function;

  bool operator()(const any_value &elem) const {
    return truthy(sequence_function::operator()(elem));
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
  sequence_reduction(expr &e, variant_logger& logstream)
      : exp(e), logger(logstream) {}

  any_value operator()(any_value accu, any_value elem) const {
    evaluator sub{[&accu, &elem](value_variant keyval, int) -> any_value {
                    if (const managed_string_view *pkey = std::get_if<managed_string_view>(&keyval)) {
                      CXX_LIKELY;
                      if (*pkey == "current") return elem;
                      if (*pkey == "accumulator") return accu;
                    }
                    return to_value(nullptr);
                  },
                  logger};

    return sub.eval(exp);
  }

 private:
  expr &exp;
  variant_logger& logger;
};

std::int64_t evaluator::unpack_optional_int_arg(const oper &n, int argpos,
                                                std::int64_t defaultVal) {
  if (std::size_t(argpos) >= n.size()) {
    CXX_UNLIKELY;
    return defaultVal;
  }

  return unpack_value<std::int64_t>(eval(n.operand(argpos)));
}

template <class unary_predicate_t>
void evaluator::unary(const oper &n, unary_predicate_t pred) {
  CXX_MAYBE_UNUSED
  const int num = n.num_evaluated_operands();
  assert(num == 1);

  const bool res = pred(eval(n.operand(0)));

  calcres = res;
}

template <class binary_op_t>
void evaluator::binary(const oper &n, binary_op_t binop) {
  const int num = n.num_evaluated_operands();
  assert(num == 1 || num == 2);

  int idx = -1;
  any_value lhs;

  if (num == 2) {
    CXX_LIKELY;
    lhs = eval(n.operand(++idx));
  } else {
    lhs = std::int64_t(0);
  }

  any_value rhs = eval(n.operand(++idx));

  calcres = compute(lhs, rhs, binop);
}

template <class binary_op_t>
void evaluator::reduce_sequence(const oper &n, binary_op_t op) {
  const int num = n.num_evaluated_operands();
  assert(num >= 1);

  int idx = -1;

  any_value tmp0 = eval(n.operand(++idx));
  any_value res  = convert(std::move(tmp0), op);

  while (idx != (num - 1)) {
    any_value tmp1 = eval(n.operand(++idx));
    any_value rhs  = convert(std::move(tmp1), op);
    any_value tmp2 = compute(res, rhs, op);

    res = std::move(tmp2);
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
  any_value rhs = eval(n.operand(++idx));

  while (res && (idx != (num - 1))) {
    any_value lhs = std::move(rhs);

    rhs = eval(n.operand(++idx));
    res = compute(lhs, rhs, pred);
  }

  calcres = res;
}

void evaluator::eval_short_circuit(const oper &n, bool val) {
  const int num = n.num_evaluated_operands();

  if (num == 0) {
    CXX_UNLIKELY;
    throw_type_error();
  }

  int idx = -1;
  any_value tmpval = eval(n.operand(++idx));
  //~ std::cerr << idx << ") " << oper << std::endl;

  bool found = (idx == num - 1) || (truthy(tmpval) == val);

  // loop until *aa == val or when *aa is the last valid element
  while (!found) {
    tmpval = eval(n.operand(++idx));

    found = (idx == (num - 1)) || (truthy(tmpval) == val);
  }

  calcres = std::move(tmpval);
}

any_value evaluator::eval(const expr &n) {
  any_value res;

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
  reduce_sequence(n, operator_impl<cat>{});
}

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
void evaluator::visit(const regex_match &n) {
  binary(n, operator_impl<regex_match>{});
}
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */

void evaluator::visit(const membership &n) {
  assert(n.num_evaluated_operands() >= 1);

  // lhs in rhs - rhs is a possibly large set.
  any_value lhs = eval(n.operand(0));
  any_value rhs = eval(n.operand(1));

  auto array_op = [&lhs](array_value const* v) -> any_value {
    variant_span spn = element_range(v);
    auto const   lim = spn.end();

    return std::find(spn.begin(), lim, lhs) != lim;
  };

  auto string_op = [&lhs, &rhs]() -> any_value {
    try {
      return compute(lhs, rhs, operator_impl<membership>{});
    } catch (const type_error &) {
    }

    return to_value(false);
  };

  calcres = with_type<array_value const*>(rhs, array_op, string_op);
}

void evaluator::visit(const substr &n) {
  assert(n.num_evaluated_operands() >= 1);

  managed_string_view str = unpack_value<managed_string_view>(eval(n.operand(0)));
  std::int64_t ofs = unpack_optional_int_arg(n, 1, 0);
  std::int64_t cnt = unpack_optional_int_arg(n, 2, 0);

  if (ofs < 0) {
    CXX_UNLIKELY;
    ofs = std::max(std::int64_t(str.size()) + ofs, std::int64_t(0));
  }

  if (cnt < 0) {
    CXX_UNLIKELY;
    cnt = std::max(std::int64_t(str.size()) - ofs + cnt, std::int64_t(0));
  }

  calcres = to_value(str.substr(ofs, cnt));
}

void evaluator::visit(const array &n) {
  std::vector<any_value> elems;

  elems.reserve(n.num_evaluated_operands());

  evaluator *self = this;
  auto      eval_array_elems = [self](const any_expr &exp) -> any_value { return self->eval(*exp); };

  std::transform(n.begin(), n.end(), std::back_inserter(elems), eval_array_elems);

  calcres = &mk_array_value( std::move(elems) );
}

void evaluator::visit(const merge &n) {
  reduce_sequence(n, operator_impl<merge>{});
}

void evaluator::visit(const reduce &n) {
  any_value arr = eval(n.operand(0));
  expr &expr = n.operand(1);
  any_value accu = eval(n.operand(2));

  auto op = [&expr, accu, calclogger = &this->logger]
            (array_value const* v) -> any_value {
    variant_span spn = element_range(v);
    return std::accumulate( spn.begin(), spn.end(),
                            std::move(accu),
                            sequence_reduction{expr, *calclogger}
                          );
  };

  auto mkInvalid = []() -> any_value { return nullptr; };

  calcres = with_type<array_value const*>(arr, op, mkInvalid);
}

array_value const*
empty_array_value() { return &mk_array_value(); }

void evaluator::visit(const map &n) {
  any_value arr = eval(n.operand(0));
  auto mapper = [&n, &arr, calclogger = &this->logger]
                 (array_value const* v) -> array_value const* {
    variant_span spn = element_range(v);
    expr &expr = n.operand(1);
    std::vector<any_value> mapped_elements;

    mapped_elements.reserve(spn.size());

    std::transform(spn.begin(), spn.end(),
                   std::back_inserter(mapped_elements),
                   sequence_function{expr, *calclogger});

    return &mk_array_value(std::move(mapped_elements));
  };

  calcres = with_type<array_value const*>(arr, mapper, &empty_array_value);
}

void evaluator::visit(const filter &n) {
  any_value arr = eval(n.operand(0));
  auto filter = [&n, &arr, calclogger = &this->logger]
                (array_value const* v) -> array_value const* {
    variant_span spn = element_range(v);
    expr &expr = n.operand(1);
    std::vector<any_value> filtered_elements;

    // non destructive predicate is required for evaluating and copying
    std::copy_if(spn.begin(), spn.end(),
                 std::back_inserter(filtered_elements),
                 sequence_predicate_nondestructive{expr, *calclogger});

    return &mk_array_value(std::move(filtered_elements));
  };

  calcres = with_type<array_value const*>(arr, filter, &empty_array_value);
}

void evaluator::visit(const all &n) {
  any_value arr = eval(n.operand(0));

  auto all_of = [&n, &arr, calclogger = &this->logger]
                (array_value const* v) -> bool {
    variant_span spn = element_range(v);
    expr &expr = n.operand(1);

    return std::all_of( spn.begin(), spn.end(),
                        sequence_predicate{expr, *calclogger}
                      );
  };

  auto mismatch_false = []()->bool { return false; };

  calcres = with_type<array_value const*>(arr, all_of, mismatch_false);
}

void evaluator::visit(const none &n) {
  any_value arr = eval(n.operand(0));

  auto none_of = [&n, &arr, calclogger = &this->logger]
                 (array_value const* v) -> bool {
    variant_span spn = element_range(v);
    expr &expr = n.operand(1);

    return std::none_of( spn.begin(), spn.end(),
                         sequence_predicate{expr, *calclogger}
                       );
  };

  auto mismatch_true = []()->bool { return true; };

  calcres = with_type<array_value const*>(arr, none_of, mismatch_true);
}

void evaluator::visit(const some &n) {
  any_value arr = eval(n.operand(0));

  auto any_of = [&n, &arr, calclogger = &this->logger]
                (array_value const* v) -> bool {
    variant_span spn = element_range(v);
    expr &expr = n.operand(1);

    return std::any_of( spn.begin(), spn.end(),
                        sequence_predicate{expr, *calclogger}
                      );
  };

  auto mismatch_false = []()->bool { return false; };

  calcres = with_type<array_value const*>(arr, any_of, mismatch_false);
}

void evaluator::visit(const error &) { unsupported(); }

void evaluator::visit(const var &n) {
  assert(n.num_evaluated_operands() >= 1);

  any_value elm = eval(n.operand(0));

  try {
    calcres = vars(elm, n.num());
  } catch (const variable_resolution_error &) {
    calcres = (n.num_evaluated_operands() > 1) ? eval(n.operand(1))
                                               : to_value(nullptr);
  }
}

std::tuple<std::vector<value_variant>, std::size_t>
evaluator::missing_aux(const oper& n, std::size_t arrpos) {
  any_value arr = eval(n.operand(arrpos));
  std::vector<value_variant> tmpval;

  auto non_array_alt = [&tmpval, &arr, &n, pos=arrpos+1, calc = this]() -> variant_span
  {
    std::transform( std::next(n.begin(), pos), n.end(),
                    std::back_inserter(tmpval),
                    [&calc](const any_expr &e) -> any_value { return calc->eval(*e); }
                  );

    return element_range(tmpval);
  };

  auto unwrapped = [](array_value const* v) -> variant_span { return element_range(v); };

  variant_span elems = with_type<array_value const*>(arr, unwrapped, non_array_alt);

  auto notavail = [calc = this](const any_value &val) -> bool {
    try {
      any_value res = calc->vars(val, COMPUTED_VARIABLE_NAME);

      return null_equivalent(res);
    } catch (const jsonlogic::variable_resolution_error &) {
      return true;
    }
  };

  std::vector<value_variant> res;

  std::copy_if( std::make_move_iterator(elems.begin()), std::make_move_iterator(elems.end()),
                std::back_inserter(res),
                notavail
              );

  return { std::move(res),
           elems.size() - res.size()
         };
}

void evaluator::visit(const missing &n) {
  std::vector<value_variant> res;

  std::tie(res, std::ignore) = missing_aux(n, 0);

  calcres = &mk_array_value(std::move(res));
}

void evaluator::visit(const missing_some &n) {
  const std::uint64_t minreq = unpack_value<std::uint64_t>(eval(n.operand(0)));

  auto [res, avail] = missing_aux(n, 1);

  if (avail >= minreq)
    res.clear();

  calcres = &mk_array_value(std::move(res));
}

void evaluator::visit(const if_expr &n) {
  const int num = n.num_evaluated_operands();

  if (num == 0) {
    calcres = to_value(nullptr);
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

  calcres = (pos < num) ? eval(n.operand(pos)) : to_value(nullptr);
}

void evaluator::visit(const log &n) {
  assert(n.num_evaluated_operands() == 1);

  calcres = eval(n.operand(0));

  logger(calcres);
}

void evaluator::visit(const array_value &n) { calcres = new array_value(n); }

void evaluator::visit(const null_value &n) { _value(n); }
void evaluator::visit(const bool_value &n) { _value(n); }
void evaluator::visit(const int_value &n) { _value(n); }
void evaluator::visit(const unsigned_int_value &n) { _value(n); }
void evaluator::visit(const real_value &n) { _value(n); }
void evaluator::visit(const string_value &n) { _value(n); }

any_value eval_path(const managed_string_view& path, const json::object *obj) {
  if (obj) {
    if (auto pos = obj->find(path); pos != obj->end())
      return jsonlogic::to_value(pos->value());

    if (std::size_t pos = path.find('.'); pos != json::string::npos) {
      managed_string_view selector = path.substr(0, pos);
      managed_string_view suffix   = path.substr(pos + 1);

      return eval_path(suffix, obj->at(selector).if_object());
    }
  }

  throw variable_resolution_error{"in logic.cc::eval_path."};
}

template <class IntT>
any_value eval_index(IntT idx, const json::array &arr) {
  return jsonlogic::to_value(arr[idx]);
}


any_value apply(const expr &exp, const variable_accessor &vars) {
  variant_logger logger = [](const value_variant& val) -> void { std::cerr << val << std::endl; };
  evaluator ev{vars, logger};

  return ev.eval(exp);
}

any_value apply(logic_data& rule, const variable_accessor &vars) {
  assert(rule.syntax_tree().get());
  return jsonlogic::apply(*rule.syntax_tree(), vars);
}

any_value apply(logic_data& rule) {
  auto no_variables = [](value_variant, int) -> any_value { throw std::logic_error{"variable accessor not available"}; };

  return jsonlogic::apply(rule, no_variables);
}

any_value apply(logic_data& rule, std::vector<value_variant> vars) {
  return jsonlogic::apply(rule, variant_accessor(std::move(vars)));
}

}  // namespace



variable_accessor json_accessor(json::value data) {
  return [data = std::move(data)](value_variant keyval, int) -> any_value {
    if (const managed_string_view *ppath = std::get_if<managed_string_view>(&keyval)) {
      //~ std::cerr << *ppath << std::endl;
      return ppath->size() ? eval_path(*ppath, data.if_object())
                           : to_value(data);
    }

    if (const std::int64_t *pidx = std::get_if<std::int64_t>(&keyval))
      return eval_index(*pidx, data.as_array());

    if (const std::uint64_t *puidx = std::get_if<std::uint64_t>(&keyval))
      return eval_index(*puidx, data.as_array());

    throw std::logic_error{"jsonlogic - unsupported var access"};
  };
}

variable_accessor variant_accessor(std::vector<value_variant> vars) {
  return [vars = std::move(vars)](value_variant, int idx) -> any_value {
    if ((idx >= 0) && (std::size_t(idx) < vars.size())) {
      CXX_LIKELY;
      return vars[idx];
    }

    throw std::logic_error{"unable to access (computed) variable"};
  };
}



std::ostream& operator<<(std::ostream& os, const value_variant& val)
{
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

    case sint_variant: {
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
      os << '"' << std::string_view(std::get<managed_string_view>(val)) << '"';
      break;
    }

    case sequ_variant: {
      const char* sep = "";
      os << '[';
      for (const value_variant& v : std::get<array_value const*>(val)->value()) {
        os << sep << v;
        sep = ",";
      }
      os << ']';
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

} // namespace

std::ostream &operator<<(std::ostream &os, any_expr &n) {
  value_printer prn{os};

  deref(n).accept(prn);
  return os;
}

expr &oper::operand(int n) const { return deref(this->at(n).get()); }

//
// logic_rule

logic_rule::logic_rule(std::unique_ptr<logic_data>&& rule_data)
: data(std::move(rule_data))
{}

logic_rule::logic_rule(logic_rule&&)            = default;
logic_rule& logic_rule::operator=(logic_rule&&) = default;
logic_rule::~logic_rule()                       = default;

logic_data& logic_rule::internal_data() { return *data; }

std::vector<std::string_view> const &logic_rule::variable_names() const {
  return data->variable_names();
}

bool logic_rule::has_computed_variable_names() const {
  return data->has_computed_variable_names();
}

any_value logic_rule::apply() { return jsonlogic::apply(*data); }


any_value logic_rule::apply(const variable_accessor &var_accessor) {
  return jsonlogic::apply(*data, var_accessor);
}

any_value logic_rule::apply(std::vector<value_variant> vars)  {
  return jsonlogic::apply(*data, std::move(vars));
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

/// AST traversal function that calls v's visit methods in post-fix
/// order
void traverseInSAttributeOrder(expr &e, visitor &vis);

void traverseInSAttributeOrder(expr &e, visitor &vis) {
  SAttributeTraversal trav{vis};

  e.accept(trav);
}
#endif /* SUPPLEMENTAL */
