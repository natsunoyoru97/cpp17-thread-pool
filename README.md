# cpp17-thread-pool
A simple C++17 thread pool implementation, which is inspired from [progschj's work](https://github.com/progschj).

Note: The threadsafe queue has a slightly different returned value if you intend to call `front()`, `back()` and `TryPop()`. It is a `std::optional<T>` value rather than a `T` value or a `bool`.

