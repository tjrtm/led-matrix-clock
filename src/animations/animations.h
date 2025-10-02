#pragma once

#include "raylib.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <string>
#include <vector>

class Animation {
public:
    Animation(int width, int height) : width_(width), height_(height) {}
    virtual ~Animation() = default;

    virtual const char *Name() const = 0;
    virtual void Reset() = 0;
    virtual void Update(float dt) = 0;
    virtual void DrawFrame() = 0;

protected:
    int width_;
    int height_;
};

class RainbowCycleAnimation : public Animation {
public:
    RainbowCycleAnimation(int width, int height) : Animation(width, height) { Reset(); }

    const char *Name() const override { return "rainbow_cycle"; }

    void Reset() override { phase_ = 0.0f; }

    void Update(float dt) override {
        phase_ += dt * 0.12f;
        if (phase_ >= 1.0f) {
            phase_ -= std::floor(phase_);
        }
    }

    void DrawFrame() override {
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                float hue = std::fmod(((float)x / (float)width_) + phase_ + ((float)y / (float)(height_ * 2)), 1.0f);
                Color color = ColorFromHSV(hue * 360.0f, 1.0f, 1.0f);
                DrawPixel(x, y, color);
            }
        }
    }

private:
    float phase_ = 0.0f;
};

class MatrixRainAnimation : public Animation {
public:
    MatrixRainAnimation(int width, int height)
        : Animation(width, height), rng_(std::random_device{}()) {
        Reset();
    }

    const char *Name() const override { return "matrix_rain"; }

    void Reset() override {
        std::uniform_real_distribution<float> speed(8.0f, 20.0f);
        std::uniform_int_distribution<int> length(6, 18);
        columns_.clear();
        columns_.reserve(width_ / 2);
        for (int x = 0; x < width_; x += 2) {
            columns_.push_back({static_cast<float>(GetRandomValue(-height_, height_)), speed(rng_), length(rng_), x});
        }
    }

    void Update(float dt) override {
        for (auto &col : columns_) {
            col.y += col.speed * dt;
            if (col.y - col.length > height_) {
                col.y = static_cast<float>(GetRandomValue(-height_, 0));
                col.speed = speedDist_(rng_);
                col.length = lengthDist_(rng_);
            }
        }
    }

    void DrawFrame() override {
        for (auto &col : columns_) {
            for (int i = 0; i < col.length; ++i) {
                int drawY = static_cast<int>(col.y) - i;
                if (drawY < 0 || drawY >= height_) {
                    continue;
                }
                float intensity = 1.0f - (static_cast<float>(i) / static_cast<float>(col.length));
                unsigned char g = static_cast<unsigned char>(std::clamp(intensity * 255.0f, 40.0f, 255.0f));
                Color color = (i == 0) ? Color{180, 255, 180, 255} : Color{40, g, 40, 255};
                DrawPixel(col.x, drawY, color);
                if (col.x + 1 < width_) {
                    DrawPixel(col.x + 1, drawY, Fade(color, 0.7f));
                }
            }
        }
    }

private:
    struct Column {
        float y;
        float speed;
        int length;
        int x;
    };

    std::vector<Column> columns_;
    std::mt19937 rng_;
    std::uniform_real_distribution<float> speedDist_{8.0f, 20.0f};
    std::uniform_int_distribution<int> lengthDist_{6, 18};
};

class StarfieldAnimation : public Animation {
public:
    StarfieldAnimation(int width, int height)
        : Animation(width, height), rng_(std::random_device{}()) {
        Reset();
    }

    const char *Name() const override { return "starfield"; }

    void Reset() override {
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        std::uniform_real_distribution<float> depth(0.2f, 1.0f);
        stars_.clear();
        stars_.reserve(90);
        for (int i = 0; i < 90; ++i) {
            stars_.push_back({dist(rng_), dist(rng_), depth(rng_)});
        }
    }

    void Update(float dt) override {
        for (auto &star : stars_) {
            star.z -= dt * 0.35f;
            if (star.z <= 0.05f) {
                RespawnStar(star);
            }
        }
    }

    void DrawFrame() override {
        float halfW = width_ / 2.0f;
        float halfH = height_ / 2.0f;
        for (auto &star : stars_) {
            float projX = (star.x / star.z) * halfW + halfW;
            float projY = (star.y / star.z) * halfH + halfH;
            if (projX < 0 || projX >= width_ || projY < 0 || projY >= height_) {
                continue;
            }
            float brightness = std::clamp(1.0f - ((star.z - 0.05f) / 0.95f), 0.0f, 1.0f);
            unsigned char value = static_cast<unsigned char>(200.0f + 55.0f * brightness);
            DrawPixel(static_cast<int>(projX), static_cast<int>(projY), Color{value, value, value, 255});
        }
    }

private:
    struct Star {
        float x;
        float y;
        float z;
    };

    void RespawnStar(Star &star) {
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        std::uniform_real_distribution<float> depth(0.3f, 1.0f);
        star.x = dist(rng_);
        star.y = dist(rng_);
        star.z = depth(rng_);
    }

