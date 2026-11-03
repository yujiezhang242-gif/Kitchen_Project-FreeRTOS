#include <array>
#include <cerrno>
#include <cstdint>
#include <iostream>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <system_error>
#include <unistd.h>
#include <utility>

class UniqueFd {
public:
    explicit UniqueFd(int fd = -1) noexcept
        : fd_{fd}
    {
    }

    ~UniqueFd()
    {
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd&& other) noexcept
        : fd_{std::exchange(other.fd_, -1)}
    {
    }

    UniqueFd& operator=(UniqueFd&& other) noexcept
    {
        if (this != &other) {
            reset();
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }

    int get() const noexcept
    {
        return fd_;
    }

    explicit operator bool() const noexcept
    {
        return fd_ >= 0;
    }

    void reset(int new_fd = -1) noexcept
    {
        if (fd_ >= 0) {
            ::close(fd_);
        }
        fd_ = new_fd;
    }

private:
    int fd_;
};

[[noreturn]] void throw_errno(const char* operation)
{
    throw std::system_error{
        errno, std::generic_category(), operation};
}

int main()
{
    UniqueFd epoll_fd{::epoll_create1(EPOLL_CLOEXEC)};
    if (!epoll_fd) {
        throw_errno("epoll_create1");
    }

    UniqueFd timer_fd{
        ::timerfd_create(
            CLOCK_MONOTONIC,
            TFD_CLOEXEC | TFD_NONBLOCK)};
    if (!timer_fd) {
        throw_errno("timerfd_create");
    }

    itimerspec timer{};
    timer.it_value.tv_sec = 1;
    timer.it_interval.tv_sec = 1;
    if (::timerfd_settime(timer_fd.get(), 0, &timer, nullptr) < 0) {
        throw_errno("timerfd_settime");
    }

    epoll_event registration{};
    registration.events = EPOLLIN;
    registration.data.fd = timer_fd.get();
    if (::epoll_ctl(
            epoll_fd.get(),
            EPOLL_CTL_ADD,
            timer_fd.get(),
            &registration) < 0) {
        throw_errno("epoll_ctl");
    }

    std::array<epoll_event, 8> events{};
    int ticks_left = 5;

    while (ticks_left > 0) {
        const int count = ::epoll_wait(
            epoll_fd.get(),
            events.data(),
            static_cast<int>(events.size()),
            -1);

        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count < 0) {
            throw_errno("epoll_wait");
        }

        for (int index = 0; index < count; ++index) {
            const auto event_index =
                static_cast<std::size_t>(index);
            if (events[event_index].data.fd != timer_fd.get()) {
                continue;
            }

            std::uint64_t expirations{};
            const ssize_t size = ::read(
                timer_fd.get(),
                &expirations,
                sizeof(expirations));

            if (size == static_cast<ssize_t>(sizeof(expirations))) {
                std::cout << "timer event, expirations="
                          << expirations << '\n';
                --ticks_left;
            } else if (size < 0 &&
                       errno != EAGAIN &&
                       errno != EINTR) {
                throw_errno("read timerfd");
            }
        }
    }
}
