#include <dima/type.hpp>
#include <iostream>
#include <iterator>

class Expression : public dima::Type<Expression> {
  public:
    int x, y;
};

int main() {
    {
        dima::Array<Expression> arr = Expression::allocate_array(10);
        for (auto it = arr.begin(); it != arr.end(); ++it) {
            // I now have direct access to the slots through 'it'
            dima::Var<Expression> val = *it;
            val->x = std::distance(arr.begin(), it);
            val->y = std::distance(it, arr.end());
            std::cout << "dist: " << val->x << std::endl;
        }
        std::cout << "Capacity: " << Expression::get_capacity() << std::endl;
        std::cout << "Used: " << Expression::get_allocation_count() << std::endl;
        for (auto it = arr.begin(); it != arr.end(); ++it) {
            Expression *expr = (*it).get();
            std::cout << "Expr: (" << expr->x << ", " << expr->y << ")" << std::endl;
        }
    }
    std::cout << "Capacity: " << Expression::get_capacity() << std::endl;
    std::cout << "Used: " << Expression::get_allocation_count() << std::endl;
    return 0;
}
