/// A Shared String class

#pragma once

#include <string>
#include <string_view>
#include <memory>

namespace jsonlogic
{
  using shared_string_ptr = std::shared_ptr<std::string>;

  struct managed_string_view : private shared_string_ptr, public std::string_view
  {
      static
      std::string_view to_string_view(shared_string_ptr p) { return *p; }

      using holder    = shared_string_ptr;
      using base      = std::string_view;
      using size_type = std::string_view::size_type;

      ~managed_string_view()                                     = default;
      managed_string_view(const managed_string_view&)            = default;
      managed_string_view(managed_string_view&&)                 = default;
      managed_string_view& operator=(const managed_string_view&) = default;
      managed_string_view& operator=(managed_string_view&&)      = default;

      explicit
      managed_string_view(std::string_view view)
      : holder(std::make_shared<std::string>(view.begin(), view.end())), base(to_string_view(*this))
      {}

      // explicit
      managed_string_view(std::string&& s)
      : holder(std::make_shared<std::string>(std::move(s))), base(to_string_view(*this))
      {}

      template <class ForwardIterator>
      managed_string_view(ForwardIterator beg, std::size_t len)
      : holder(std::make_shared<std::string>(beg, len)), base(to_string_view(*this))
      {}


    private:

      managed_string_view(holder string_ptr, std::string_view view)
      : holder(std::move(string_ptr)), base(view)
      {}

    public:

      managed_string_view substr(size_type ofs = 0, size_type cnt = base::npos) const
      {
        return { holder(*this), base::substr(ofs, cnt) };
      }

      std::string_view view() const { return *this; }
  };

  // \todo replace with space ship operator
  inline bool operator==(const managed_string_view& lhs, const managed_string_view& rhs) { return lhs.view() == rhs.view(); }
  inline bool operator!=(const managed_string_view& lhs, const managed_string_view& rhs) { return lhs.view() != rhs.view(); }
  inline bool operator<(const managed_string_view& lhs, const managed_string_view& rhs)  { return lhs.view() < rhs.view(); }
  inline bool operator<=(const managed_string_view& lhs, const managed_string_view& rhs) { return lhs.view() <= rhs.view(); }
  inline bool operator>(const managed_string_view& lhs, const managed_string_view& rhs)  { return lhs.view() > rhs.view(); }
  inline bool operator>=(const managed_string_view& lhs, const managed_string_view& rhs) { return lhs.view() >= rhs.view(); }
}