    std::vector<Star> stars_;
    std::mt19937 rng_;
};

class SwirlAnimation : public Animation {
public:
    SwirlAnimation(int width, int height) : Animation(width, height) { Reset(); }

    const char *Name() const override { return "swirl"; }

    void Reset() override { time_ = 0.0f; }

    void Update(float dt) override { time_ += dt * 0.9f; }

    void DrawFrame() override {
        float cx = (width_ - 1) / 2.0f;
        float cy = (height_ - 1) / 2.0f;
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                float dx = x - cx;
                float dy = y - cy;
                float dist = std::sqrt(dx * dx + dy * dy);
                float angle = std::atan2(dy, dx);
                float wave = std::sin(dist * 0.6f - time_ * 4.0f + angle * 2.0f);
                float brightness = std::clamp((wave + 1.0f) * 0.5f, 0.0f, 1.0f);
                float hue = std::fmod((angle / (2 * PI)) + 0.5f, 1.0f);
                Color color = ColorFromHSV(hue * 360.0f, 0.75f, 0.3f + 0.7f * brightness);
                DrawPixel(x, y, color);
            }
        }
    }

private:
    float time_ = 0.0f;
};

class BouncingBallAnimation : public Animation {
public:
    BouncingBallAnimation(int width, int height)
        : Animation(width, height), rng_(std::random_device{}()) {
        Reset();
    }

    const char *Name() const override { return "bouncing_balls"; }

    void Reset() override {
        std::uniform_real_distribution<float> px(4.0f, width_ - 4.0f);
        std::uniform_real_distribution<float> py(4.0f, height_ - 4.0f);
        std::uniform_real_distribution<float> vel(-24.0f, 24.0f);
        balls_.clear();
        balls_.reserve(5);
        for (int i = 0; i < 5; ++i) {
            Vector2 pos{px(rng_), py(rng_)};
            Vector2 velv{vel(rng_), vel(rng_)};
            Color color = ColorFromHSV((i * 60.0f), 0.9f, 1.0f);
            balls_.push_back({pos, velv, color});
        }
    }

    void Update(float dt) override {
        for (auto &ball : balls_) {
            ball.pos.x += ball.vel.x * dt;
            ball.pos.y += ball.vel.y * dt;
            if (ball.pos.x < 2.0f) {
                ball.pos.x = 2.0f;
                ball.vel.x *= -1.0f;
            }
            if (ball.pos.x > width_ - 3.0f) {
                ball.pos.x = width_ - 3.0f;
                ball.vel.x *= -1.0f;
            }
            if (ball.pos.y < 2.0f) {
                ball.pos.y = 2.0f;
                ball.vel.y *= -1.0f;
            }
            if (ball.pos.y > height_ - 3.0f) {
                ball.pos.y = height_ - 3.0f;
                ball.vel.y *= -1.0f;
            }
        }
    }

    void DrawFrame() override {
        for (auto &ball : balls_) {
            DrawCircleV(ball.pos, 2.5f, ball.color);
        }
    }

private:
    struct Ball {
        Vector2 pos;
        Vector2 vel;
        Color color;
    };

    std::vector<Ball> balls_;
    std::mt19937 rng_;
};

class WaveLinesAnimation : public Animation {
public:
    WaveLinesAnimation(int width, int height) : Animation(width, height) { Reset(); }

    const char *Name() const override { return "wave_lines"; }

    void Reset() override { time_ = 0.0f; }

    void Update(float dt) override { time_ += dt; }

    void DrawFrame() override {
        for (int x = 0; x < width_; ++x) {
            float base = std::sin(time_ * 2.0f + x * 0.25f);
            float offset = std::cos(time_ + x * 0.13f) * 4.0f;
            float centerY = (height_ / 2.0f) + offset;
            Color color = ColorFromHSV(std::fmod((time_ * 40.0f + x * 2.5f), 360.0f), 0.8f, 0.9f);
            int y = static_cast<int>(centerY + base * (height_ / 3.0f));
            for (int dy = -2; dy <= 2; ++dy) {
                int drawY = y + dy;
                if (drawY >= 0 && drawY < height_) {
                    float fade = 1.0f - (std::abs(dy) / 3.0f);
                    DrawPixel(x, drawY, Fade(color, std::clamp(fade, 0.1f, 1.0f)));
                }
            }
        }
    }

private:
    float time_ = 0.0f;
};

class SparkleAnimation : public Animation {
public:
    SparkleAnimation(int width, int height)
        : Animation(width, height), rng_(std::random_device{}()),
          distribution_(0, width * height - 1) {
        Reset();
    }

    const char *Name() const override { return "sparkle"; }

    void Reset() override {
        brightness_.assign(width_ * height_, 0.0f);
    }

    void Update(float dt) override {
        for (float &b : brightness_) {
            b *= (1.0f - dt * 3.5f);
            if (b < 0.0f) {
                b = 0.0f;
            }
        }
        for (int i = 0; i < 8; ++i) {
            int idx = distribution_(rng_);
            brightness_[idx] = 1.0f;
        }
    }

