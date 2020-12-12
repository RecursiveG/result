#include "result.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cinttypes>
#include <memory>
#include <stdexcept>
#include <string>

using std::string;
using ::testing::Return;
using ::testing::InvokeWithoutArgs;

// Test basic handling of an OK value
TEST(ResultTest, ReturnVal) {
    auto func_return_int = []()->Result<int, ResultVoid>{ return 42; };
    auto func_return_str = []()->Result<string, ResultVoid>{ return "abc"; };
    auto i = func_return_int();
    auto s = func_return_str();
    ASSERT_TRUE(i.Ok());
    EXPECT_EQ(i.Value(), 42);
    ASSERT_TRUE(s.Ok());
    EXPECT_EQ(s.Value(), "abc");
}

// Test basic handling of an ERR value
TEST(ResultTest, ReturnErr) {
    auto func_return_int_err = []()->Result<ResultVoid, int>{ return Err(42); };
    auto func_return_str_err = []()->Result<ResultVoid, string>{ return Err("foo"); };
    auto i = func_return_int_err();
    auto s = func_return_str_err();
    ASSERT_FALSE(i.Ok());
    EXPECT_EQ(i.Error(), 42);
    ASSERT_FALSE(s.Ok());
    EXPECT_EQ(s.Error(), "foo");
}

// Test when result type is the same as error type
TEST(ResultTest, SameTandE) {
    Result<int, int> ok {42};
    Result<int, int> err {Err(42)};
    ASSERT_TRUE(ok.Ok());
    EXPECT_EQ(ok.Value(), 42);
    ASSERT_FALSE(err.Ok());
    EXPECT_EQ(err.Error(), 42);
}

// Can we raise a Result<A,E> in a function returning Result<A',E> ?
TEST(ResultTest, ChangeValueType) {
    auto func_change_value_type = [](Result<int, string> r)->Result<ResultVoid, string> {
        ASSIGN_OR_RAISE(auto x [[maybe_unused]], std::move(r));
        static_assert(std::is_same<decltype(x), int>::value);
        return {};
    };
    auto x = func_change_value_type(Err("bar"));
    static_assert(std::is_same<decltype(x), Result<ResultVoid, string>>::value);
    ASSERT_FALSE(x.Ok());
    EXPECT_EQ(x.Error(), "bar");
}

