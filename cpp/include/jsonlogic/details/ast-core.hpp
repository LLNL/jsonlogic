
#pragma once

#include <memory>

namespace jsonlogic {

struct visitor;

// the root class
struct expr {
  expr() = default;
  virtual ~expr() = default;

  virtual void accept(visitor &) = 0;

private:
  expr(expr &&) = delete;
  expr(const expr &) = delete;
  expr &operator=(expr &&) = delete;
  expr &operator=(const expr &) = delete;
};

using any_expr = std::unique_ptr<expr>;

std::ostream &operator<<(std::ostream &os, any_expr &n);

} // namespace jsonlogic
