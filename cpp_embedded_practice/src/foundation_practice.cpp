#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

enum class DeviceState {
    idle,
    running,
    fault
};

class Device {
public:
    bool start()
    {
        if (state_ != DeviceState::idle) {
            return false;
        }
        state_ = DeviceState::running;
        return true;
    }

    bool fail()
    {
        if (state_ == DeviceState::fault) {
            return false;
        }
        state_ = DeviceState::fault;
        return true;
    }

    bool reset()
    {
        if (state_ != DeviceState::fault) {
            return false;
        }
        state_ = DeviceState::idle;
        return true;
    }

    DeviceState state() const noexcept
    {
        return state_;
    }

private:
    DeviceState state_{DeviceState::idle};
};

class MovingAverage {
public:
    explicit MovingAverage(std::size_t window_size)
        : window_size_{window_size}
    {
        if (window_size_ == 0) {
            throw std::invalid_argument{"window size must be positive"};
        }
    }

    void add_sample(double value)
    {
        samples_.push_back(value);
        sum_ += value;

        if (samples_.size() > window_size_) {
            sum_ -= samples_.front();
            samples_.pop_front();
        }
    }

    std::optional<double> average() const
    {
        if (samples_.empty()) {
            return std::nullopt;
        }
        return sum_ / static_cast<double>(samples_.size());
    }

private:
    std::size_t window_size_;
    std::deque<double> samples_;
    double sum_{0.0};
};

struct TemperatureFrame {
    int channel;
    double celsius;
};

std::optional<int> parse_integer(std::string_view text)
{
    int value{};
    const auto result =
        std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} ||
        result.ptr != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

std::optional<TemperatureFrame> parse_frame(std::string_view line)
{
    if (!line.empty() && line.back() == '\n') {
        line.remove_suffix(1);
    }

    constexpr std::string_view prefix{"TEMP,"};
    if (line.substr(0, prefix.size()) != prefix) {
        return std::nullopt;
    }
    line.remove_prefix(prefix.size());

    const auto comma = line.find(',');
    if (comma == std::string_view::npos) {
        return std::nullopt;
    }

    const auto channel = parse_integer(line.substr(0, comma));
    const auto raw_value = parse_integer(line.substr(comma + 1));
    if (!channel || !raw_value) {
        return std::nullopt;
    }
    if (*channel < 0 || *channel > 7 ||
        *raw_value < -400 || *raw_value > 1250) {
        return std::nullopt;
    }

    return TemperatureFrame{*channel, *raw_value / 10.0};
}

template <typename T, std::size_t Capacity>
class RingBuffer {
    static_assert(Capacity > 0, "capacity must be positive");

public:
    bool push(T value)
    {
        if (size_ == Capacity) {
            return false;
        }
        data_[tail_] = std::move(value);
        tail_ = (tail_ + 1) % Capacity;
        ++size_;
        return true;
    }

    std::optional<T> pop()
    {
        if (size_ == 0) {
            return std::nullopt;
        }
        T value = std::move(data_[head_]);
        head_ = (head_ + 1) % Capacity;
        --size_;
        return value;
    }

    bool empty() const noexcept
    {
        return size_ == 0;
    }

    bool full() const noexcept
    {
        return size_ == Capacity;
    }

private:
    std::array<T, Capacity> data_{};
    std::size_t head_{0};
    std::size_t tail_{0};
    std::size_t size_{0};
};

class LineDecoder {
public:
    using Callback = std::function<void(std::string_view)>;

    explicit LineDecoder(Callback callback)
        : callback_{std::move(callback)}
    {
        buffer_.reserve(max_line_size);
    }

    void feed(std::string_view bytes)
    {
        for (char ch : bytes) {
            if (discarding_) {
                if (ch == '\n') {
                    discarding_ = false;
                }
                continue;
            }

            if (ch == '\n') {
                callback_(buffer_);
                buffer_.clear();
                continue;
            }

            if (buffer_.size() == max_line_size) {
                buffer_.clear();
                discarding_ = true;
                continue;
            }
            buffer_.push_back(ch);
        }
    }

private:
    static constexpr std::size_t max_line_size = 128;
    Callback callback_;
    std::string buffer_;
    bool discarding_{false};
};

template <typename T>
class BlockingQueue {
public:
    bool push(T value)
    {
        {
            std::lock_guard<std::mutex> lock{mutex_};
            if (closed_) {
                return false;
            }
            queue_.push(std::move(value));
        }
        condition_.notify_one();
        return true;
    }

    std::optional<T> pop()
    {
        std::unique_lock<std::mutex> lock{mutex_};
        condition_.wait(lock, [this] {
            return closed_ || !queue_.empty();
        });

        if (queue_.empty()) {
            return std::nullopt;
        }
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    void close()
    {
        {
            std::lock_guard<std::mutex> lock{mutex_};
            closed_ = true;
        }
        condition_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<T> queue_;
    bool closed_{false};
};

void test_device()
{
    Device device;
    assert(device.state() == DeviceState::idle);
    assert(device.start());
    assert(!device.start());
    assert(device.fail());
    assert(device.reset());
    assert(device.state() == DeviceState::idle);
}

void test_moving_average()
{
    MovingAverage average{3};
    assert(!average.average());
    average.add_sample(10.0);
    average.add_sample(20.0);
    assert(*average.average() == 15.0);
    average.add_sample(30.0);
    average.add_sample(50.0);
    assert(*average.average() == (20.0 + 30.0 + 50.0) / 3.0);
}

void test_frame_parser()
{
    const auto frame = parse_frame("TEMP,2,365\n");
    assert(frame);
    assert(frame->channel == 2);
    assert(frame->celsius == 36.5);
    assert(!parse_frame("TEMP,8,365\n"));
    assert(!parse_frame("TEMP,2,not-a-number\n"));
    assert(!parse_frame("UNKNOWN,2,365\n"));
}

void test_ring_buffer()
{
    RingBuffer<int, 2> buffer;
    assert(buffer.empty());
    assert(buffer.push(10));
    assert(buffer.push(20));
    assert(buffer.full());
    assert(!buffer.push(30));
    assert(buffer.pop() == 10);
    assert(buffer.pop() == 20);
    assert(!buffer.pop());
}

void test_line_decoder()
{
    std::vector<std::string> lines;
    LineDecoder decoder{[&lines](std::string_view line) {
        lines.emplace_back(line);
    }};

    decoder.feed("TEMP,0,");
    decoder.feed("235\nTEMP,1,300\npart");
    decoder.feed("ial\n");

    assert(lines == std::vector<std::string>({
                        "TEMP,0,235",
                        "TEMP,1,300",
                        "partial",
                    }));
}

void test_blocking_queue()
{
    BlockingQueue<int> queue;
    std::vector<int> received;

    std::thread consumer{[&] {
        while (const auto value = queue.pop()) {
            received.push_back(*value);
        }
    }};

    assert(queue.push(1));
    assert(queue.push(2));
    assert(queue.push(3));
    queue.close();
    consumer.join();

    assert((received == std::vector<int>{1, 2, 3}));
    assert(!queue.push(4));
}

int main()
{
    test_device();
    test_moving_average();
    test_frame_parser();
    test_ring_buffer();
    test_line_decoder();
    test_blocking_queue();

    std::cout << "All foundation practice checks passed.\n";
}
