#include <intx/intx.hpp>
#include <iostream>

int main(int argc, char**)
{
  constexpr auto a = intx::from_string<intx::uint256>("0x542");
  constexpr auto b = intx::uint256{5, 5};
  constexpr auto c = intx::uint512{5, 5};
  std::cout << intx::hex(a) << std::endl;
  std::cout << intx::to_string(a) << std::endl;
  std::cout << intx::hex(b) << std::endl;
  std::cout << intx::to_string(b) << std::endl;
  std::cout << intx::hex(c) << std::endl;
  std::cout << intx::to_string(c) << std::endl;

  return 0;
}
