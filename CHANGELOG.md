
# Change Log

## [Unreleased] - 28-9-2026

### Added

- **struct client**: Added `ip` buffer field to store client IP addresses
- **clientFind()**: Implemented lookup helper to retrieve client node pointers by file descriptor
- **main()**: Added incoming connection handling via `accept()`, IP string extraction using `inet_ntop()`, and automatic client registration as `"Guest"`

### Changed

- **clientAdd()**: Updated function signature to accept IP address string argument (`const char *ip`)
- **clientAdd()**: Added IP address storage logic with a default fallback to `"0.0.0.0"` when `NULL`

### Fixed

## [Unreleased] - 10-8-2026

### Added

- **serverInit()**: Added error handling and return code checks for `bind()` and `listen()` failures
- **chanAdd()**: Added duplicate channel name check
- **chanAdd()**: Added `NULL` check to handle failed memory allocations
- **clientAdd()**: Added duplicate checks for client socket file descriptors and usernames
- **clientDel()**: Added `break` statement inside channel array cleanup loop to skip redundant iterations
- **clientFree()**: Added `close()` calls to release socket file descriptors before freeing memory
- **broadcastMessage()**: Added `member_count` counter to break loop early once all active members are processed
- **broadcastMessage()**: Added buffer truncation checks to prevent string overflow

### Changed

- **chanAdd()**: Optimized list insertion to head insertion
- **chanInit()**: Replaced `strlcpy()` with `snprintf()`
- **clientAdd()**: Optimized list insertion to head insertion
- **clientAdd()**: Replaced `strlcpy()` with `snprintf()`

### Fixed

- **clientDel()**: Fixed `NULL` pointer dereference crash when target client is missing
- **clientDel()**: Fixed logic bug where deleting the last node in the list failed
- **broadcastMessage()**: Fixed `NULL` pointer dereference crash when sender is `NULL`
