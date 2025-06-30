#pragma once

#include <memory>
#include <map>
#include <set>
#include <vector>
#include <iosfwd>
#include <iostream>

#include "cxx-compat.hpp"

#if !defined(WITH_JSONLOGIC_EXTENSIONS)
#define WITH_JSONLOGIC_EXTENSIONS 1
#endif /* !defined(WITH_JSONLOGIC_EXTENSIONS) */

namespace jsonlogic {

struct visitor;

// the root class
struct expr {
  virtual ~expr()               = default;
  expr()                        = default;
  expr(expr &&)                 = default;
  expr(const expr &)            = default;
  expr &operator=(expr &&)      = default;
  expr &operator=(const expr &) = default;

  virtual void accept(visitor &) const = 0;
};

using any_expr = std::unique_ptr<expr>;

struct oper : expr, private std::vector<any_expr> {
  using container_type = std::vector<any_expr>;

  using container_type::at;
  using container_type::back;
  using container_type::begin;
  using container_type::const_iterator;
  using container_type::const_reverse_iterator;
  using container_type::crbegin;
  using container_type::crend;
  using container_type::end;
  using container_type::iterator;
  using container_type::push_back;
  using container_type::rbegin;
  using container_type::rend;
  using container_type::reverse_iterator;
  using container_type::size;

  oper() = default;

  // convenience function so that the constructor does not need to be
  // implemented in every derived class.
  void set_operands(container_type &&opers) { this->swap(opers); }

  const container_type &operands() const { return *this; }
  container_type &operands() { return *this; }
  container_type &&move_operands() && { return std::move(*this); }

  expr &operand(int n) const;

  virtual int num_evaluated_operands() const;

  private:
    oper(oper &&)                 = delete;
    oper(const oper &)            = delete;
    oper &operator=(oper &&)      = delete;
    oper &operator=(const oper &) = delete;
};

// defines operators that have an upper bound on how many
//   arguments are evaluated.
template <int MaxArity>
struct oper_n : oper {
  enum { MAX_OPERANDS = MaxArity };

  int num_evaluated_operands() const final;
};

struct value_base : expr {
  virtual value_variant to_variant() const = 0;
};

template <class T>
struct value_generic : value_base {
  using value_type = T;

  explicit value_generic(value_type t) : val(std::move(t)) {}

  // T &value() { return val; }
  const value_type &value() const { return val; }

  value_variant to_variant() const final;

private:
  value_type val;
};

//
// comparisons

// binary
struct equal : oper_n<2> {
  void accept(visitor &) const final;
};

struct strict_equal : oper_n<2> {
  void accept(visitor &) const final;
};

struct not_equal : oper_n<2> {
  void accept(visitor &) const final;
};

struct strict_not_equal : oper_n<2> {
  void accept(visitor &) const final;
};

// binary or ternary
struct less : oper_n<3> {
  void accept(visitor &) const final;
};

struct greater : oper_n<3> {
  void accept(visitor &) const final;
};

struct less_or_equal : oper_n<3> {
  void accept(visitor &) const final;
};

struct greater_or_equal : oper_n<3> {
  void accept(visitor &) const final;
};

// logical operators

// unary
struct logical_not : oper_n<1> {
  void accept(visitor &) const final;
};

struct logical_not_not : oper_n<1> {
  void accept(visitor &) const final;
};

// n-ary
struct logical_and : oper {
  void accept(visitor &) const final;
};

struct logical_or : oper {
  void accept(visitor &) const final;
};

// control structure
struct if_expr : oper {
  void accept(visitor &) const final;
};

// n-ary arithmetic

struct add : oper {
  void accept(visitor &) const final;
};

struct multiply : oper {
  void accept(visitor &) const final;
};

struct min : oper {
  void accept(visitor &) const final;
};

struct max : oper {
  void accept(visitor &) const final;
};

// binary arithmetic

struct subtract : oper_n<2> {
  void accept(visitor &) const final;
};

struct divide : oper_n<2> {
  void accept(visitor &) const final;
};

struct modulo : oper_n<2> {
  void accept(visitor &) const final;
};

// array

// arrays are considered collections of uninterpreted expressions.
//   interpreted (evaluated) expressions are stored in a value_array.
//   the distinction helps to avoid unnecessary copying and interpretation
//   steps.
// The class is final and it supports move ctor/assignment, so the data
//   can move efficiently.
struct array final : oper  // array is modeled as operator
{
  void accept(visitor &) const final;

