#pragma once
#include "Logger.h"

#include <chrono>
#include <thread>

class WaveWorker {
public:
    explicit WaveWorker(
        std::chrono::milliseconds const interval = std::chrono::milliseconds(100)
        , std::chrono::milliseconds const delay = std::chrono::milliseconds(100)
        )
        : interval_(interval), delay_(delay) {}

    void start() {
        if (running_.exchange(true)) return;
        jt_ = std::jthread([this](std::stop_token st){ run(st); });
    }

    void startSimple() {
        if (running_.exchange(true)) return;
        jt_ = std::jthread([this](std::stop_token st){ runSimple(st); });
    }

    void stop() {
        if (!running_.exchange(false)) return;
        jt_.request_stop();
        // jthread auto-joins in destructor; no explicit join needed
    }

    ~WaveWorker(){ stop(); }

private:
    void runSimple(const std::stop_token& st) {
        const std::vector<std::string> chars = {".", "·", "˙", "°", "-", "~", "*"};
        size_t i = 0;

        while (!st.stop_requested()) {
            Logger::printRaw("\r" + chars[i % chars.size()] , false, true);
            std::this_thread::sleep_for(interval_);
            i++;
        }

        Logger::printRaw("\r", false, true);
    }

    void run(const std::stop_token& st) {
        const std::vector<std::string> wave = {".", "·", "˙", "°", "-", "~", "*", "o", "O", "°", "˙", "·"};
        const size_t wave_size = wave.size();

        size_t frame = 0;
        constexpr size_t displayWidth = 32;

        while (!st.stop_requested()) {
            std::string line = "\r";

            for (size_t pos = 0; pos < displayWidth; pos++) {
                size_t wave_pos = (pos + frame) % wave_size;
                if ((pos + frame / 3) % 7 == 0) {
                    wave_pos = (wave_pos + wave_size / 2) % wave_size;
                }
                line += wave[wave_pos];
            }

            static int spam_burst = 0;
            if (frame > 100 && (spam_burst > 0 || rand() % 150 == 0)) {
                if (spam_burst == 0) {
                    spam_burst = 5 + rand() % 49;
                }
                spam_burst--;
            }

            static int glitch_burst = 0;
            static int last_frame = 0;
            static bool shouldShiftBack = false;
            if (spam_burst == 0 && (glitch_burst > 0 || rand() % 222 == 0)) {
                if (glitch_burst == 0) {
                    glitch_burst = 1 + rand() % 8;
                }
                glitch_burst--;
                Logger::printRaw("\r" + std::string(displayWidth, ' '), false, false);
                frame += 7 + rand() % 25;
                std::this_thread::sleep_for(interval_ / 4);
            } else {
                if (spam_burst > 0) {
                    if (shouldShiftBack) {
                        frame = last_frame + 1;
                        shouldShiftBack = false;
                    } else if (rand() % 90 == 0) {
                        last_frame = frame;
                        frame += 3 + rand() % 10;
                        shouldShiftBack = true;
                    }
                    line += "\n";
                    Logger::printRaw(line + " ", false, false);
                    std::this_thread::sleep_for(interval_ / 5);
                } else {
                    Logger::printRaw(line + " ", false, false);
                    std::this_thread::sleep_for(interval_);
                }
            }

            frame++;
        }

        Logger::printRaw("\r" + std::string(displayWidth, ' ') + "\r", false, false);
    }

    std::chrono::milliseconds interval_;
    std::chrono::milliseconds delay_;
    std::atomic<bool> running_{false};
    std::jthread jt_;
};