# Math Visualizer

A pure C++ 3D graphics engine and mathematical function visualizer built from scratch with zero external dependencies.

## Features
- **Raw Framebuffer Rendering**: Writes directly to `/dev/fb0`.
- **Software Double-Buffering**: Flicker-free rendering using a manual backbuffer.
- **3D Engine**: Perspective projection, 4x4 matrix transformations, and 3D to 2D vertex mapping.
- **Bresenham's Line Algorithm**: Optimized integer-based line drawing.
- **Raw Input**: Direct reading of `/dev/input/event*` for keyboard handling.

## Build & Run
```bash
make
sudo ./math_visualizer
```

## Requirements
- Linux (running in a TTY or a non-DRM-locked session).
- Access to `/dev/fb0` and `/dev/input/event*`.