  array() = default;

  array(array &&);
  array &operator=(array &&);
};

struct map : oper_n<2> {
  void accept(visitor &) const final;
};

struct reduce : oper_n<3> {
  void accept(visitor &) const final;
};

struct filter : oper_n<2> {
  void accept(visitor &) const final;
};

struct all : oper_n<2> {
  void accept(visitor &) const final;
};

struct none : oper_n<2> {
  void accept(visitor &) const final;
};

struct some : oper_n<2> {
  void accept(visitor &) const final;
};

struct merge : oper {
  void accept(visitor &) const final;
};

// data access
struct var : oper {
  enum { computed = -1 };

  void accept(visitor &) const final;

  void num(int val) { idx = val; }
  std::int16_t num() const { return idx; }

private:
  std::int16_t idx = computed;
};

/// missing is modeled as operator with arbitrary number of arguments
/// \details
/// in Calculator::visit(missing&) :
///   if the first argument is an array, only the array will be considered
///   otherwise all operands are treated as array.
/// \{
struct missing : oper {
  void accept(visitor &) const final;
};

struct missing_some : oper_n<2> {
  void accept(visitor &) const final;
};
/// \}

/// string operations
/// \{
struct cat : oper {
  void accept(visitor &) const final;
};

struct substr : oper_n<3> {
  void accept(visitor &) const final;
};
/// \}

/// string and array operation implementing "in"
struct membership : oper {
  void accept(visitor &) const final;
};

/// value classes
///   all but object_data are closely aligned with types listed value_variant.
/// \todo consider using a single value_variant class...
/// \{
struct null_value : value_base {
  null_value() = default;
  null_value(std::nullptr_t) {}

  void accept(visitor &) const final;

  std::nullptr_t value() const { return nullptr; }

  value_variant to_variant() const final;
};

struct bool_value : value_generic<bool> {
  using base = value_generic<bool>;
  using base::base;

  void accept(visitor &) const final;
};

struct int_value : value_generic<std::int64_t> {
  using base = value_generic<std::int64_t>;
  using base::base;

  void accept(visitor &) const final;
};

struct unsigned_int_value : value_generic<std::uint64_t> {
  using base = value_generic<std::uint64_t>;
  using base::base;

  void accept(visitor &) const final;
};

struct real_value : value_generic<double> {
  using base = value_generic<double>;
  using base::base;

  void accept(visitor &) const final;
};

struct string_value : value_generic<std::string_view> {
  using base = value_generic<std::string_view>;
  using base::base;

  void accept(visitor &) const final;
};

struct array_value : value_base
{
    using container_type = std::vector<value_variant>;

    ~array_value()                              = default;
    array_value(array_value&&)                  = default;
    array_value& operator=(array_value&&)       = default;
    array_value(const array_value&)             = default;
    array_value& operator=(const array_value&)  = default;

    explicit
    array_value(container_type elems)
    : vec(std::make_shared<container_type>(std::move(elems)))
    {}

    value_variant to_variant() const final;
    container_type const& value() const;
    const array_value* copy() const;
    void accept(visitor &) const final;

  private:
    const std::shared_ptr<container_type> vec;

    array_value()                               = delete;
};


// object types do not seem to have strong support by jsonlogic
using object_value_data = std::map<std::string_view, any_expr>;

struct object_value : expr, private object_value_data {
  using base = object_value_data;
  using base::base;

  ~object_value() = default;

  using base::begin;
  using base::const_iterator;
  using base::end;
  using base::find;
  using base::insert;
  using base::iterator;
  using base::value_type;

  base &elements() { return *this; }

