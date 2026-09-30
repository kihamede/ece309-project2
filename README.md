# C++ Mini Conversation Harness

A command-line conversation harness implemented in C++ for ECE 309 at NC State University. The project focuses on object-oriented design, state management, scripted conversations, sentinel detection, testing, and CMake-based builds.

## Overview

The program supports interactive and scripted multi-turn conversations while maintaining structured message history and detecting conversation-ending sentinel sequences.

Key functionality includes:

- Structured message and conversation management
- Scripted multi-turn conversations
- Sentinel-based termination detection
- Conversation transcript saving
- Command-line execution
- Automated testing
- CMake-based build system

## My Implementation

My work focused on the core conversation and message-processing components, including:

- `Message` data structures
- `Conversation` state management
- `SentinelScanner` detection logic
- Project 2 test suite
- Design documentation

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

Example scripted conversation:

```bash
./build/miniharness --script scripts/greeting.script
```

Save the resulting transcript:

```bash
./build/miniharness --script scripts/greeting.script --save transcript.txt
```

## Testing

The project includes **18 assert-based tests** covering the student-implemented functionality.

```bash
./build/test_p2
```

The build configuration also supports AddressSanitizer and UndefinedBehaviorSanitizer checks for detecting memory and runtime issues.

## Technologies

- C++
- CMake
- Git / GitHub
- Command-Line Development
- Automated Testing
- Object-Oriented Programming

## What I Learned

This project strengthened my experience with C++ class design, maintaining program state across multiple interactions, automated testing, build systems, and working within an existing multi-file codebase.
