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
