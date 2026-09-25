# 09 — Testing

## Framework

- **PlatformIO** with **Unity** test framework
- Native tests run on the Linux host (no hardware needed)
- Test env: `[env:native]` in `platformio.ini`

## Running Tests

```bash
# All native tests
pio test -e native

# Sanitizers and static analysis, as CI runs them
pio test -e native_asan
TSAN_OPTIONS=halt_on_error=1 pio test -e native_tsan
pio check -e native --fail-on-defect=medium

# Coverage, gated in CI at 95% line and 78% branch over lib/
pio test -e native_coverage && gcovr --root . --filter 'lib/' --print-summary

# Single test suite
pio test -e native -f native/test_units
```

## Architecture

### Pure logic lives in lib/

Every testable piece lives in a `lib/<module>/` directory; see
`rules/03-project-structure.md` for the list. Each module:
- **No Arduino headers** (`Arduino.h`, `M5Unified.h`, `WiFi.h`, etc.)
- **No Arduino types** (`String`, `IPAddress`, etc.) — use `char*`, `uint8_t[]`
- **Standard C/C++ only**, `extern "C"` for plain functions; `ns_shared` is
  header-only C++ templates
- Compiles on both ESP32 (via PlatformIO) and host (native)

### What stays in cores3/src

- Anything touching hardware (M5.Display, WiFi, SD, I2S)
- Functions using Arduino-specific types as primary interface
- setup/loop, task bodies, web server handlers, display drawing, alarm playback

## Test file layout

```
test/
  native/
    test_units/
      test_units.cpp
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

- Test function names: `test_<function>_<scenario>` (e.g., `test_direction_TripleUp`)
- Each test file has `setUp()` and `tearDown()` (even if empty — Unity requires them)
- Each test file has its own `main()` with `UNITY_BEGIN()` / `UNITY_END()`
- Use `TEST_ASSERT_EQUAL_*` macros for typed comparisons

## Gotchas and Mistakes to Avoid

### PlatformIO `extends` syntax

**Wrong:** `extends = m5stack-cores3`
**Right:** `extends = env:m5stack-cores3`

The `env:` prefix is required. Without it PlatformIO 6.2 does not error: the
env silently inherits nothing and builds with defaults. Same for interpolation:
`${env:m5stack-cores3.build_flags}`.

### Test suite directory structure

**Wrong:** Flat files in `test/native/test_units.cpp`
**Right:** Each suite in its own subdirectory: `test/native/test_units/test_units.cpp`

PlatformIO treats each subdirectory as a separate test suite. Flat `.cpp` files with their own `main()` in one directory causes "Nothing to build" errors — PlatformIO can't find them as suites.

### Sharing pure logic with native tests

**Wrong:** `build_src_filter = +<../ns_pure_logic.cpp>` or `-I.` flags to find root-level files.
**Right:** Put shared code in `lib/<name>/` directory (e.g., `lib/ns_pure_logic/`).

The root `platformio.ini` discovers `lib/` automatically; `cores3/` reaches it through `lib_extra_dirs = ../lib`. Trying to pull in source files from the repo root via `build_src_filter` or `-I` flags is fragile and doesn't work reliably with relative paths for native builds.

### Arduino String to char* for pure logic

`lib/` must not see Arduino types. At a `cores3/src/` call site holding a
`String`, copy into a `char` buffer sized from `length() + 1` and pass that. See
`httpGetSanitized()` in `nightscout.cpp`: it copies the response body, lets
`sanitizeJson()` shorten it in place, and hands the buffer and new length to the
parser.

### `const char*` tightening

When extracting functions, tighten `char*` params to `const char*` where the function doesn't modify the input (e.g., `directionToAngle`). This is safe — `char[]` and `char*` implicitly convert to `const char*`. It catches accidental mutation bugs at compile time.

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
