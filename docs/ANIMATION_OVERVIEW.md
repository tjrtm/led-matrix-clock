# Animation Architecture Overview

## Existing Clock Pipeline Analysis
- **Rendering Flow:** `src/main.cpp` renders the clock into a 64x32 off-screen `RenderTexture2D` before mirroring every pixel to the LED matrix. The loop begins with weather polling, time/temperature formatting, and UI drawing (time, weather, temperature trend) followed by a brightness reduction step for night and manual dimming.
- **Update Cadence:** When the hardware shim is active the window targets 30 FPS, otherwise 5 FPS. `GetFrameTime()` is the canonical delta between frames.
- **Matrix Output:** After drawing, the texture is copied pixel-by-pixel through `MatrixDriver::writePixel()` then flushed with `flipBuffer()`.
- **Extensibility Points:** Any animation must render into the same 64x32 texture and respect brightness modifiers so the matrix hardware path stays untouched.

## Animation Strategy
- **Reusable Base Class:** A common `Animation` interface (reset, update, draw) encapsulates per-frame logic while sharing width/height context.
- **Universal Algorithms:**
  - Color-space cycling (HSV based) for smooth gradients.
  - Procedural particles (Matrix rain, starfield) driven by deterministic RNG for speed.
  - Trigonometric fields (swirl, wave lines) for fluid patterns without texture assets.
  - Simple physics (bouncing balls, pulse squares) using vector math.
  - Cellular automata (fire) and stochastic sparkles for organic motion.
  - Text-based motion using existing font rendering (scrolling banner).
- **Performance Considerations:** The algorithms rely on lightweight math (sine/cosine, random updates, basic loops) and reuse static allocations where possible to stay responsive on both the shim and physical hardware.

## Implementation Plan
1. **Introduce Animation Framework** – Create reusable classes under `src/animations/` with ten animation implementations and a manager that controls lifecycle, timing, and render delegation.
2. **Add HTTP Control Surface** – Embed a lightweight REST server (`cpp-httplib`) to accept animation requests and expose the catalogue.
3. **Integrate With Main Loop** – Instantiate the manager/server, update each frame, and branch the render path between clock UI and animation output while keeping night/dim overlays intact.
4. **State Management** – Ensure animations run for eight seconds, then automatically revert to the default clock view without disrupting ongoing weather/temperature updates.
5. **Documentation & Testing** – Describe the API endpoints, payloads, and available animations for external triggers.

## REST API Usage
- **List animations**
  - `GET /api/animations`
  - Response: `{ "animations": ["rainbow_cycle", "matrix_rain", ...] }`
- **Trigger an animation**
  - `POST /api/animations/run`
  - Body: `{ "animation": "fire" }`
  - Success: `{ "status": "accepted", "animation": "fire", "duration_ms": 8000 }`
  - Errors return HTTP 400 (bad payload) or 404 (unknown animation) with diagnostic JSON.
- **Timing:** Each accepted request interrupts the clock for eight seconds before the manager automatically fades back to the regular display.

The server listens on port `8080` and is available while the application is running.
