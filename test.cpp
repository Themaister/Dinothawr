#include "blit.hpp"
#include <iostream>

int main()
{
   Blit::Rect a{{10, 20}, 10, 5};
   Blit::Rect b{{19, 20}, 10, 5};
   auto rect = a & b;
   if (rect)
   {
      std::cout << "True" << std::endl;
      std::cout << "X: " << rect.pos.x << " Y: " << rect.pos.y << std::endl;
      std::cout << "W: " << rect.w << " H: " << rect.h << std::endl;
   }
   else
      std::cout << "False" << std::endl;
}

