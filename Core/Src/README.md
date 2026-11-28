# STM32 source files

This folder contains cleaned versions of all 18 uploaded C files. The uploaded
`main(2).c` is named `main.c` here.

## Use in STM32CubeIDE

1. Extract `src.zip`.
2. Copy the C files from `src` into your existing project's source directory,
   normally `Core/Src`, replacing the corresponding files.
3. Keep your existing project headers, HAL/CMSIS drivers, startup file, linker
   script, and build settings.
4. Build the existing project in STM32CubeIDE.

Use the supplied `main.c` as the replacement for your current main source file.
Only one file defining `main()` should be included in the build.

## What was cleaned up

- Consistent four-space indentation, brace placement, and operator spacing.
- Explicit braces around conditional branches.
- Descriptive names for private state, local variables, and scheduler fields.
- English comments and brief documentation for the application modules.
- Named constants for automatic-mode timers and I2C probe settings.
- Heartbeat implementation placed in the appropriate CubeMX user-code section.
- Explicit `<stddef.h>` include for `NULL` and `ptrdiff_t` in `sysmem.c`.
- Explicit `(void)` parameter list for `initialise_monitor_handles`.
- LCD line helpers retain the public `LcdLine_t.buf` member from `lcd.h`.

Public function names and shared global names are retained for compatibility
with your existing headers. The original algorithms, branch order, task
registration order, timer values, state transitions, LCD text, I2C operations,
GPIO settings, and clock configuration are preserved.

## Validation

Parsed source structure was compared with the originals after accounting for
private identifier changes and added conditional braces. All 18 files passed,
covering 136 function definitions and 24 preprocessor configurations. CubeMX
`USER CODE` markers were also checked.

All 12 application modules also passed GCC C11 syntax checks with warnings
treated as errors, using the uploaded project headers unchanged. Temporary HAL
declarations were used only for this host check and are excluded from this ZIP.
The check covers header declarations, function signatures, and struct members;
it does not validate the STM32 HAL implementation or hardware behavior.

This is a source-folder replacement, not a complete standalone CubeIDE project.
A target build and hardware run were not performed because the HAL/CMSIS
drivers and complete build configuration were not included in the uploads.
