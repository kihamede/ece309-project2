# Design Log — Project 2

## Growth factor and amortized cost

For my `Conversation` class I used a growth factor of 2. The conversation starts with a capacity of 0. When the first message is appended, I allocate room for 1 message. After that, whenever `size_ == capacity_`, I double the capacity, so the capacities go 1, 2, 4, 8, 16, and so on. I chose doubling because it is simple to implement and it avoids reallocating the array for every append.

Most calls to `append()` only place the new `Message` into the next open array position, so those calls take constant time. A reallocation costs more because all of the old messages have to be moved to a new array. However, the expensive operations do not happen every time. If there are `n` appends, the numbers of elements moved at the reallocations are approximately 1 + 2 + 4 + 8 + ... up to `n`. This geometric series is less than `2n`. That means the total amount of moving for `n` appends is O(n). Dividing that total work over the `n` append operations gives O(1) amortized time per append.

## Rule of Five evidence

`Conversation` owns a dynamically allocated `Message` array, so I implemented all five resource-management functions. The destructor uses `delete[]` on `data_`. The copy constructor allocates a separate array with the same capacity and copies each valid `Message`. The copy assignment operator first creates a new array and copies the other conversation into it. Only after that succeeds does it delete the old array and replace the pointer. This also handles self-assignment with `if (this != &other)`.

The move constructor and move assignment operator do not copy each `Message`. Instead, they take the other object's `data_`, `size_`, and `capacity_`. Then they set the source object's pointer to `nullptr` and set its size and capacity to 0. This makes the moved-from object valid and safe to destroy. My tests check that copies have different `begin()` addresses while moves keep the original buffer address and empty the source object. AddressSanitizer is also enabled by the provided CMake file, so leaks and invalid frees should be detected while the tests run.

## Sentinel scanner: bounded pending_ proof

The sentinel scanner has to detect `<|end_conversation|>` even if it is split between chunks. Let the sentinel length be `m`. After each call to `feed()`, my scanner keeps at most the final `m - 1` characters in `pending_`. It combines the old pending text with the new chunk and first searches that combined text for the full sentinel. If the sentinel is found, everything before it is safe output and the sentinel is reported.

If the sentinel is not found, only a suffix of the combined text could possibly become the beginning of a sentinel after the next chunk arrives. A full sentinel has length `m`, so without already having a match, at most `m - 1` trailing characters need to be saved. If the combined text is longer than `m - 1`, the earlier characters cannot be needed for a future match and are returned as safe text. Therefore after every call, `pending_.size()` is at most `m - 1`. This bound does not depend on how large the whole model response becomes, which prevents the scanner from storing the entire stream.

## What I would change differently

If I were extending this project, I would consider using a prefix-matching algorithm such as KMP for the sentinel scanner. The current approach is easier to understand and meets the required memory bound, which is why I used it for this assignment. KMP would be more efficient for very repetitive adversarial input because it can reuse information about earlier partial matches instead of repeatedly searching the small combined string. I would also add non-const element access to `Conversation` only if a later project actually needed it, instead of expanding the interface before there is a reason.
