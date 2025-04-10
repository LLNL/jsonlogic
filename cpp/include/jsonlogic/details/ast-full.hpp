#pragma once

#include <boost/json.hpp>

#include "ast-core.hpp"
#include "cxx-compat.hpp"

#if !defined(WITH_JSONLOGIC_EXTENSIONS)
#define WITH_JSONLOGIC_EXTENSIONS 1
#endif /* !defined(WITH_JSONLOGIC_EXTENSIONS) */

namespace jsonlogic {
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

  // convenience function so that the constructor does logical_not need to be
  // implemented membership every derived class.
  void set_operands(container_type &&opers) { this->swap(opers); }

  const container_type &operands() const { return *this; }
  container_type &operands() { return *this; }
  container_type &&move_operands() && { return std::move(*this); }

  expr &operand(int n) const;

  virtual int num_evaluated_operands() const;
};

// defines operators that have an upper bound on how many
//   arguments are evaluated.
template <int MaxArity> struct oper_n : oper {
  enum { MAX_OPERANDS = MaxArity };

  int num_evaluated_operands() const final;
};

struct value_base : expr {
  virtual boost::json::value to_json() const = 0;
};

template <class T> struct value_generic : value_base {
  using value_type = T;

  explicit value_generic(T t) : val(std::move(t)) {}

  T &value() { return val; }
  const T &value() const { return val; }

  boost::json::value to_json() const final;

private:
  T val;
};

//
// comparisons

// binary
struct equal : oper_n<2> {
  void accept(visitor &) final;
};

struct strict_equal : oper_n<2> {
  void accept(visitor &) final;
};

struct not_equal : oper_n<2> {
  void accept(visitor &) final;
};

struct strict_not_equal : oper_n<2> {
  void accept(visitor &) final;
};

// binary or ternary
struct less : oper_n<3> {
  void accept(visitor &) final;
};

struct greater : oper_n<3> {
  void accept(visitor &) final;
};

struct less_or_equal : oper_n<3> {
  void accept(visitor &) final;
};

struct greater_or_equal : oper_n<3> {
  void accept(visitor &) final;
};

// logical operators

// unary
struct logical_not : oper_n<1> {
  void accept(visitor &) final;
};

struct logical_not_not : oper_n<1> {
  void accept(visitor &) final;
};

// n-ary
struct logical_and : oper {
  void accept(visitor &) final;
};

struct logical_or : oper {
  void accept(visitor &) final;
};

// control structure
struct if_expr : oper {
  void accept(visitor &) final;
};

// n-ary arithmetic

struct add : oper {
  void accept(visitor &) final;
};

struct multiply : oper {
  void accept(visitor &) final;
};

struct min : oper {
  void accept(visitor &) final;
};

struct max : oper {
  void accept(visitor &) final;
};

// binary arithmetic

struct subtract : oper_n<2> {
  void accept(visitor &) final;
};

struct divide : oper_n<2> {
  void accept(visitor &) final;
};

struct modulo : oper_n<2> {
  void accept(visitor &) final;
};

// array

// arrays serve a dual purpose
//   they can be considered collections, but also an aggregate value.
// The class is final and it supports move ctor/assignment, so the data
//   can move efficiently.
struct array final : oper // array is modeled as operator
{
  void accept(visitor &) final;

  array() = default;

  array(array &&);
  array &operator=(array &&);
};

array::array(array &&other) : oper() {
  set_operands(std::move(other).move_operands());
}

array &array::operator=(array &&other) {
  set_operands(std::move(other).move_operands());
  return *this;
}

struct map : oper_n<2> {
  void accept(visitor &) final;
};

struct reduce : oper_n<3> {
  void accept(visitor &) final;
};

struct filter : oper_n<2> {
  void accept(visitor &) final;
};

struct all : oper_n<2> {
  void accept(visitor &) final;
};

struct none : oper_n<2> {
  void accept(visitor &) final;
};

struct some : oper_n<2> {
  void accept(visitor &) final;
};

