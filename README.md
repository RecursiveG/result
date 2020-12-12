# Result<T, E>

Yet another `Result<T, E>` implementation for C++.

You can include this in CMake:
```
include(FetchContent)
FetchContent_Declare(result
    GIT_REPOSITORY https://github.com/RecursiveG/result.git
    GIT_TAG        084c3106db35ac25ce0849d974bde42cacb89c09
)
FetchContent_MakeAvailable(result)

add_executable(main main.cpp)
target_link_libraries(main result)
```

`main.cpp` example:
```cpp
#include <iostream>
#include <string>
#include <cmath>
#include <stdlib.h>

#include "result.h"

Result<long, std::string> ToNumber(const char* s) {
    char* endptr = nullptr;
    long ret = strtol(s, &endptr, 0);
    if (endptr[0] != '\0') {
        return Err(std::string("Cannot convert ") + s + " to a number.");
    } else {
        return ret;
    }
}

Result<double, std::string> Sqrt(double d) {
    if (d < 0) return Err("Cannot take square root of a negative number.");
    return sqrt(d);
}

double Inc(double x) {
    return x+1;
}

Result<double, std::string> Example1(const char* s) {
    ASSIGN_OR_RAISE(auto num, ToNumber(s));
    ASSIGN_OR_RAISE(auto sqrted, Sqrt(num));
    return Inc(sqrted);
}

Result<double, std::string> Example2(const char* s) {
    return ToNumber(s)
            .Bind(&Sqrt)
            .Fmap(&Inc);
}

int main(int argc, char* argv[]) {
    auto e1 = Example1(argv[1]);
    if (e1) {
        std::cout << "Example1: " << e1.Value() << std::endl;
    } else {
        std::cout << "Example1: " << e1.Error() << std::endl;
    }
    
    try {
        std::cout << "Example2: "
                  << Example2(argv[1]).Expect("Arithmetic error")
                  << std::endl;
    } catch (const std::runtime_error& ex) {
        std::cout << "Example2: " << ex.what() << std::endl;
    }
}
```
