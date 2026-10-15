# Settings tests

Run `sh tests/settings/run.sh` with a C++17 compiler on Linux. The runner copies
the real Preferences.h/iniConfig.cpp into /tmp, compiles against a minimal
MainContext stub, and runs unprivileged (uses `runuser` when launched as root).
No SDL, ImGui, target hardware or third-party test framework is required.

Covers the shared singleton across translation units, dirty/no-op saves,
legacy keys, all added MainContext preferences, strict numeric validation,
finite doubles, lossless string escaping/Unicode, unknown-key preservation,
scoped dashboard identities, unreadable-file overwrite prevention,
temporary-file and atomic-replacement failures, cleanup, and successful retry.
The Windows replacement path requires a Windows build/runtime for validation.
