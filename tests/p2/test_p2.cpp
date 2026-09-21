// tests/p2/test_p2.cpp
// ECE 309 Project 2
// Simple assert-based tests for Conversation, SentinelScanner, and the
// provided harness/model code.

#include "core/conversation.h"
#include "core/message.h"
#include "core/sentinel_scanner.h"
#include "harness/harness.h"
#include "model/replay_client.h"
#include "model/scripted_client.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>


struct SentinelScannerTestAccess {
    static std::size_t pending_size(const SentinelScanner& scanner) {
        return scanner.pending_.size();
    }
};

namespace {

const std::string kSentinel = "<|end_conversation|>";

void write_file(const std::string& path, const std::string& text) {
    std::ofstream file(path);
    assert(file.is_open());
    file << text;
}

class FakeInput : public InputSource {
public:
    explicit FakeInput(std::vector<std::string> lines)
        : lines_(std::move(lines)) {}

    std::string read_line() override {
        if (index_ >= lines_.size()) {
            eof_ = true;
            return "";
        }

        return lines_[index_++];
    }

    bool is_eof() const override {
        return eof_;
    }

private:
    std::vector<std::string> lines_;
    std::size_t index_ = 0;
    bool eof_ = false;
};

class FakeOutput : public OutputSink {
public:
    void write(std::string_view text) override {
        text_ += text;
    }

    const std::string& text() const {
        return text_;
    }

private:
    std::string text_;
};

class CollectingSink : public TokenSink {
public:
    void on_chunk(std::string_view chunk) override {
        text += chunk;
    }

    void on_complete() override {}

