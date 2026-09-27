#include <gtest/gtest.h>
#include <ranges>
#include <thread>
#include <memory>
#include <type_traits>
#include "SCSPQ.hpp"

struct NothrowCopyOnly
{
    NothrowCopyOnly() = default;
    NothrowCopyOnly(const NothrowCopyOnly &) noexcept = default;
    NothrowCopyOnly &operator=(const NothrowCopyOnly &) noexcept { return *this; }
    NothrowCopyOnly(NothrowCopyOnly &&) = delete;
    NothrowCopyOnly &operator=(NothrowCopyOnly &&) = delete;
};

struct ThrowingCopyOnly
{
    ThrowingCopyOnly() = default;
    ThrowingCopyOnly(const ThrowingCopyOnly &) noexcept(false) {}
    ThrowingCopyOnly &operator=(const ThrowingCopyOnly &) noexcept(false) { return *this; }
    ThrowingCopyOnly(ThrowingCopyOnly &&) = delete;
    ThrowingCopyOnly &operator=(ThrowingCopyOnly &&) = delete;
};

struct NothrowMoveOnly
{
    NothrowMoveOnly() = default;
    NothrowMoveOnly(const NothrowMoveOnly &) = delete;
    NothrowMoveOnly &operator=(const NothrowMoveOnly &) = delete;
    NothrowMoveOnly(NothrowMoveOnly &&) noexcept = default;
    NothrowMoveOnly &operator=(NothrowMoveOnly &&) noexcept { return *this; }
};

struct ThrowingMoveOnly
{
    ThrowingMoveOnly() = default;
    ThrowingMoveOnly(const ThrowingMoveOnly &) = delete;
    ThrowingMoveOnly &operator=(const ThrowingMoveOnly &) = delete;
    ThrowingMoveOnly(ThrowingMoveOnly &&) noexcept(false) {}
    ThrowingMoveOnly &operator=(ThrowingMoveOnly &&) noexcept(false) { return *this; }
};

TEST(SCSPQ, checkEmpty)
{
    SCSPQ<int> queue(1);
    EXPECT_TRUE(queue.empty());
}

TEST(SCSPQ, checkFull)
{
    SCSPQ<int> queue(10);
    for (auto &&val : std::views::iota(0, 10))
        ASSERT_TRUE(queue.push(val));
    EXPECT_FALSE(queue.push(1));
    EXPECT_TRUE(queue.full());
}

TEST(SCSPQ, checkSize)
{
    SCSPQ<int> queue(10);
    for (auto &&val : std::views::iota(0, 5))
        ASSERT_TRUE(queue.push(val));
    EXPECT_EQ(queue.size(), 5);
}

TEST(SCSPQ, checkClear)
{
    SCSPQ<int> queue(10);
    for (auto &&val : std::views::iota(0, 5))
        ASSERT_TRUE(queue.push(val));
    EXPECT_EQ(queue.size(), 5);
    queue.clear();
    EXPECT_TRUE(queue.empty());
}

TEST(SCSPQ, checkDataFlow)
{
    SCSPQ<int> queue(10);

    for (auto &&val : std::views::iota(0, 10))
        ASSERT_TRUE(queue.push(val));
    for (int res{}; auto &&val : std::views::iota(0, 10))
    {
        ASSERT_TRUE(queue.pop(res));
        ASSERT_EQ(res, val);
    }

    for (auto &&val : std::views::iota(0, 10))
        ASSERT_TRUE(queue.push(val));
    for (auto &&val : std::views::iota(0, 10))
    {
        auto res = queue.pop();
        ASSERT_TRUE(res);
        ASSERT_EQ(res, val);
    }
}

TEST(SCSPQ, checkWrapAround)
{
    SCSPQ<int> queue(10);
    for (size_t i = 0; i < 100; i++)
    {
        for (auto &&val : std::views::iota(0, 10))
            ASSERT_TRUE(queue.push(val));

        int retValue;
        for (auto &&val : std::views::iota(0, 10))
        {
            ASSERT_TRUE(queue.pop(retValue));
            ASSERT_EQ(retValue, val);
        }
    }
}

TEST(SCSPQ, checkCopy)
{
    SCSPQ<NothrowCopyOnly> queue(10);
    NothrowCopyOnly val{};
    EXPECT_TRUE(queue.push(val));
    EXPECT_TRUE(queue.push(val));
    EXPECT_TRUE(queue.pop(val));
    EXPECT_TRUE(queue.pop());
    static_assert(noexcept(std::declval<SCSPQ<NothrowCopyOnly> &>().push(std::declval<const NothrowCopyOnly &>())));
    static_assert(!noexcept(std::declval<SCSPQ<ThrowingCopyOnly> &>().push(std::declval<const ThrowingCopyOnly &>())));
    static_assert(noexcept(std::declval<SCSPQ<NothrowCopyOnly> &>().pop(std::declval<NothrowCopyOnly &>())));
    static_assert(!noexcept(std::declval<SCSPQ<ThrowingCopyOnly> &>().pop(std::declval<ThrowingCopyOnly &>())));
    static_assert(noexcept(std::declval<SCSPQ<NothrowCopyOnly> &>().pop()));
    static_assert(!noexcept(std::declval<SCSPQ<ThrowingCopyOnly> &>().pop()));
}

TEST(SCSPQ, checkMove)
{
    SCSPQ<NothrowMoveOnly> queue(10);
    NothrowMoveOnly val{};
    EXPECT_TRUE(queue.push(NothrowMoveOnly{}));
    EXPECT_TRUE(queue.push(NothrowMoveOnly{}));
    EXPECT_TRUE(queue.pop(val));
    EXPECT_TRUE(queue.pop());
    static_assert(noexcept(std::declval<SCSPQ<NothrowMoveOnly> &>().push(std::declval<NothrowMoveOnly &&>())));
    static_assert(!noexcept(std::declval<SCSPQ<ThrowingMoveOnly> &>().push(std::declval<ThrowingMoveOnly &&>())));
    static_assert(noexcept(std::declval<SCSPQ<NothrowMoveOnly> &>().pop(std::declval<NothrowMoveOnly &>())));
    static_assert(!noexcept(std::declval<SCSPQ<ThrowingMoveOnly> &>().pop(std::declval<ThrowingMoveOnly &>())));
    static_assert(noexcept(std::declval<SCSPQ<NothrowMoveOnly> &>().pop()));
    static_assert(!noexcept(std::declval<SCSPQ<ThrowingMoveOnly> &>().pop()));
}

TEST(SCSPQ, checkConcurency)
{
    size_t iterations = 10'000'000;
    SCSPQ<size_t> queue(20);
    std::jthread producer(
        [&queue, iterations]()
        {
            for (size_t i = 0; i < iterations; ++i)
                while (!queue.push(i))
                    ;
        });
    std::jthread consumer(
        [&queue, iterations]()
        {
            size_t res{};
            for (size_t i = 0; i < iterations; ++i)
            {
                while (!queue.pop(res))
                    ;

                ASSERT_EQ(res, i);
            }
        });
}