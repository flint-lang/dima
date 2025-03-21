# dima

The C++ library version of Flints Deterministic Incremental Memory Architecture

## usage

This is a header-only library and it can be used in any project, it requires `c++17` or newer. You can use this library by just downloading everything from the `dima` directory. It is recommended to add this library to your project as a git submodule via

```sh
git submodule add https://github.com/flint-lang/dima.git path/to/submodule
```

and to add the `-Ipath/to/submodule` flag to your C++ compile flags. This library is very straight forward to use:

### 1. Register the Head

You first need to register the DIMA Head. This acts as the "root" from which all Blocks branch out.

```cpp
#include <dima/head>

class YourType {
    int x;
    int y;

    YourType(int x, int y) : x(x), y(y) {}
}

static dima::Head<YourType> your_types;
```

### 2. Allocate things

And then, you only need to call `.allocate(...)` on your head variable to allocate a new variable of type `YourType`:

```cpp
int main() {
    Var<YourType> var = your_type.allocate(1, 2);
    return 0;
}
```

### 3. Use your variables

DIMA is ARC-managed behind the scenes. It aims to reduce scattering that happens quite often through C++'s `std::unique_ptr` and shared pointers in general, as every single one of them gets heap allocated, which makes memory very fragmented.

When you have allocated a variable, you can directly access the type `YourType` through the `->` operator. `Var` is only a RAII-based wrapper for the ARC-part of DIMA

```cpp
#include <iostream>

int main() {
    Var<YourType> var = your_type.allocate(1, 2);
    var->x = 5;
    var->y = 3;
    std::cout << "var: (" << var->x << ", " << var->y << ")" << std::endl;
    return 0;
}
```

The above code will simply print `var: (5, 3)` to the console. Notice how you cannot just change the whole `YourType` varible contained within `Var`? This is entirely intentional. Also, everything from DIMA is reference counted, so doing

```cpp
Var<YourVar> var2 = var;
```

will actually mean that both `var` and `var2` point to the same `dima::Slot`, so they point to the same `YourData` structure:

```cpp
int main() {
    Var<YourType> var = your_type.allocate(1, 2);
    Var<YourType> var2 = var;
    var2->y = 3;
    std::cout << "var: (" << var->x << ", " << var->y << ")" << std::endl;
    return 0;
}
```

This code will print `var: (1, 3)` to the console. If you truly want to create a new variable, you need to allocate it:

```cpp
int main() {
    Var<YourType> var = your_type.allocate(1, 2);
    Var<YourType> var2 = your_type.allocate(3, 4);
    var->x = 5;
    var2->y = 10;
    std::cout << "var: (" << var->x << ", " << var->y << ")" << std::endl;
    std::cout << "var2: (" << var2->y << ", " << var2->y << ")" << std::endl;
    return 0;
}
```

## Additional functionality

That was basically it.. Thats how you can use DIMA. But there actually exists quite a bit of additional functionality in DIMA, or powered by DIMA.

First, you can check at any time how many slots are occupied from a given type:

```cpp
your_type.get_allocation_count()
```

This returns a `size_t` value of how many slots are occupied. There also exist the `get_free_count` and `get_capacity` functions on heads to check how many slots are free and how many space for slots there is, both of which return a `size_t` value.
