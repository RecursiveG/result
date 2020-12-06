# Result<T, E>

Yet another `Result<T, E>` implementation for C++.

```cpp
Result<int, string> foo() {
    if (do_something_success()) {
        return 42;
    } else {
        return Err("error in foo");
    }
}

Result<string, string> bar() {
    auto x = VALUE_OR_RAISE(foo());
    if (do_more_success(x)) {
        return "bar success";
    } else {
        return Err("error in bar");
    }
}

void buz() {
    auto result = bar();
    if (result) {
        cout << "Success: " << result.Value() << endl;
    } else {
        cout << "Error location: " << result.Err() << endl;
    }
}

Result<type1, string> maybe_failure1(type0 x) {...}
Result<type2, string> maybe_failure2(type1 x) {...}
Result<type3, string> maybe_failure3(type2 x) {...}
Result<...> boo() {
    auto type3_val = VALUE_OR_RAISE(
        maybe_failure1(init_value)
            .bind(maybe_failure2)
            .bind(maybe_failure3)
    );
}
```
