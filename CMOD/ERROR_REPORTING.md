# CMOD error reporting

CMOD writes diagnostics to standard error. LASSIE's Process Output window
already displays that stream and marks failed processes in red.

| Exit code | Meaning |
| --- | --- |
| 0 | The requested build completed. |
| 1 | A diagnosed project-input or output failure. |
| 2 | An unexpected C++ exception or a diagnosed internal error. |

For example, a missing configuration field produces:

```text
CMOD project error: A required project setting is missing.
Project: Example.dissco
Context: ProjectConfiguration.NumberOfChannels
Suggestion: Restore this setting in Project Properties, then save the project in LASSIE.
Build failed.
```

Project diagnostics cover unreadable or malformed project XML, missing or invalid
configuration, invalid numeric expressions and nested functions, missing object
references, invalid Select indices, functions used without an event context,
CURRENT_TYPE or CURRENT_LAYER in a child Start Time, invalid child counts, and
empty score staffs. Output diagnostics cover directory creation, audio/score
writes, and LilyPond failures.
Unexpected C++ exceptions ask the user to send developers the project, seed,
and diagnostic. Hard process faults and remaining legacy direct-exit paths
retain their existing handling; they are not made recoverable by these exceptions.

## Adding a diagnostic

Throw `CmodError` with a category, a specific reason, input context, and a
corrective action. Add outer context while rethrowing at a boundary that knows
the project field or expression. Do not discard an underlying parser's reason,
or classify an arbitrary `std::exception` as a user-input error.

`Main.cpp` reports the exception once and returns a nonzero exit code. A failed
run must not reach its `Build complete.` message.
