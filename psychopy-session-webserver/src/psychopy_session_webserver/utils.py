from typing import Tuple


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


def convertToPsychopy(color: str) -> Tuple[float, float, float]:
    if color.startswith("#") == False:
        raise RuntimeError("wrong input format")
    color = color[1:]
    if len(color) % 3 != 0 or len(color) == 0:
        raise RuntimeError("wrong input format")
    incr = len(color) // 3
    base = 16**incr - 1
    print(base)
    return tuple(
        2 * int(color[i : i + incr], 16) / base - 1.0
        for i in range(0, len(color), incr)
    )
