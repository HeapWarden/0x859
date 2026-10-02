#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <type_traits>
#include <concepts>
#include <new>

template <typename T>
    requires(std::constructible_from<T, T &&> && std::assignable_from<T &, T &&>) ||
            (std::constructible_from<T, const T &> && std::assignable_from<T &, const T &>)
class SCSPQ
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winterference-size"
    static constexpr std::size_t kCacheLineSize = std::hardware_destructive_interference_size;
#pragma GCC diagnostic pop

    const std::size_t capacity_;
    std::allocator<T> allocator_;
    T *buffer_;
    alignas(kCacheLineSize) std::atomic<std::size_t> head_{};
    alignas(kCacheLineSize) std::atomic<std::size_t> tail_{};

public:
    explicit SCSPQ(std::size_t capacity) : capacity_(capacity), buffer_(capacity ? allocator_.allocate(capacity) : nullptr) {}
    ~SCSPQ() { allocator_.deallocate(buffer_, capacity_); }

    SCSPQ(const SCSPQ &) = delete;
    SCSPQ &operator=(const SCSPQ &) = delete;
    SCSPQ(SCSPQ &&) = delete;
    SCSPQ &operator=(SCSPQ &&) = delete;

    [[nodiscard]] bool push(const T &value) noexcept(std::is_nothrow_copy_constructible_v<T>)
    {
        if (capacity_ == 0)
            return false;

        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        const std::size_t head = head_.load(std::memory_order_acquire);

        if (tail - head >= capacity_)
            return false;

        std::construct_at(buffer_ + tail % capacity_, value);
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool push(T &&value) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        if (capacity_ == 0)
            return false;

        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        const std::size_t head = head_.load(std::memory_order_acquire);

        if (tail - head >= capacity_)
            return false;

        std::construct_at(buffer_ + tail % capacity_, std::move(value));
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool pop(T &result) noexcept(std::is_nothrow_assignable_v<T &, T &&> ||
                                               (!std::assignable_from<T &, T &&> && std::is_nothrow_assignable_v<T &, const T &>))
    {
        if (capacity_ == 0)
            return false;

        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t tail = tail_.load(std::memory_order_acquire);

        if (head == tail)
            return false;

        if constexpr (std::assignable_from<T &, T &&>)
            result = std::move(buffer_[head % capacity_]);
        else
            result = buffer_[head % capacity_];

        std::destroy_at(buffer_ + head % capacity_);
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] std::optional<T> pop() noexcept(std::is_nothrow_assignable_v<T &, T &&> ||
                                                  (!std::assignable_from<T &, T &&> && std::is_nothrow_assignable_v<T &, const T &>))
    {
        if (capacity_ == 0)
            return std::nullopt;

        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t tail = tail_.load(std::memory_order_acquire);

        if (head == tail)
            return std::nullopt;

        std::optional<T> result;
        if constexpr (std::constructible_from<T, T &&>)
            result.emplace(std::move(buffer_[head % capacity_]));
        else
            result.emplace(buffer_[head % capacity_]);

        std::destroy_at(buffer_ + head % capacity_);
        head_.store(head + 1, std::memory_order_release);
        return result;
    }
    void clear() noexcept { head_.store(tail_.load(std::memory_order_acquire), std::memory_order_release); }
    std::size_t size() const noexcept { return tail_.load(std::memory_order_acquire) - head_.load(std::memory_order_acquire); }
    std::size_t capacity() const noexcept { return capacity_; }
    bool empty() const noexcept { return size() == 0; }
    bool full() const noexcept { return size() >= capacity_; }
};