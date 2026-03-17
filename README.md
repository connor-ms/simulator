# simulator

A cross platform C++ fluid simulation using WebGPU. Uses MLS-MPM to perform the simulation.

## Requirements

- cmake
- emscripten (if building for web)

```
brew install cmake emscripten
```

## Setup

```
git clone https://github.com/connor-ms/simulator.git
cd simulator
git submodule update --init
```

## Building

```
mkdir build
cmake -B build
cmake --build build -j4
./build/app
```

### Building for web

```
mkdir build-web
emcmake cmake -B build-web
cmake --build build-web -j4
npx http-server
```

Then open http://127.0.0.1:8080 in your browser and navigate to `build-web/app.html`.

Note that building for web has not been tested recently. This has primarily been tested on Mac, but _should_ work on other platforms/web.