struct merge : oper {
  void accept(visitor &) final;
};

// data access
struct var : oper {
  enum { computed = -1 };

  void accept(visitor &) final;

  void num(int val) { idx = val; }
  int num() const { return idx; }

private:
  int idx = computed;
};

/// missing is modeled as operator with arbitrary number of arguments
/// \details
/// membership Calculator::visit(missing&) :
///   if_expr the first argument is an array, only the array will be considered
///   otherwise all operands are treated as array.
struct missing : oper {
  void accept(visitor &) final;
};

struct missing_some : oper_n<2> {
  void accept(visitor &) final;
};

// string operations
struct cat : oper {
  void accept(visitor &) final;
};

struct substr : oper_n<3> {
  void accept(visitor &) final;
};

// string and array operation
struct membership : oper {
  void accept(visitor &) final;
};

// values
struct null_value : value_base {
  null_value() = default;
  null_value(std::nullptr_t) {}

  void accept(visitor &) final;

  std::nullptr_t value() const { return nullptr; }

  boost::json::value to_json() const final;
};

struct bool_value : value_generic<bool> {
  using base = value_generic<bool>;
  using base::base;

  void accept(visitor &) final;
};

struct int_value : value_generic<std::int64_t> {
  using base = value_generic<std::int64_t>;
  using base::base;

  void accept(visitor &) final;
};

struct unsigned_int_value : value_generic<std::uint64_t> {
  using base = value_generic<std::uint64_t>;
  using base::base;

  void accept(visitor &) final;
};

struct real_value : value_generic<double> {
  using base = value_generic<double>;
  using base::base;

  void accept(visitor &) final;
};

struct string_value : value_generic<boost::json::string> {
  using base = value_generic<boost::json::string>;
  using base::base;

  void accept(visitor &) final;
};

struct object_value : expr, private std::map<boost::json::string, any_expr> {
  using base = std::map<boost::json::string, any_expr>;
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

  void accept(visitor &) final;
};

// logger
struct log : oper_n<1> {
  void accept(visitor &) final;
};

// error node
struct error : expr {
  void accept(visitor &) final;
};

//
// jsonlogic extensions

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
struct regex_match : oper_n<2> {
  void accept(visitor &) final;
};
#endif /* WITH_JSON_LOGIC_CPP_EXTENSIONS */

// visitor
struct visitor {
  virtual void visit(expr &) = 0; // error
  virtual void visit(oper &n) = 0;
  virtual void visit(equal &) = 0;
  virtual void visit(strict_equal &) = 0;
  virtual void visit(not_equal &) = 0;
  virtual void visit(strict_not_equal &) = 0;
  virtual void visit(less &) = 0;
  virtual void visit(greater &) = 0;
  virtual void visit(less_or_equal &) = 0;
  virtual void visit(greater_or_equal &) = 0;
  virtual void visit(logical_and &) = 0;
  virtual void visit(logical_or &) = 0;
  virtual void visit(logical_not &) = 0;
  virtual void visit(logical_not_not &) = 0;
  virtual void visit(add &) = 0;
  virtual void visit(subtract &) = 0;
  virtual void visit(multiply &) = 0;
  virtual void visit(divide &) = 0;
  virtual void visit(modulo &) = 0;
  virtual void visit(min &) = 0;
  virtual void visit(max &) = 0;
  virtual void visit(map &) = 0;
  virtual void visit(reduce &) = 0;
  virtual void visit(filter &) = 0;
  virtual void visit(all &) = 0;
  virtual void visit(none &) = 0;
  virtual void visit(some &) = 0;
  virtual void visit(array &) = 0;
  virtual void visit(merge &) = 0;
  virtual void visit(cat &) = 0;
  virtual void visit(substr &) = 0;
  virtual void visit(membership &) = 0;
  virtual void visit(var &) = 0;
  virtual void visit(missing &) = 0;
  virtual void visit(missing_some &) = 0;
  virtual void visit(log &) = 0;

  // control structure
  virtual void visit(if_expr &) = 0;

