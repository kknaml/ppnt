
export module kknt.jr.event_loop;

import std;
import v8;
import ppnt.traits;

export namespace ppnt::jr {

    struct TimerKey {
        std::chrono::steady_clock::time_point deadline;
        int id;

        auto operator<=>(const TimerKey &other) const noexcept -> std::strong_ordering {
            return this->deadline <=> other.deadline;
        }
    };

    struct TimerData {
        v8::Global<v8::Function> callback;
        bool is_interval;
        long interval_ms;
    };


    class EventLoop final : public NonCopy  {
    private:
        v8::Isolate *isolate_;
        int next_id_{1};
        std::priority_queue<TimerKey, std::vector<TimerKey>, std::greater<>> timer_heap_;
        std::unordered_map<int, TimerData> timer_map_;
    public:
        explicit EventLoop(v8::Isolate *isolate) : isolate_{isolate} {}

        auto clear_all() -> void {
            timer_map_.clear();
            timer_heap_ = {};
        }

        auto run_until_empty(v8::Local<v8::Context> context) -> void {
            while (tick(context)) {
                wait_for_next();
            }
        }

        auto cancel(int id) -> void {
            timer_map_.erase(id);
        }

    private:

        auto schedule(
            v8::Local<v8::Function> callback,
            long delay_ms,
            bool is_interval
         ) -> int {
            auto id = next_id_++;
            auto now = std::chrono::steady_clock::now();
            auto deadline = now + std::chrono::milliseconds(delay_ms);
            timer_heap_.push({deadline, id});
            auto data = TimerData{};
            data.callback.Reset(isolate_, callback);
            data.is_interval = is_interval;
            data.interval_ms = delay_ms;
            timer_map_.emplace(id, std::move(data));
            return id;
        }

        auto tick(v8::Local<v8::Context> context) -> bool {
            isolate_->PerformMicrotaskCheckpoint();
            auto now = std::chrono::steady_clock::now();
            while (!timer_heap_.empty()) {
                auto key = timer_heap_.top();
                if (key.deadline > now) {
                    return true;
                }
                timer_heap_.pop();
                auto it = timer_map_.find(key.id);
                if (it == timer_map_.end()) {
                    continue;
                }
                auto cb_copy = v8::Global<v8::Function>(isolate_, it->second.callback);
                auto is_interval = it->second.is_interval;

                if (!is_interval) {
                    timer_map_.erase(it);
                }
                {
                    auto handle_Scope = v8::HandleScope{isolate_};
                    auto func = v8::Local<v8::Function>::New(isolate_, cb_copy);
                    auto receiver = context->Global();

                    auto try_catch = v8::TryCatch{isolate_};
                    func->Call(context, receiver, 0, nullptr).IsEmpty();
                    if (try_catch.HasCaught()) {
                        auto err = v8::String::Utf8Value{isolate_, try_catch.Message()->Get()};
                        std::println("[JS Error]: {}", *err);
                    }
                }
                isolate_->PerformMicrotaskCheckpoint();

                if (is_interval) {
                    auto refresh_it = timer_map_.find(key.id);
                    if (refresh_it != timer_map_.end()) {
                        auto new_deadline = key.deadline + std::chrono::milliseconds(refresh_it->second.interval_ms);
                        timer_heap_.push({new_deadline, key.id});
                    }
                }
            }
            return !timer_map_.empty();
        }

        auto wait_for_next() -> void {
            if (timer_heap_.empty()) return;
            auto key = timer_heap_.top();
            auto now = std::chrono::steady_clock::now();
            if (key.deadline > now) {
                std::this_thread::sleep_for(key.deadline - now);
            }
        }
    };
}
