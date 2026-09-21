// include/core/message.h
// ECE 309 Project 2

#pragma once

#include <string>
#include <utility>

enum class Role { System, User, Assistant };

class Message {
public:
    // Empty System message. This lets Conversation allocate Message arrays.
    Message() : role_(Role::System), content_("") {}

    Message(Role role, std::string content)
        : role_(role), content_(std::move(content)) {}

    Role role() const noexcept {
        return role_;
    }

    const std::string& content() const noexcept {
        return content_;
    }

private:
    Role role_;
    std::string content_;
};
