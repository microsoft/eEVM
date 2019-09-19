#include <intx/intx.hpp>
#include <iomanip>
#include <iostream>

inline auto print_fixed(const intx::uint256& v)
{
  std::cout << "0x" << std::hex << std::setw(40) << std::setfill('0')
            << intx::hex(v) << std::endl;
}

int main(int argc, char**)
{
  constexpr auto a = intx::from_string<intx::uint256>("0x542");
  constexpr auto b = intx::uint256{5, 0xa};
  constexpr auto c = intx::uint512{intx::uint256{0xa, 0xb}, 0x4020};
  std::cout << intx::hex(a) << std::endl;
  print_fixed(a);
  std::cout << intx::to_string(a) << std::endl;
  std::cout << intx::hex(b) << std::endl;
  std::cout << intx::to_string(b) << std::endl;
  std::cout << intx::hex(c) << std::endl;
  std::cout << intx::to_string(c) << std::endl;

  return 0;
}
