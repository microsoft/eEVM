#include <intx/intx.hpp>
#include <iomanip>
#include <iostream>

namespace intx
{
  template <unsigned N>
  std::ostream& operator<<(std::ostream& o, const uint<N>& n)
  {
    const auto fmt_flags = o.flags();
    const auto basefield = fmt_flags & std::ostream::basefield;
    const auto showbase = fmt_flags & std::ostream::showbase;

    switch (basefield)
    {
      case (std::ostream::hex):
      {
        if (showbase)
        {
          o << "0x";
        }
        o << to_string(n, 16);
        break;
      }

      case (std::ostream::oct):
      {
        if (showbase)
        {
          o << "0";
        }
        o << to_string(n, 8);
        break;
      }

      default:
      {
        o << to_string(n, 10);
        break;
      }
    }
    return o;
  }
}

int main(int argc, char**)
{
  constexpr auto a = intx::from_string<intx::uint256>("0x542");
  constexpr auto b = intx::uint256{5, 0xa};
  constexpr auto c = intx::uint512{intx::uint256{0xa, 0xb}, 0x4020};

  std::cout << std::dec << a << std::endl;
  std::cout << std::dec << b << std::endl;
  std::cout << std::dec << c << std::endl;

  std::cout << std::hex << a << std::endl;
  std::cout << std::hex << b << std::endl;
  std::cout << std::hex << c << std::endl;

  std::cout << std::oct << a << std::endl;
  std::cout << std::oct << b << std::endl;
  std::cout << std::oct << c << std::endl;

  std::cout << std::showbase << std::dec << a << std::endl;
  std::cout << std::showbase << std::hex << a << std::endl;
  std::cout << std::showbase << std::oct << a << std::endl;

  return 0;
}
