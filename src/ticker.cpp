#include "ticker.h"


namespace ticker {

// Методы класса Ticker
    void Ticker::Start() {
        net::dispatch(strand_, [self = shared_from_this()] {
            self->last_tick_ = Clock::now(); // last_tick_
            self->ScheduleTick();
        });
    }

    void Ticker::ScheduleTick() {
        timer_.expires_after(period_);
        timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
            self->OnTick(ec);
        });
    }

    void Ticker::OnTick(sys::error_code ec) {
        using namespace std::chrono;

        if (!ec) {
            auto this_tick = Clock::now();
            auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
            last_tick_ = this_tick;
            handler_(delta);
            ScheduleTick();
        }
    }

} // namespace ticker