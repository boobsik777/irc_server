# irc_server

## Changelog

### 11.8.2026:

serverInit()
    -Added error handling and return code checks for bind() and listen() failures

chanAdd()
    -Optimized list insertion to head insertion
    -Added duplicate channel name check
    -Added NULL check to handle failed memory allocations

chanInit()
    -Replaced strlcpy() with snprintf()

clientAdd()
    -Optimized list insertion to head insertion
    -Added duplicate checks 
    -Replaced strlcpy() with snprintf()

clientDel()
    -Fixed NULL pointer dereference crash when target client is missing
    -Fixed logic bug where deleting the last node in the list failed
    -Added break statement inside channel array cleanup loop to skip redundant iterations

clientFree()
    -Added close() calls to release socket file descriptors before freeing memory

broadcastMessage()
    -Fixed NULL pointer dereference crash when sender is NULL
    -Added member_count counter to break loop early once all active members are processed
    -Added buffer truncation checks to prevent string overflow