  void accept(visitor &) const final;
};

/// \}

/// logger
struct log : oper_n<1> {
  void accept(visitor &) const final;
};

/// error node
struct error : expr {
  void accept(visitor &) const final;
};

//
// jsonlogic extensions

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
struct regex_match : oper_n<2> {
  void accept(visitor &) const final;
};
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */

// visitor
struct visitor {
  virtual void visit(const expr &) = 0;  // error
  virtual void visit(const oper &n) = 0;
  virtual void visit(const equal &) = 0;
  virtual void visit(const strict_equal &) = 0;
  virtual void visit(const not_equal &) = 0;
  virtual void visit(const strict_not_equal &) = 0;
  virtual void visit(const less &) = 0;
  virtual void visit(const greater &) = 0;
  virtual void visit(const less_or_equal &) = 0;
  virtual void visit(const greater_or_equal &) = 0;
  virtual void visit(const logical_and &) = 0;
  virtual void visit(const logical_or &) = 0;
  virtual void visit(const logical_not &) = 0;
  virtual void visit(const logical_not_not &) = 0;
  virtual void visit(const add &) = 0;
  virtual void visit(const subtract &) = 0;
  virtual void visit(const multiply &) = 0;
  virtual void visit(const divide &) = 0;
  virtual void visit(const modulo &) = 0;
  virtual void visit(const min &) = 0;
  virtual void visit(const max &) = 0;
  virtual void visit(const map &) = 0;
  virtual void visit(const reduce &) = 0;
  virtual void visit(const filter &) = 0;
  virtual void visit(const all &) = 0;
  virtual void visit(const none &) = 0;
  virtual void visit(const some &) = 0;
  virtual void visit(const array &) = 0;
  virtual void visit(const merge &) = 0;
  virtual void visit(const cat &) = 0;
  virtual void visit(const substr &) = 0;
  virtual void visit(const membership &) = 0;
  virtual void visit(const var &) = 0;
  virtual void visit(const missing &) = 0;
  virtual void visit(const missing_some &) = 0;
  virtual void visit(const log &) = 0;

  // control structure
  virtual void visit(const if_expr &) = 0;

  // values
  virtual void visit(const value_base &) = 0;
  virtual void visit(const null_value &) = 0;
  virtual void visit(const bool_value &) = 0;
  virtual void visit(const int_value &) = 0;
  virtual void visit(const unsigned_int_value &) = 0;
  virtual void visit(const real_value &) = 0;
  virtual void visit(const string_value &) = 0;
  virtual void visit(const array_value &) = 0;
  //~ virtual void visit(const array_view &) = 0;
  virtual void visit(const object_value &) = 0;

  virtual void visit(const error &) = 0;

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
  // extensions
  virtual void visit(const regex_match &) = 0;
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */
};

/// \private
template <class ast_functor, class ast_base, class argument_types,
          class result_type>
struct generic_dispatcher : visitor {
  generic_dispatcher(ast_functor astfn, argument_types fnargs)
      : fn(std::move(astfn)), args(std::move(fnargs)), res() {}

  // dummy apply for nodes that are not a sub-class of ast_base
  result_type apply(expr &, const void *) { return result_type{}; }

  template <class ast_node, class... argument, size_t... I>
  result_type apply_internal(ast_node &n, std::tuple<argument...> &&args,
                             std::index_sequence<I...>) {
    return fn(n, std::move(std::get<I>(args))...);
  }

  template <class ast_node>
  result_type apply(ast_node &n, const ast_base *) {
    return apply_internal(
        n, std::move(args),
        std::make_index_sequence<std::tuple_size<argument_types>::value>());
  }

  CXX_NORETURN void throw_unexpected_expr(const expr &);

  void visit(const expr &n) final { throw_unexpected_expr(n); }
  void visit(const oper &n) final { visit(static_cast<const expr &>(n)); }
  void visit(const equal &n) final { res = apply(n, &n); }
  void visit(const strict_equal &n) final { res = apply(n, &n); }
  void visit(const not_equal &n) final { res = apply(n, &n); }
  void visit(const strict_not_equal &n) final { res = apply(n, &n); }
  void visit(const less &n) final { res = apply(n, &n); }
  void visit(const greater &n) final { res = apply(n, &n); }
  void visit(const less_or_equal &n) final { res = apply(n, &n); }
  void visit(const greater_or_equal &n) final { res = apply(n, &n); }
  void visit(const logical_and &n) final { res = apply(n, &n); }
  void visit(const logical_or &n) final { res = apply(n, &n); }
  void visit(const logical_not &n) final { res = apply(n, &n); }
  void visit(const logical_not_not &n) final { res = apply(n, &n); }
  void visit(const add &n) final { res = apply(n, &n); }
  void visit(const subtract &n) final { res = apply(n, &n); }
  void visit(const multiply &n) final { res = apply(n, &n); }
  void visit(const divide &n) final { res = apply(n, &n); }
  void visit(const modulo &n) final { res = apply(n, &n); }
  void visit(const min &n) final { res = apply(n, &n); }
  void visit(const max &n) final { res = apply(n, &n); }
  void visit(const map &n) final { res = apply(n, &n); }
  void visit(const reduce &n) final { res = apply(n, &n); }
  void visit(const filter &n) final { res = apply(n, &n); }
  void visit(const all &n) final { res = apply(n, &n); }
  void visit(const none &n) final { res = apply(n, &n); }
  void visit(const some &n) final { res = apply(n, &n); }
  void visit(const array &n) final { res = apply(n, &n); }
  void visit(const merge &n) final { res = apply(n, &n); }
  void visit(const cat &n) final { res = apply(n, &n); }
  void visit(const substr &n) final { res = apply(n, &n); }
  void visit(const membership &n) final { res = apply(n, &n); }
  void visit(const var &n) final { res = apply(n, &n); }
  void visit(const missing &n) final { res = apply(n, &n); }
  void visit(const missing_some &n) final { res = apply(n, &n); }
  void visit(const log &n) final { res = apply(n, &n); }