    std::string text;
};

void test_empty_conversation_bounds() {
    Conversation conv;

    assert(conv.size() == 0);
    assert(conv.begin() == conv.end());

    bool threw = false;
    try {
        conv.at(0);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    assert(threw);
}

void test_message_basics() {
    Message empty;
    assert(empty.role() == Role::System);
    assert(empty.content().empty());

    Message user(Role::User, "hello");
    assert(user.role() == Role::User);
    assert(user.content() == "hello");
}

void test_system_message_ordering() {
    const std::string script = "p2_test_system.script";
    write_file(script,
               "role: assistant\n"
               "unused<|end_conversation|>\n");

    auto model = std::make_unique<ScriptedModelClient>(script);
    HarnessConfig cfg;
    cfg.system_message = "Be concise.";

    Harness harness(std::move(model), cfg);
    assert(harness.conversation().size() == 1);
    assert(harness.conversation().at(0).role() == Role::System);
    assert(harness.conversation().at(0).content() == "Be concise.");

    std::remove(script.c_str());
}

void test_copy_constructor_deep_copy() {
    Conversation first;
    first.append(Message(Role::User, "one"));
    first.append(Message(Role::Assistant, "two"));

    Conversation copy(first);

    assert(copy.size() == first.size());
    assert(copy.begin() != first.begin());
    assert(copy.at(0).content() == "one");
    assert(copy.at(1).content() == "two");
}

void test_copy_assignment_deep_copy() {
    Conversation first;
    first.append(Message(Role::User, "copied"));

    Conversation second;
    second.append(Message(Role::Assistant, "old"));
    second = first;

    assert(second.size() == 1);
    assert(second.begin() != first.begin());
    assert(second.at(0).content() == "copied");
}

void test_move_constructor_steals_buffer() {
    Conversation first;
    first.append(Message(Role::User, "move me"));
    const Message* old_pointer = first.begin();

    Conversation moved(std::move(first));

    assert(moved.begin() == old_pointer);
    assert(moved.size() == 1);
    assert(moved.at(0).content() == "move me");
    assert(first.size() == 0);
    assert(first.begin() == nullptr);
    assert(first.begin() == first.end());
}

void test_move_assignment_steals_buffer() {
    Conversation first;
    first.append(Message(Role::User, "move assign"));
    const Message* old_pointer = first.begin();

    Conversation second;
    second.append(Message(Role::Assistant, "old"));
    second = std::move(first);

    assert(second.begin() == old_pointer);
    assert(second.size() == 1);
    assert(second.at(0).content() == "move assign");
    assert(first.size() == 0);
    assert(first.begin() == nullptr);
}

void test_growth_and_reallocation() {
    Conversation conv;

    conv.append(Message(Role::User, "0"));
    const Message* capacity_one = conv.begin();

    conv.append(Message(Role::User, "1"));
    const Message* capacity_two = conv.begin();
    assert(capacity_two != capacity_one);

    conv.append(Message(Role::User, "2"));
    const Message* capacity_four = conv.begin();
    assert(capacity_four != capacity_two);

    // With doubling, the fourth append still fits in capacity 4.
    conv.append(Message(Role::User, "3"));
    assert(conv.begin() == capacity_four);

    // The fifth append grows from 4 to 8.
    conv.append(Message(Role::User, "4"));
    assert(conv.begin() != capacity_four);

    assert(conv.size() == 5);
    for (std::size_t i = 0; i < conv.size(); ++i) {
        assert(conv.at(i).content() == std::to_string(i));
    }
}

void test_scanner_clean_text() {
    SentinelScanner scanner(kSentinel);

    auto first = scanner.feed("hello world");
    auto last = scanner.flush();

    assert(!first.sentinel_found);
    assert(!last.sentinel_found);
    assert(first.safe_text + last.safe_text == "hello world");
}

void test_scanner_whole_sentinel() {
    SentinelScanner scanner(kSentinel);

    auto out = scanner.feed("Goodbye." + kSentinel);

    assert(out.sentinel_found);
    assert(out.safe_text == "Goodbye.");
}

void test_scanner_every_split_boundary() {
    const std::string text = "Goodbye." + kSentinel;

    for (std::size_t split = 0; split <= text.size(); ++split) {
        SentinelScanner scanner(kSentinel);

        auto out1 = scanner.feed(text.substr(0, split));
        auto out2 = scanner.feed(text.substr(split));

        assert(out1.sentinel_found || out2.sentinel_found);
        assert(out1.safe_text + out2.safe_text == "Goodbye.");
    }
}

void test_scanner_one_character_at_a_time() {
    SentinelScanner scanner(kSentinel);
    const std::string text = "abc" + kSentinel;
    std::string safe;
    bool found = false;

    for (char ch : text) {
        std::string one(1, ch);
        auto out = scanner.feed(one);
        safe += out.safe_text;
        if (out.sentinel_found) {
            found = true;
            break;
        }
    }

    assert(found);
    assert(safe == "abc");
}

void test_scanner_false_alarm() {
    SentinelScanner scanner(kSentinel);
    const std::string input = "text <|end_world|> more text";

    auto out1 = scanner.feed(input);
    auto out2 = scanner.flush();

    assert(!out1.sentinel_found);
    assert(!out2.sentinel_found);
    assert(out1.safe_text + out2.safe_text == input);
}

void test_scanner_large_adversarial_stream() {
    SentinelScanner scanner(kSentinel);
    std::string output;

    // A large stream with many prefixes that look like the sentinel.
    const std::string piece = "<|end_";
    for (int i = 0; i < 700000; ++i) {
        for (char ch : piece) {
            std::string one(1, ch);
            auto out = scanner.feed(one);
            assert(!out.sentinel_found);
            assert(SentinelScannerTestAccess::pending_size(scanner) <=
                   kSentinel.size() - 1);
            output += out.safe_text;
        }
    }

    auto final_out = scanner.flush();
    output += final_out.safe_text;

    std::string expected;
    expected.reserve(piece.size() * 700000);
    for (int i = 0; i < 700000; ++i) {
        expected += piece;
    }

    assert(output == expected);
}

void test_harness_turn_limit() {
    const std::string script = "p2_test_turn_limit.script";
    write_file(script,
               "role: assistant\nfirst reply\n---\n"
               "role: assistant\nsecond reply\n");

    auto model = std::make_unique<ScriptedModelClient>(script);
    HarnessConfig cfg;
    cfg.max_turns = 1;
    Harness harness(std::move(model), cfg);

    FakeInput input({"hello"});
    FakeOutput output;

    StopReason reason = harness.run(input, output);
    assert(reason.kind == StopReason::Kind::TurnLimit);
    assert(harness.conversation().size() == 2);

    std::remove(script.c_str());
}

void test_harness_eof() {
    const std::string script = "p2_test_eof.script";
    write_file(script,
               "role: assistant\nunused<|end_conversation|>\n");

    auto model = std::make_unique<ScriptedModelClient>(script);
    Harness harness(std::move(model), HarnessConfig{});

    FakeInput input({});
    FakeOutput output;

    StopReason reason = harness.run(input, output);
    assert(reason.kind == StopReason::Kind::UserExit);
    assert(harness.conversation().size() == 0);

    std::remove(script.c_str());
}

void test_harness_sentinel_halt() {
    const std::string script = "p2_test_sentinel.script";
    write_file(script,
               "chunk: 3\n"
               "role: assistant\n"
               "Goodbye!<|end_conversation|>\n");

    auto model = std::make_unique<ScriptedModelClient>(script);
    Harness harness(std::move(model), HarnessConfig{});

    FakeInput input({"bye"});
    FakeOutput output;

    StopReason reason = harness.run(input, output);

    assert(reason.kind == StopReason::Kind::Sentinel);
    assert(output.text().find(kSentinel) == std::string::npos);
    assert(output.text().find("Goodbye!") != std::string::npos);
    assert(harness.conversation().size() == 2);
    assert(harness.conversation().at(1).content() == "Goodbye!" + kSentinel);

    std::remove(script.c_str());
}

void test_transcript_round_trip() {
    const std::string transcript = "p2_test_transcript.txt";
    write_file(transcript,
               "role: system\n"
               "Be concise.\n"
               "---\n"
               "role: user\n"
               "hello\n"
               "---\n"
               "role: assistant\n"
               "Hi there.\n"
               "---\n"
               "role: user\n"
               "bye\n"
               "---\n"
               "role: assistant\n"
               "Goodbye!<|end_conversation|>\n");

    ReplayModelClient replay(transcript);
    assert(replay.system_message() == "Be concise.");

    Conversation conv;
    conv.append(Message(Role::System, "Be concise."));
    conv.append(Message(Role::User, "hello"));

    CollectingSink first;
    replay.generate(conv, first);
    assert(first.text == "Hi there.");

    conv.append(Message(Role::Assistant, first.text));
    conv.append(Message(Role::User, "bye"));

    CollectingSink second;
    replay.generate(conv, second);
    assert(second.text == "Goodbye!" + kSentinel);

    std::remove(transcript.c_str());
}

}  // namespace

int main() {
    test_empty_conversation_bounds();
    test_message_basics();
    test_system_message_ordering();
    test_copy_constructor_deep_copy();
    test_copy_assignment_deep_copy();
    test_move_constructor_steals_buffer();
    test_move_assignment_steals_buffer();
    test_growth_and_reallocation();
    test_scanner_clean_text();
    test_scanner_whole_sentinel();
    test_scanner_every_split_boundary();
    test_scanner_one_character_at_a_time();
    test_scanner_false_alarm();
    test_scanner_large_adversarial_stream();
    test_harness_turn_limit();
    test_harness_eof();
    test_harness_sentinel_halt();
    test_transcript_round_trip();

    std::cout << "All 18 Project 2 tests passed.\n";
    return 0;
}
