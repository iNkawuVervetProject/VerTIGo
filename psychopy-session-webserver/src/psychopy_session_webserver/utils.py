def format_ns(d: int):
    if d == 0:
        return "0s"
    if abs(d) < 1_000:
        return f"{d}ns"
    if abs(d) < 1_000_000:
        return f"{d/1000}µs"
    if abs(d) < 1_000_000_000:
        return f"{d/1_000_000}ms"
    if abs(d) < 60 * 1_000_000_000:
        return f"{d / 1_000_000_000}s"

    seconds = d % (60 * 1_000_000_000)
    d = d - seconds
    d //= 60 * 1_000_000_000
    seconds /= 1_000_000_000
    if abs(d) < 60:
        return f"{d}m{seconds:g}s"

    minutes = d % 60
    hours = d // 60
    return f"{hours}h{minutes}m{seconds:g}s"