  // values
  virtual void visit(value_base &) = 0;
  virtual void visit(null_value &) = 0;
  virtual void visit(bool_value &) = 0;
  virtual void visit(int_value &) = 0;
  virtual void visit(unsigned_int_value &) = 0;
  virtual void visit(real_value &) = 0;
  virtual void visit(string_value &) = 0;
  virtual void visit(object_value &) = 0;

  virtual void visit(error &) = 0;

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
  // extensions
  virtual void visit(regex_match &) = 0;
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

  template <class ast_node> result_type apply(ast_node &n, const ast_base *) {
    return apply_internal(
        n, std::move(args),
        std::make_index_sequence<std::tuple_size<argument_types>::value>());
  }

  CXX_NORETURN void throw_unexpected_expr(expr &);

  void visit(expr &n) final { throw_unexpected_expr(n); }
  void visit(oper &n) final { visit(static_cast<expr &>(n)); }
  void visit(equal &n) final { res = apply(n, &n); }
  void visit(strict_equal &n) final { res = apply(n, &n); }
  void visit(not_equal &n) final { res = apply(n, &n); }
  void visit(strict_not_equal &n) final { res = apply(n, &n); }
  void visit(less &n) final { res = apply(n, &n); }
  void visit(greater &n) final { res = apply(n, &n); }
  void visit(less_or_equal &n) final { res = apply(n, &n); }
  void visit(greater_or_equal &n) final { res = apply(n, &n); }
  void visit(logical_and &n) final { res = apply(n, &n); }
  void visit(logical_or &n) final { res = apply(n, &n); }
  void visit(logical_not &n) final { res = apply(n, &n); }
  void visit(logical_not_not &n) final { res = apply(n, &n); }
  void visit(add &n) final { res = apply(n, &n); }
  void visit(subtract &n) final { res = apply(n, &n); }
  void visit(multiply &n) final { res = apply(n, &n); }
  void visit(divide &n) final { res = apply(n, &n); }
  void visit(modulo &n) final { res = apply(n, &n); }
  void visit(min &n) final { res = apply(n, &n); }
  void visit(max &n) final { res = apply(n, &n); }
  void visit(map &n) final { res = apply(n, &n); }
  void visit(reduce &n) final { res = apply(n, &n); }
  void visit(filter &n) final { res = apply(n, &n); }
  void visit(all &n) final { res = apply(n, &n); }
  void visit(none &n) final { res = apply(n, &n); }
  void visit(some &n) final { res = apply(n, &n); }
  void visit(array &n) final { res = apply(n, &n); }
  void visit(merge &n) final { res = apply(n, &n); }
  void visit(cat &n) final { res = apply(n, &n); }
  void visit(substr &n) final { res = apply(n, &n); }
  void visit(membership &n) final { res = apply(n, &n); }
  void visit(var &n) final { res = apply(n, &n); }
  void visit(missing &n) final { res = apply(n, &n); }
  void visit(missing_some &n) final { res = apply(n, &n); }
  void visit(log &n) final { res = apply(n, &n); }

  // control structure
  void visit(if_expr &n) final { res = apply(n, &n); }

  // values
  void visit(value_base &n) final { visit(static_cast<expr &>(n)); }
  void visit(null_value &n) final { res = apply(n, &n); }
  void visit(bool_value &n) final { res = apply(n, &n); }
  void visit(int_value &n) final { res = apply(n, &n); }
  void visit(unsigned_int_value &n) final { res = apply(n, &n); }
  void visit(real_value &n) final { res = apply(n, &n); }
  void visit(string_value &n) final { res = apply(n, &n); }
  void visit(object_value &n) final { res = apply(n, &n); }

  void visit(error &n) final { res = apply(n, &n); }

#if WITH_JSON_LOGIC_CPP_EXTENSIONS
  // extensions
  void visit(regex_match &n) final { res = apply(n, &n); }
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
                        result_type>::throw_unexpected_expr(expr &) {
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
} // namespace jsonlogic