    void DrawFrame() override {
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                float value = brightness_[y * width_ + x];
                Color color = ColorFromHSV(60.0f, 0.2f, std::clamp(value, 0.0f, 1.0f));
                if (value > 0.8f) {
                    color = Color{255, 255, 200, 255};
                }
                DrawPixel(x, y, color);
            }
        }
    }

private:
    std::vector<float> brightness_;
    std::mt19937 rng_;
    std::uniform_int_distribution<int> distribution_;
};

class FireAnimation : public Animation {
public:
    FireAnimation(int width, int height) : Animation(width, height), rng_(std::random_device{}()) {
        Reset();
    }

    const char *Name() const override { return "fire"; }

    void Reset() override {
        buffer_.assign(width_ * height_, 0);
    }

    void Update(float dt) override {
        (void)dt;
        std::uniform_int_distribution<int> bottom(160, 255);
        for (int x = 0; x < width_; ++x) {
            buffer_[(height_ - 1) * width_ + x] = bottom(rng_);
        }
        for (int y = height_ - 2; y >= 0; --y) {
            for (int x = 0; x < width_; ++x) {
                int below = buffer_[(y + 1) * width_ + x];
                int belowLeft = buffer_[(y + 1) * width_ + ((x == 0) ? x : x - 1)];
                int belowRight = buffer_[(y + 1) * width_ + ((x == width_ - 1) ? x : x + 1)];
                int belowFar = buffer_[((y + 2) < height_ ? (y + 2) : (height_ - 1)) * width_ + x];
                int value = (below + belowLeft + belowRight + belowFar) / 4;
                value -= GetRandomValue(0, 12);
                if (value < 0) {
                    value = 0;
                }
                buffer_[y * width_ + x] = value;
            }
        }
    }

    void DrawFrame() override {
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                int value = buffer_[y * width_ + x];
                float hue = 20.0f + (value / 255.0f) * 40.0f;
                float brightness = std::clamp(value / 255.0f, 0.0f, 1.0f);
                Color color = ColorFromHSV(hue, 1.0f, std::max(0.2f, brightness));
                DrawPixel(x, y, color);
            }
        }
    }

private:
    std::vector<int> buffer_;
    std::mt19937 rng_;
};

class PulseSquaresAnimation : public Animation {
public:
    PulseSquaresAnimation(int width, int height) : Animation(width, height) { Reset(); }

    const char *Name() const override { return "pulse_squares"; }

    void Reset() override {
        pulses_.clear();
        time_ = 0.0f;
    }

    void Update(float dt) override {
        time_ += dt;
        if (pulses_.empty() || pulses_.back().radius > std::max(width_, height_) / 6.0f) {
            pulses_.push_back({0.0f, 14.0f});
        }
        for (auto &pulse : pulses_) {
            pulse.radius += pulse.speed * dt;
        }
        while (!pulses_.empty() && pulses_.front().radius > std::max(width_, height_)) {
            pulses_.erase(pulses_.begin());
        }
    }

    void DrawFrame() override {
        float cx = (width_ - 1) / 2.0f;
        float cy = (height_ - 1) / 2.0f;
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                float dist = std::max(std::abs(x - cx), std::abs(y - cy));
                float brightness = 0.0f;
                for (auto &pulse : pulses_) {
                    float diff = std::fabs(dist - pulse.radius);
                    if (diff < 2.5f) {
                        brightness = std::max(brightness, 1.0f - (diff / 2.5f));
                    }
                }
                Color color = ColorFromHSV(std::fmod((time_ * 60.0f + dist * 10.0f), 360.0f), 0.7f, 0.2f + 0.8f * brightness);
                DrawPixel(x, y, color);
            }
        }
    }

private:
    struct Pulse {
        float radius;
        float speed;
    };

    std::vector<Pulse> pulses_;
    float time_ = 0.0f;
};

class ScrollingTextAnimation : public Animation {
public:
    ScrollingTextAnimation(int width, int height, std::string message)
        : Animation(width, height), message_(std::move(message)) {
        Reset();
    }

    const char *Name() const override { return "scrolling_text"; }

    void Reset() override {
        textWidth_ = MeasureText(message_.c_str(), fontSize_);
        offset_ = static_cast<float>(width_);
    }

    void Update(float dt) override {
        offset_ -= dt * 24.0f;
        if (offset_ < -textWidth_) {
            offset_ = static_cast<float>(width_);
        }
    }

    void DrawFrame() override {
        int y = height_ / 2 - fontSize_ / 2;
        DrawText(message_.c_str(), static_cast<int>(offset_), y, fontSize_, WHITE);
        DrawText(message_.c_str(), static_cast<int>(offset_) + textWidth_ + 4, y, fontSize_, WHITE);
    }

private:
    std::string message_;
    float offset_ = 0.0f;
    int textWidth_ = 0;
    int fontSize_ = 6;
};

