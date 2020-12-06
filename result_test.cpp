#include "result.h"

#include <gtest/gtest.h>

#include <cinttypes>
#include <memory>
#include <stdexcept>
#include <string>

using std::string;

// Test basic handling of an OK value
TEST(ResultTest, ReturnVal) {
    auto func_return_int = []()->Result<int, ResultVoid>{ return 42; };
    auto func_return_str = []()->Result<string, ResultVoid>{ return "abc"; };
    auto i = func_return_int();
    auto s = func_return_str();
    EXPECT_TRUE(i.Ok());
    EXPECT_EQ(i.Value(), 42);
    EXPECT_TRUE(s.Ok());
    EXPECT_EQ(s.Value(), "abc");
}

// Test basic handling of an ERR value
TEST(ResultTest, ReturnErr) {
    auto func_return_int_err = []()->Result<ResultVoid, int>{ return Err(42); };
    auto func_return_str_err = []()->Result<ResultVoid, string>{ return Err("foo"); };
    auto i = func_return_int_err();
    auto s = func_return_str_err();
    EXPECT_FALSE(i.Ok());
    EXPECT_EQ(i.Err(), 42);
    EXPECT_FALSE(s.Ok());
    EXPECT_EQ(s.Err(), "foo");
}

// Test when result type is the same as error type
TEST(ResultTest, SameTandE) {
    Result<int, int> ok {42};
    Result<int, int> err {Err(42)};
    EXPECT_TRUE(ok.Ok());
    EXPECT_EQ(ok.Value(), 42);
    EXPECT_FALSE(err.Ok());
    EXPECT_EQ(err.Err(), 42);
}

// Can we raise a Result<A,E> in a function returning Result<A',E> ?
TEST(ResultTest, ChangeValueType) {
    auto func_change_value_type = [](Result<int, string> r)->Result<ResultVoid, string> {
        ASSIGN_OR_RAISE(auto x [[maybe_unused]], std::move(r));
        static_assert(std::is_same<decltype(x), int>::value);
        return ResultVoid{};
    };
    auto x = func_change_value_type(Err("bar"));
    static_assert(std::is_same<decltype(x), Result<ResultVoid, string>>::value);
    EXPECT_FALSE(x.Ok());
    EXPECT_EQ(x.Err(), "bar");
}

TEST(ResultTest, PlusOne) {
    auto func_plus_one = [](Result<int, string> in) -> Result<int, string> {
        ASSIGN_OR_RAISE(int x, std::move(in));
        return x + 1;
    };
    auto x = func_plus_one(5);
    EXPECT_TRUE(x.Ok());
    EXPECT_EQ(x.Value(), 6);

    auto y = func_plus_one(Err("boo"));
    EXPECT_FALSE(y.Ok());
    EXPECT_EQ(y.Err(), "boo");
}

// Can we handle move-only types?
TEST(ResultTest, ReturnUniquePtr) {
    uint8_t *addr = nullptr;
    auto func_allocate = [&addr]()->Result<std::unique_ptr<uint8_t[]>, ResultVoid> {
        auto p = std::make_unique<uint8_t[]>(4096);
        addr = p.get();
        return p;
    };
    auto x = func_allocate().TakeValue();
    static_assert(std::is_same<decltype(x), std::unique_ptr<uint8_t[]>>::value);
    EXPECT_NE(addr, nullptr);
    EXPECT_EQ(x.get(), addr);
}

// --------- copy bomb test --------- //
// You cannot get it wrong so easily. //

// A class that throws an exception when copied.
// Try to catch unexpected copy operations.
class CopyBomb {
  public:
    CopyBomb() : val_(0){};
    CopyBomb(int val) : val_(val){};
    CopyBomb(const CopyBomb&) : val_(0) {
        throw std::logic_error("copy constructor bomb triggered");
    }
    CopyBomb& operator=(const CopyBomb&) {
        throw std::logic_error("copy assignment bomb triggered");
    }
    CopyBomb(CopyBomb&&) = default;
    CopyBomb& operator=(CopyBomb&&) = default;
    int val_;
};

TEST(ResultTest, NoExtraCopyForValue) {
    auto func_create_bomb = []()->Result<CopyBomb, ResultVoid> {return 42;};
    auto func_plus_one = [](Result<CopyBomb, ResultVoid> r) -> Result<CopyBomb, ResultVoid> {
        ASSIGN_OR_RAISE(auto b, std::move(r));
        b.val_++;
        return b;
    };
    auto x = func_plus_one(func_create_bomb());
    EXPECT_TRUE(x.Ok());
    EXPECT_EQ(x.Value().val_, 43);
}

TEST(ResultTest, NoExtraCopyForError) {
    auto func_create_bomb = []()->Result<ResultVoid, CopyBomb> {return Err(42);};
    auto func_plus_one = [](Result<ResultVoid, CopyBomb> r) -> Result<ResultVoid, CopyBomb> {
        r.Err().val_ ++;
        ASSIGN_OR_RAISE(auto b [[maybe_unused]], std::move(r));
        return ResultVoid{};
    };
    auto x = func_plus_one(func_create_bomb());
    EXPECT_FALSE(x.Ok());
    EXPECT_EQ(x.Err().val_, 43);
}
