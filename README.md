# KERNEL-SPACE MATHEMATICAL ENGINE (KSME)

A high-performance, zero-dependency 3D visualization suite engineered from first principles for the Linux Direct Rendering Manager (DRM/KMS). KSME bypasses user-space display servers (X11/Wayland) to interface directly with the kernel's memory-mapped hardware buffers.

## 1. ARCHITECTURAL OVERVIEW

KSME is built entirely from scratch in C++17. It contains no external library dependencies (libc and the Linux kernel headers are the only requirements), ensuring a minimal binary footprint and direct hardware proximity.

### 1.1 Hardware Display Subsystem (DRM/KMS)
- **Direct Device Multiplexing**: Native `/dev/dri/cardX` ioctl interfacing for hardware resource discovery.
- **Memory Architecture**: Implementation of "Dumb Buffer" allocation via `DRM_IOCTL_MODE_CREATE_DUMB` and shared memory mapping (`mmap`).
- **Atomic Page-Flipping**: Frame synchronization utilizing `DRM_MODE_PAGE_FLIP_EVENT` to achieve VBlank-locked, flicker-free rendering.
- **Deep TTY Restoration**: A multi-stage cleanup pipeline that restores the Virtual Terminal from `KD_GRAPHICS` to `KD_TEXT`, performs a programmatic TTY switch (chvt) to force kernel redraw, and relinquishes DRM mastership.

### 1.2 Recursive Expression Parser
- **LL(1) Grammar**: A hand-rolled, recursive descent parser providing runtime evaluation of arbitrary UTF-8 mathematical strings.
- **AST Compilation**: Converts infix notation into a functional Abstract Syntax Tree (AST) for real-time vertex hot-swapping.
- **Mathematical Support**: Full support for standard arithmetic, power operations (`^`), trigonometric functions (`sin`, `cos`, `tan`), and absolute/sqrt operators.
- **Variable Injection**: Injects spatial ($x, y$) and temporal ($t$) variables directly into the evaluation loop.

### 1.3 3D Rendering Pipeline
- **Parallel Rasterization**: Multi-threaded edge-walking and vertex sampling utilizing `std::async` and CPU-level hardware concurrency.
- **Transformation Pipeline**: 4x4 homogeneous matrix operations implementing World-View-Projection (WVP) transforms with perspective-correct division.
- **Normal Estimation**: Real-time surface normal calculation utilizing central-difference approximation ($n = \nabla f$) for any arbitrary parametric manifold.
- **Vertex Shading**: Two-sided Lambertian lighting model with dynamic ambient-diffuse blending.
- **Procedural Color Mapping**: Three-dimensional color interpolation based on height, origin-radial distance, and temporal oscillation.

### 1.4 Geometry & Manifolds
- **Parametric Engine**: Supports non-Euclidean geometry and complex 3D manifolds defined by $(u, v)$ domains.
- **Preset Registry**: Includes high-fidelity implementations of a Torus, Möbius Strip, Klein Bottle, and complex harmonic surfaces ("The Beast").
- **1D Line Plotting**: High-resolution $(0.02\Delta)$ 2D function plotter within 3D space, featuring automated asymptote detection to prevent line-bleeding on division-by-zero.
- **Coordinate System**: Renders a dedicated X (Red), Y (Green), and Z (Blue) axis system with a calibrated floor grid for spatial orientation.

### 1.5 Input & Telemetry
- **Asynchronous evdev**: Multi-threaded scanning of `/dev/input/event*` devices for low-latency keyboard and mouse state tracking.
- **Vector HUD**: A custom software-scaled font engine with hand-encoded glyph definitions for real-time telemetry (FPS, active formula, state data).

## 2. SYSTEM REQUIREMENTS
- **Platform**: Linux Kernel 4.15+ (Arch Linux recommended).
- **Permissions**: Root privileges required for `DRM_IOCTL_SET_MASTER`.
- **Environment**: Raw TTY session (Standalone Virtual Console).

## 3. BUILD & EXECUTION
The engine utilizes a minimalist `Makefile` for high-throughput compilation.

```bash
make -j$(nproc)
sudo ./record.sh
```

## 4. INTERFACE SPECIFICATION
| INPUT | STATE | ACTION |
| :--- | :--- | :--- |
| `1 - 4` | Navigation | Manifold Preset Transition |
| `5` | Navigation | Coordinate Plane Sandbox |
| `6` | Navigation | "The Beast" Harmonic Manifold |
| `WASD` | Navigation | Linear Camera Translation |
| `Mouse` | Navigation | Angular Camera Rotation |
| `M` | Navigation | Toggle Mesh/Line Modality (Preset 5) |
| `I` | Navigation | Initialize AST Input Buffer |
| `CTRL+X` | Input | Compile AST and hot-swap mesh logic |
| `ESC` | Any | Process Termination / Cancel Input |
