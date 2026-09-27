#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <type_traits>
#include <new>

template <typename T>
class SCSPQ
{
    static constexpr std::size_t kCacheLineSize = 64;

    const std::size_t capacity_;
    std::unique_ptr<T[]> buffer_;
    alignas(kCacheLineSize) std::atomic<std::size_t> head_{};
    alignas(kCacheLineSize) std::atomic<std::size_t> tail_{};

public:
    explicit SCSPQ(std::size_t capacity) : capacity_(capacity), buffer_(capacity ? std::make_unique<T[]>(capacity) : nullptr) {}

    SCSPQ(const SCSPQ &) = delete;
    SCSPQ &operator=(const SCSPQ &) = delete;
    SCSPQ(SCSPQ &&) = delete;
    SCSPQ &operator=(SCSPQ &&) = delete;
    ~SCSPQ() = default;

    [[nodiscard]] bool push(const T &value) noexcept(std::is_nothrow_copy_assignable_v<T>)
    {
        if (capacity_ == 0)
            return false;

        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        const std::size_t head = head_.load(std::memory_order_acquire);

        if (tail - head >= capacity_)
            return false;

        buffer_[tail % capacity_] = value;
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool push(T &&value) noexcept(std::is_nothrow_move_assignable_v<T>)
    {
        if (capacity_ == 0)
            return false;

        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        const std::size_t head = head_.load(std::memory_order_acquire);

        if (tail - head >= capacity_)
            return false;

        buffer_[tail % capacity_] = std::move(value);
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool pop(T &result) noexcept((std::is_move_assignable_v<T> && std::is_nothrow_move_assignable_v<T>) ||
                                               (std::is_copy_assignable_v<T> && std::is_nothrow_copy_assignable_v<T>))
    {
        if (capacity_ == 0)
            return false;

        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t tail = tail_.load(std::memory_order_acquire);

        if (head == tail)
            return false;

        if constexpr (std::is_move_assignable_v<T>)
            result = std::move(buffer_[head % capacity_]);
        else
            result = buffer_[head % capacity_];

        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() noexcept((std::is_move_constructible_v<T> && std::is_nothrow_move_constructible_v<T>) ||
                                    (std::is_copy_constructible_v<T> && std::is_nothrow_copy_constructible_v<T>))
    {
        if (capacity_ == 0)
            return std::nullopt;

        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t tail = tail_.load(std::memory_order_acquire);

        if (head == tail)
            return std::nullopt;

        if constexpr (std::is_move_constructible_v<T>)
        {
            std::optional<T> result{std::move(buffer_[head % capacity_])};
            head_.store(head + 1, std::memory_order_release);
            return result;
        }
        else
        {
            std::optional<T> result{buffer_[head % capacity_]};
            head_.store(head + 1, std::memory_order_release);
            return result;
        }
    }
    void clear() noexcept { head_.store(tail_.load(std::memory_order_acquire), std::memory_order_release); }
    std::size_t size() const noexcept { return tail_.load(std::memory_order_acquire) - head_.load(std::memory_order_acquire); }
    std::size_t capacity() const noexcept { return capacity_; }
    bool empty() const noexcept { return size() == 0; }
    bool full() const noexcept { return size() >= capacity_; }
};