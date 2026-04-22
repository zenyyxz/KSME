# MATH VISUALIZER

A high-performance, zero-dependency 3D mathematical surface engine targeting the Linux Direct Rendering Manager (DRM/KMS). This software bypasses X11/Wayland to interface directly with the kernel's display subsystem for maximum throughput and minimal latency.

## Technical Specifications
- **Display Backend**: DRM/KMS with double-buffering and atomic page-flipping.
- **Rendering Pipeline**: Custom software rasterizer with multi-threaded grid evaluation.
- **Lighting Model**: Dynamic Lambertian shading with central-difference normal estimation.
- **Math Engine**: Recursive descent expression parser for runtime formula evaluation.
- **Input System**: Raw evdev event processing for asynchronous keyboard/mouse state management.

## Prerequisites
- Linux kernel 4.15+ with DRM support.
- Root or `video` group permissions for `/dev/dri/card*` and `/dev/input/event*`.
- Standard C++17 compiler (GCC 9+ recommended).

## Build and Execution
Execution requires a standalone TTY environment. Ensure all display managers (GDM, SDDM, Hyprland) are terminated before launch.

```bash
make clean && make
sudo ./math_visualizer
```

## Interaction Model
### Global Navigation
- **WASD**: Camera translation.
- **Mouse**: Camera rotation (Spherical coordinates).
- **1 - 4**: Fixed parametric presets (Ripple, Torus, Mobius, Klein).
- **5**: Mathematical Sandbox (Coordinate Plane).
- **6**: "The Beast" (Complex harmonic surface).
- **ESC**: Process termination.

### Sandbox Mode (Preset 5)
- **'i'**: Enter expression input mode.
- **'m'**: Toggle between 3D Surface Mesh and High-Resolution 1D Line Plot.
- **CTRL+X**: Compile and apply expression.
- **ESC**: Discard input.

## Expression Parser
The runtime parser supports the following tokens:
- **Variables**: `x`, `y` (spatial), `t` (temporal).
- **Operators**: `+`, `-`, `*`, `/`, `^` (power).
- **Functions**: `sin(a)`, `cos(a)`, `tan(a)`, `sqrt(a)`, `abs(a)`.
- **Nesting**: Infinite recursion via parentheses `(...)`.
