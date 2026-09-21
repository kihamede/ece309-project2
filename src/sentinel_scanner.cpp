// src/sentinel_scanner.cpp
// ECE 309 Project 2

#include "core/sentinel_scanner.h"

#include <utility>

SentinelScanner::SentinelScanner(std::string sentinel)
    : sentinel_(std::move(sentinel)) {}

SentinelScanner::Out SentinelScanner::feed(std::string_view chunk) {
    // Add the new chunk to the small amount of text we were holding back.
    pending_.append(chunk.data(), chunk.size());

    // An empty sentinel is treated as an immediate match.
    if (sentinel_.empty()) {
        pending_.clear();
        return {"", true};
    }

    std::size_t found = pending_.find(sentinel_);
    if (found != std::string::npos) {
        std::string safe = pending_.substr(0, found);
        pending_.clear();
        return {safe, true};
    }

    // If there is no full match, only the final sentinel length - 1
    // characters could still become the start of a sentinel later.
    std::size_t keep = sentinel_.size() - 1;

    if (pending_.size() <= keep) {
        return {"", false};
    }

    std::size_t safe_count = pending_.size() - keep;
    std::string safe = pending_.substr(0, safe_count);
    pending_.erase(0, safe_count);

    return {safe, false};
}

SentinelScanner::Out SentinelScanner::flush() {
    std::string safe = pending_;
    pending_.clear();
    return {safe, false};
}