  // control structure
  void visit(const if_expr &n) final { res = apply(n, &n); }

  // values
  void visit(const value_base &n) final { visit(static_cast<const expr &>(n)); }
  void visit(const null_value &n) final { res = apply(n, &n); }
  void visit(const bool_value &n) final { res = apply(n, &n); }
  void visit(const int_value &n) final { res = apply(n, &n); }
  void visit(const unsigned_int_value &n) final { res = apply(n, &n); }
  void visit(const real_value &n) final { res = apply(n, &n); }
  void visit(const string_value &n) final { res = apply(n, &n); }
  void visit(const array_value &n) final { res = apply(n, &n); }
  //~ void visit(const array_view &n) final { res = apply(n, &n); }
  void visit(const object_value &n) final { res = apply(n, &n); }

  void visit(const error &n) final { res = apply(n, &n); }

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
  // extensions
  void visit(const regex_match &n) final { res = apply(n, &n); }
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */

  result_type result() && { return std::move(res); }

 private:
  ast_functor fn;
  argument_types args;
  result_type res;
};

template <class ast_functor, class ast_base, class argument_types,
          class result_type>
void generic_dispatcher<ast_functor, ast_base, argument_types,
                        result_type>::throw_unexpected_expr(const expr &) {
  throw std::logic_error("unexpected Ast type.");
}

//~ template <class ast_functor, class... arguments>
template <class ast_functor, class ast_node, class... arguments>
auto generic_visit(ast_functor fn, ast_node *n, arguments... args)
    -> decltype(fn(*n, args...)) {
  using argument_types = std::tuple<arguments...>;
  using result_type = decltype(fn(*n, args...));
  using dispatcher_type =
      generic_dispatcher<ast_functor, ast_node, argument_types, result_type>;

  dispatcher_type disp{std::move(fn), std::make_tuple(std::forward(args)...)};

  n->accept(disp);
  return std::move(disp).result();
}


//
// logic internal data

/// type for string storage
/// \note if the compiler uses small string optimization, we cannot
///       use an std::vector because resizing invalidates the
///       string_views.
///       alternatives include forward_list or deque
using string_table_base = std::set<std::string>;

/// string table ensures lifetime of string exceeds lifetime of string_views
struct string_table : private string_table_base
{
    using string_table_base::size;

    string_table()                               = default;
    ~string_table()                              = default;
    string_table(string_table&&)                 = default;
    string_table& operator=(string_table&&)      = default;

    std::string_view safe_string(std::string v)
    {
      auto emplaced = this->emplace(std::move(v));

      std::string_view res = *emplaced.first;
      return res;
    }

    template <class iterator>
    std::string_view safe_string(iterator aa, iterator zz)
    {
      return safe_string(std::string(aa, zz));
    }

  private:
    string_table(const string_table&)            = delete;
    string_table& operator=(const string_table&) = delete;
};


using logic_data_base = std::tuple<any_expr, std::vector<std::string_view>, string_table, bool>;
struct logic_data : logic_data_base
{
  using base = logic_data_base;
  using base::base;

  /// returns the logic expression
  any_expr const &syntax_tree() const { return std::get<0>(*this); }

  /// returns static variable names (i.e., variable names that are not computed)
  std::vector<std::string_view> const &variable_names() const { return std::get<1>(*this); }

  /// returns the string table
  string_table&       strings()       { return std::get<2>(*this); }
  string_table const& strings() const { return std::get<2>(*this); }

  /// returns if the expression contains computed names.
  bool has_computed_variable_names() const { return std::get<3>(*this); }
};





}  // namespace jsonlogic