// Can we handle move-only types?
TEST(ResultTest, ReturnUniquePtrValue) {
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

TEST(ResultTest, ReturnUniquePtrError) {
    uint8_t *addr = nullptr;
    auto func_allocate = [&addr]()->Result<ResultVoid, std::unique_ptr<uint8_t[]>> {
        auto p = std::make_unique<uint8_t[]>(4096);
        addr = p.get();
        return Err(p);
    };
    auto x = func_allocate().TakeError().e;
    static_assert(std::is_same<decltype(x), std::unique_ptr<uint8_t[]>>::value);
    EXPECT_NE(addr, nullptr);
    EXPECT_EQ(x.get(), addr);
}

TEST(ResultTest, CnstructorForwarding) {
    auto func_return_vector = []()->Result<std::string, ResultVoid> {
        return {"\0\0\0", 3};
    };
    EXPECT_EQ(func_return_vector(), (std::string{"\0\0\0", 3}));
}

TEST(ResultTest, EqualityCheckInt) {
#define EXPECT_EQ_COMM(x,y) {EXPECT_EQ(x,y); EXPECT_EQ(y,x);}
#define EXPECT_NE_COMM(x,y) {EXPECT_NE(x,y); EXPECT_NE(y,x);}

    Result<int,int> ok = 42;
    Result<int,int> err = Err(43);

    EXPECT_EQ(ok, ok);
    EXPECT_EQ(err, err);
    EXPECT_NE(err, ok);
    EXPECT_NE(ok, err);

    EXPECT_EQ_COMM(ok, 42);
    EXPECT_NE_COMM(ok, 43);
    EXPECT_NE_COMM(ok, Err(42));

    EXPECT_EQ_COMM(err, Err(43));
    EXPECT_NE_COMM(err, Err(42));
    EXPECT_NE_COMM(err, 43);
}

TEST(ResultTest, EqualityCheckString) {
    Result<string,string> ok = "42";
    Result<string,string> err = Err("43");

    EXPECT_EQ(ok, ok);
    EXPECT_EQ(err, err);
    EXPECT_NE(err, ok);
    EXPECT_NE(ok, err);

    EXPECT_EQ_COMM(ok, "42");
    EXPECT_NE_COMM(ok, "43");
    EXPECT_NE_COMM(ok, Err("42"));

    EXPECT_EQ_COMM(err, Err("43"));
    EXPECT_NE_COMM(err, Err("42"));
    EXPECT_NE_COMM(err, "43");
}

// --------- MACRO tests --------- //
TEST(ResultTest, MacroAssignOrRaise) {
    auto func_plus_one = [](Result<int, string> in) -> Result<int, string> {
        ASSIGN_OR_RAISE(int x, std::move(in));
        return x + 1;
    };
    auto x = func_plus_one(5);
    ASSERT_TRUE(x.Ok());
    EXPECT_EQ(x.Value(), 6);

    auto y = func_plus_one(Err("boo"));
    ASSERT_FALSE(y.Ok());
    EXPECT_EQ(y.Error(), "boo");
}

TEST(ResultTest, MacroNotNullOrRaise) {
    auto func_deref = [](int*p)->Result<int, string> {
        return *NOT_NULL_OR_RAISE(p, "nullptr");
    };
    int i = 42;
    EXPECT_EQ(func_deref(&i), 42);
    EXPECT_EQ(func_deref(nullptr), Err("nullptr"));
}

TEST(ResultTest, MacroOptionalOrRaise) {
    auto func_deref = [](std::optional<int> o)->Result<int, string> {
        return OPTIONAL_OR_RAISE(o, "empty optional");
    };
    EXPECT_EQ(func_deref(42), 42);
    EXPECT_EQ(func_deref({}), Err("empty optional"));
}

TEST(ResultTest, MacroRaiseErrno) {
    auto func_raise = [](const char* msg)->Result<ResultVoid, string> {
        RAISE_ERRNO(msg);
    };
    errno = 0;
    EXPECT_EQ(func_raise("this is msg"), Err("this is msg (errno=0, Success)"));
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
    ASSERT_TRUE(x.Ok());
    EXPECT_EQ(x.Value().val_, 43);
}

TEST(ResultTest, NoExtraCopyForError) {
    auto func_create_bomb = []()->Result<ResultVoid, CopyBomb> {return Err(42);};
    auto func_plus_one = [](Result<ResultVoid, CopyBomb> r) -> Result<ResultVoid, CopyBomb> {
        r.Error().val_ ++;
        ASSIGN_OR_RAISE(auto b [[maybe_unused]], std::move(r));
        return {};
    };
    auto x = func_plus_one(func_create_bomb());
    ASSERT_FALSE(x.Ok());
    EXPECT_EQ(x.Error().val_, 43);
}

// --------- Pointer covariant test --------- //
// Ideally:
// Result<Base, X> ==> Result<Derived, X> for all X
// Result<X, Base> ==> Result<X, Derived> for all X

struct Base {
    virtual ~Base() = default;
};
struct Derived : public Base {
    Derived(int x) : x_(x){};
    int x_;
};

TEST(ResultTest, ValuePointerCovariant) {
    auto func_covariant =
        [](Result<std::unique_ptr<Derived>, string> r) -> Result<std::unique_ptr<Base>, string> {
            return r;
        };
    auto r = func_covariant(std::make_unique<Derived>(42));
    ASSERT_TRUE(r.Ok());
    auto ptr_base = r.Value().get();
    static_assert(std::is_same<decltype(ptr_base), Base*>::value);
    auto ptr_derive = dynamic_cast<Derived*>(ptr_base);
    ASSERT_NE(ptr_derive, nullptr);
    EXPECT_EQ(ptr_derive->x_, 42);

    auto y = func_covariant(Err("bbbb"));
    ASSERT_FALSE(y.Ok());
    EXPECT_EQ(y.Error(), "bbbb");
}

TEST(ResultTest, ErrorPointerCovariant) {
    auto func_covariant =
        [](Result<string, std::unique_ptr<Derived>> r) -> Result<string, std::unique_ptr<Base>> {
            return r;
        };
    auto r = func_covariant(Err(std::make_unique<Derived>(42)));
    ASSERT_FALSE(r.Ok());
    auto ptr_base = r.Error().get();
    static_assert(std::is_same<decltype(ptr_base), Base*>::value);
    auto ptr_derive = dynamic_cast<Derived*>(ptr_base);
    ASSERT_NE(ptr_derive, nullptr);
    EXPECT_EQ(ptr_derive->x_, 42);

    auto y = func_covariant("bbbc");
    ASSERT_TRUE(y.Ok());
    EXPECT_EQ(y.Value(), "bbbc");
}

// --------- functional --------- //

template<typename T, typename R> class MockCallable {
public:
    MOCK_METHOD(R, call, (T), (const));
    R operator()(T t) const {return call(t);}
};

TEST(ResultTest, FmapValue) {
    MockCallable<int, int> lambda;
    EXPECT_CALL(lambda, call(42)).WillOnce(Return(66));

    auto x = Result<int, string>{42}.Fmap(lambda);
    ASSERT_TRUE(x);
    EXPECT_EQ(x.Value(), 66);
}

TEST(ResultTest, FmapError) {
    MockCallable<int, int> lambda;
    auto x = Result<int, string>{Err("bbz")}.Fmap(lambda);
    ASSERT_FALSE(x);
    EXPECT_EQ(x.Error(), "bbz");
}

TEST(ResultTest, BindOnValue) {
    MockCallable<int, Result<int, string>> lambda;

    EXPECT_CALL(lambda, call(42)).WillOnce(InvokeWithoutArgs([]()->Result<int, string>{
        return 32;
    }));
    auto x = Result<int, string>{42}.Bind(lambda);
    ASSERT_TRUE(x);
    EXPECT_EQ(x.Value(), 32);

    EXPECT_CALL(lambda, call(11)).WillOnce(InvokeWithoutArgs([]()->Result<int, string>{
        return Err("err");
    }));
    auto y = Result<int, string>{11}.Bind(lambda);
    ASSERT_FALSE(y);
    EXPECT_EQ(y.Error(), "err");
}

TEST(ResultTest, BindOnError) {
    MockCallable<int, Result<int, string>> lambda;
    auto x = Result<int, string>{Err("err")}.Bind(lambda);
    ASSERT_FALSE(x);
    EXPECT_EQ(x.Error(), "err");
}

TEST(ResultTest, BindOnErrorCovariant) {
    MockCallable<int, Result<int, std::unique_ptr<Base>>> lambda;
    auto ptr = std::make_unique<Derived>(36);
    Derived* addr = ptr.get();

    auto x = Result<int, std::unique_ptr<Derived>>{Err(ptr)}.Bind(lambda);
    static_assert(std::is_same<decltype(x), Result<int, std::unique_ptr<Base>>>::value);
    ASSERT_FALSE(x);
    EXPECT_EQ(x.Error().get(), addr);
}
