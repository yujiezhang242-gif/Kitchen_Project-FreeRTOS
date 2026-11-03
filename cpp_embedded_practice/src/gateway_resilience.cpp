#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using Milliseconds = std::chrono::milliseconds;

class ExponentialBackoff {
public:
    ExponentialBackoff(Milliseconds initial, Milliseconds maximum)
        : initial_{initial},
          maximum_{maximum},
          next_{initial}
    {
        assert(initial_.count() > 0);
        assert(maximum_ >= initial_);
    }

    Milliseconds take_delay()
    {
        const Milliseconds current = next_;
        next_ = std::min(next_ * 2, maximum_);
        return current;
    }

    void reset() noexcept
    {
        next_ = initial_;
    }

private:
    Milliseconds initial_;
    Milliseconds maximum_;
    Milliseconds next_;
};

enum class LinkState {
    disconnected,
    connecting,
    online,
    stopping,
    stopped
};

struct Start {};
struct ConnectSucceeded {};
struct ConnectFailed {
    std::string reason;
};
struct LinkLost {
    std::string reason;
};
struct RetryTimerExpired {};
struct StopRequested {};

using Event = std::variant<
    Start,
    ConnectSucceeded,
    ConnectFailed,
    LinkLost,
    RetryTimerExpired,
    StopRequested>;

struct StartConnectAction {};
struct ScheduleRetryAction {
    Milliseconds delay;
};
struct PublishStatusAction {
    std::string message;
};
struct CloseTransportAction {};

using Action = std::variant<
    StartConnectAction,
    ScheduleRetryAction,
    PublishStatusAction,
    CloseTransportAction>;

class GatewayStateMachine {
public:
    std::vector<Action> handle(const Event& event)
    {
        return std::visit(
            [this](const auto& concrete_event) {
                return on_event(concrete_event);
            },
            event);
    }

    LinkState state() const noexcept
    {
        return state_;
    }

private:
    std::vector<Action> on_event(const Start&)
    {
        if (state_ != LinkState::disconnected) {
            return {};
        }
        state_ = LinkState::connecting;
        return {
            PublishStatusAction{"connecting"},
            StartConnectAction{},
        };
    }

    std::vector<Action> on_event(const ConnectSucceeded&)
    {
        if (state_ != LinkState::connecting) {
            return {};
        }
        retry_.reset();
        state_ = LinkState::online;
        return {PublishStatusAction{"online"}};
    }

    std::vector<Action> on_event(const ConnectFailed& event)
    {
        if (state_ != LinkState::connecting) {
            return {};
        }
        state_ = LinkState::disconnected;
        return {
            PublishStatusAction{
                "connect failed: " + event.reason},
            ScheduleRetryAction{retry_.take_delay()},
        };
    }

    std::vector<Action> on_event(const LinkLost& event)
    {
        if (state_ != LinkState::online) {
            return {};
        }
        state_ = LinkState::disconnected;
        return {
            PublishStatusAction{"link lost: " + event.reason},
            CloseTransportAction{},
            ScheduleRetryAction{retry_.take_delay()},
        };
    }

    std::vector<Action> on_event(const RetryTimerExpired&)
    {
        if (state_ != LinkState::disconnected) {
            return {};
        }
        state_ = LinkState::connecting;
        return {
            PublishStatusAction{"reconnecting"},
            StartConnectAction{},
        };
    }

    std::vector<Action> on_event(const StopRequested&)
    {
        if (state_ == LinkState::stopped ||
            state_ == LinkState::stopping) {
            return {};
        }
        const bool was_online = state_ == LinkState::online;
        state_ = LinkState::stopping;
        std::vector<Action> actions{
            PublishStatusAction{"stopping"}};
        if (was_online) {
            actions.emplace_back(CloseTransportAction{});
        }
        state_ = LinkState::stopped;
        actions.emplace_back(PublishStatusAction{"stopped"});
        return actions;
    }

    LinkState state_{LinkState::disconnected};
    ExponentialBackoff retry_{Milliseconds{1000}, Milliseconds{30000}};
};

template <typename T>
const T* find_action(const std::vector<Action>& actions)
{
    for (const auto& action : actions) {
        if (const auto* value = std::get_if<T>(&action)) {
            return value;
        }
    }
    return nullptr;
}

void test_successful_connection()
{
    GatewayStateMachine machine;
    const auto start_actions = machine.handle(Start{});
    assert(machine.state() == LinkState::connecting);
    assert(find_action<StartConnectAction>(start_actions));

    const auto online_actions = machine.handle(ConnectSucceeded{});
    assert(machine.state() == LinkState::online);
    assert(find_action<PublishStatusAction>(online_actions));
}

void test_backoff()
{
    GatewayStateMachine machine;
    machine.handle(Start{});

    const auto first_failure =
        machine.handle(ConnectFailed{"connection refused"});
    const auto* first_retry =
        find_action<ScheduleRetryAction>(first_failure);
    assert(first_retry);
    assert(first_retry->delay == Milliseconds{1000});

    machine.handle(RetryTimerExpired{});
    const auto second_failure =
        machine.handle(ConnectFailed{"timeout"});
    const auto* second_retry =
        find_action<ScheduleRetryAction>(second_failure);
    assert(second_retry);
    assert(second_retry->delay == Milliseconds{2000});
}

void test_stop()
{
    GatewayStateMachine machine;
    machine.handle(Start{});
    machine.handle(ConnectSucceeded{});
    const auto actions = machine.handle(StopRequested{});
    assert(machine.state() == LinkState::stopped);
    assert(find_action<PublishStatusAction>(actions));
}

int main()
{
    test_successful_connection();
    test_backoff();
    test_stop();
    std::cout << "All gateway resilience checks passed.\n";
}
