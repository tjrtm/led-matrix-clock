#pragma once

#include "animations.h"
#include "third_party/httplib.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

class AnimationManager {
public:
    AnimationManager(int width, int height);
    ~AnimationManager() = default;

    AnimationManager(const AnimationManager &) = delete;
    AnimationManager &operator=(const AnimationManager &) = delete;

    void Update(float dt);
    void Render(RenderTexture2D &target);
    bool IsActive() const;

    bool RequestAnimationByName(const std::string &name);
    std::vector<std::string> AnimationNames() const;

private:
    struct Entry {
        std::string name;
        std::unique_ptr<Animation> animation;
    };

    void StartAnimation(size_t index);
    void StopAnimation();

    int width_;
    int height_;
    std::vector<Entry> animations_;
    std::unordered_map<std::string, size_t> lookup_;

    mutable std::mutex mutex_;
    std::optional<size_t> activeIndex_;
    std::optional<size_t> pendingIndex_;
    std::chrono::steady_clock::time_point endTime_;
    const std::chrono::milliseconds animationDuration_{8000};
    bool active_ = false;
};

class AnimationRequestServer {
public:
    AnimationRequestServer(AnimationManager &manager, int port = 8080);
    ~AnimationRequestServer();

    AnimationRequestServer(const AnimationRequestServer &) = delete;
    AnimationRequestServer &operator=(const AnimationRequestServer &) = delete;

    void Start();
    void Stop();

private:
    void RegisterRoutes();

    AnimationManager &manager_;
    int port_;
    httplib::Server server_;
    std::thread worker_;
    std::atomic<bool> running_{false};
    std::once_flag routeInitFlag_;
};

inline AnimationManager::AnimationManager(int width, int height)
    : width_(width), height_(height) {
    animations_.push_back({"rainbow_cycle", std::make_unique<RainbowCycleAnimation>(width_, height_)});
    animations_.push_back({"matrix_rain", std::make_unique<MatrixRainAnimation>(width_, height_)});
    animations_.push_back({"starfield", std::make_unique<StarfieldAnimation>(width_, height_)});
    animations_.push_back({"swirl", std::make_unique<SwirlAnimation>(width_, height_)});
    animations_.push_back({"bouncing_balls", std::make_unique<BouncingBallAnimation>(width_, height_)});
    animations_.push_back({"wave_lines", std::make_unique<WaveLinesAnimation>(width_, height_)});
    animations_.push_back({"sparkle", std::make_unique<SparkleAnimation>(width_, height_)});
    animations_.push_back({"fire", std::make_unique<FireAnimation>(width_, height_)});
    animations_.push_back({"pulse_squares", std::make_unique<PulseSquaresAnimation>(width_, height_)});
    animations_.push_back({"scrolling_text", std::make_unique<ScrollingTextAnimation>(width_, height_, "LED MATRIX" )});

    for (size_t i = 0; i < animations_.size(); ++i) {
        lookup_.emplace(animations_[i].name, i);
        animations_[i].animation->Reset();
    }
}

inline void AnimationManager::Update(float dt) {
    std::optional<size_t> request;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pendingIndex_) {
            request = pendingIndex_;
            pendingIndex_.reset();
        }
    }

    if (request.has_value()) {
        StartAnimation(request.value());
    }

    if (activeIndex_.has_value()) {
        animations_[activeIndex_.value()].animation->Update(dt);
        if (std::chrono::steady_clock::now() >= endTime_) {
            StopAnimation();
        }
    }
}

inline void AnimationManager::Render(RenderTexture2D &target) {
    BeginTextureMode(target);
    ClearBackground(BLACK);
    if (activeIndex_.has_value()) {
        animations_[activeIndex_.value()].animation->DrawFrame();
    }
    EndTextureMode();
}

inline bool AnimationManager::IsActive() const {
    return active_;
}

inline bool AnimationManager::RequestAnimationByName(const std::string &name) {
    auto it = lookup_.find(name);
    if (it == lookup_.end()) {
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingIndex_ = it->second;
    }
    return true;
}

inline std::vector<std::string> AnimationManager::AnimationNames() const {
    std::vector<std::string> names;
    names.reserve(animations_.size());
    for (auto &entry : animations_) {
        names.push_back(entry.name);
    }
    return names;
}

inline void AnimationManager::StartAnimation(size_t index) {
    if (index >= animations_.size()) {
        return;
    }
    animations_[index].animation->Reset();
    activeIndex_ = index;
    active_ = true;
    endTime_ = std::chrono::steady_clock::now() + animationDuration_;
}

inline void AnimationManager::StopAnimation() {
    activeIndex_.reset();
    active_ = false;
}

inline AnimationRequestServer::AnimationRequestServer(AnimationManager &manager, int port)
    : manager_(manager), port_(port) {}

inline AnimationRequestServer::~AnimationRequestServer() {
    Stop();
}

inline void AnimationRequestServer::RegisterRoutes() {
    std::call_once(routeInitFlag_, [this]() {
        server_.Get("/api/animations", [this](const httplib::Request &, httplib::Response &res) {
            nlohmann::json payload;
            payload["animations"] = manager_.AnimationNames();
            res.set_content(payload.dump(), "application/json");
        });

        server_.Post("/api/animations/run", [this](const httplib::Request &req, httplib::Response &res) {
            nlohmann::json response;
            auto jsonBody = nlohmann::json::parse(req.body, nullptr, false);
            if (jsonBody.is_discarded()) {
                res.status = 400;
                response["error"] = "Invalid JSON payload";
                res.set_content(response.dump(), "application/json");
                return;
            }
            if (!jsonBody.contains("animation") || !jsonBody["animation"].is_string()) {
                res.status = 400;
                response["error"] = "Missing 'animation' string field";
                res.set_content(response.dump(), "application/json");
                return;
            }
            std::string requested = jsonBody["animation"].get<std::string>();
            if (manager_.RequestAnimationByName(requested)) {
                response["status"] = "accepted";
                response["animation"] = requested;
                response["duration_ms"] = 8000;
                res.set_content(response.dump(), "application/json");
            } else {
                res.status = 404;
                response["error"] = "Unknown animation";
                response["available"] = manager_.AnimationNames();
                res.set_content(response.dump(), "application/json");
            }
        });
    });
}

inline void AnimationRequestServer::Start() {
    if (running_.exchange(true)) {
        return;
    }
    RegisterRoutes();
    worker_ = std::thread([this]() {
        server_.listen("0.0.0.0", port_);
    });
}

inline void AnimationRequestServer::Stop() {
    if (!running_.exchange(false)) {
        return;
    }
    server_.stop();
    if (worker_.joinable()) {
        worker_.join();
    }
}

