# 09 — Testing

## Framework

- **PlatformIO** with **Unity** test framework
- Native tests run on macOS (no hardware needed)
- Test env: `[env:native]` in `platformio.ini`

## Running Tests

```bash
# All native tests
pio test -e native

# Single test suite
pio test -e native -f test_crc
```

## Architecture

### Pure logic extraction

All testable logic lives in `lib/ns_pure_logic/`:
- `ns_pure_logic.h` — declarations with `extern "C"` linkage
- `ns_pure_logic.cpp` — implementations using only standard C/C++ types

Rules for `ns_pure_logic`:
- **No Arduino headers** (`Arduino.h`, `M5Stack.h`, `WiFi.h`, etc.)
- **No Arduino types** (`String`, `IPAddress`, etc.) — use `char*`, `uint8_t[]`
- **Standard C types only** (`uint16_t`, `size_t`, `const char*`, etc.)
- Compiles on both ESP32 (via PlatformIO) and host (native)

### What goes in ns_pure_logic

- CRC calculations
- String parsing and validation
- Data format conversions
- Configuration parsing helpers
- Any pure function: same inputs → same outputs, no side effects

### What stays in cores3/src

- Anything touching hardware (M5.Display, WiFi, SD, I2S)
- Functions using Arduino-specific types as primary interface
- setup/loop, web server, display drawing, alarm playback

## Test file layout

```
test/
  native/
    test_crc/
      test_crc.cpp
    test_direction/
      test_direction.cpp
    ...
```

Each test suite goes in its own subdirectory under `test/native/`.

## TDD workflow

1. Write the test first (RED — it should fail or not compile)
2. Implement the function in the matching `lib/` module (GREEN — tests pass)
3. Replace inline logic in `cores3/src/` with a call to it
4. Verify device build: `cd cores3 && pio run`

## Conventions

- Test function names: `test_<function>_<scenario>` (e.g., `test_calcCRC_empty_string`)
- Each test file has `setUp()` and `tearDown()` (even if empty — Unity requires them)
- Each test file has its own `main()` with `UNITY_BEGIN()` / `UNITY_END()`
- Use `TEST_ASSERT_EQUAL_*` macros for typed comparisons

## Gotchas and Mistakes to Avoid

### PlatformIO `extends` syntax

**Wrong:** `extends = base_esp32`
**Right:** `extends = env:base_esp32`

The `env:` prefix is required. Without it, PlatformIO throws `'No section: base_esp32'`. Same for variable interpolation: use `${env:base_esp32.lib_deps}`, not `${base_esp32.lib_deps}`.

### Test suite directory structure

**Wrong:** Flat files in `test/native/test_crc.cpp`
**Right:** Each suite in its own subdirectory: `test/native/test_crc/test_crc.cpp`

PlatformIO treats each subdirectory as a separate test suite. Flat `.cpp` files with their own `main()` in one directory causes "Nothing to build" errors — PlatformIO can't find them as suites.

### Sharing pure logic with native tests

**Wrong:** `build_src_filter = +<../ns_pure_logic.cpp>` or `-I.` flags to find root-level files.
**Right:** Put shared code in `lib/<name>/` directory (e.g., `lib/ns_pure_logic/`).

PlatformIO auto-discovers `lib/` for all environments including native. No extra config needed. Trying to pull in source files from the repo root via `build_src_filter` or `-I` flags is fragile and doesn't work reliably with relative paths for native builds.

### Arduino String to char* for pure logic

`lib/` must not see Arduino types. At a call site in `cores3/src/` that holds a `String`, use this pattern:

```cpp
{
  size_t jsonLen = json.length();
  char* jsonBuf = new char[jsonLen + 1];
  json.toCharArray(jsonBuf, jsonLen + 1);
  jsonLen = sanitizeJson(jsonBuf, jsonLen);
  json = String(jsonBuf);
  delete[] jsonBuf;
}
```

The block scope ensures `jsonBuf` doesn't leak. The copy back to `String` is necessary because `sanitizeJson` may shorten the buffer (e.g., removing fractional date digits).

### `const char*` tightening

When extracting functions, tighten `char*` params to `const char*` where the function doesn't modify the input (e.g., `calcCRC`). This is safe — `char[]` and `char*` implicitly convert to `const char*`. It catches accidental mutation bugs at compile time.

### Tests that read repository files

`test_sample_ini` parses the real `SD/M5NS.INI` rather than a fixture, so the
sample users are told to copy cannot drift from what the firmware accepts. It
locates the file by trying relative paths and then falling back to a path
derived from `__FILE__`, so it works regardless of the runner's directory.
### Installing PlatformIO without sudo

This machine has no `pip` or `venv` module, so `pip install platformio` and
`python3 -m venv` both fail. `uv` is already on PATH:

```bash
uv tool install platformio
```

That installs `pio` to `~/.local/bin` with no root access required. Do not
reach for `sudo apt install pipx` first.
