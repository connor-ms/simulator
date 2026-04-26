/*
 * This file is included by default when loading a shader
 * via Util::loadShaderModule(). This prevents the need to
 * redefine these structs in each file, at the expense
 * of syntax highlighting not working.
 */

struct Globals
{
    mousePos: vec2<f32>,    // 0 - 8
    isMouseDown: i32,       // 8 - 12
    _pad: i32,              // 12 - 16
    gridSize: vec4<u32>,    // 16 - 32
    dt: f32,                // 32 - 36
    particleCount: u32,     // 36 - 40
    gravity: f32,           // 40 - 44
    rest_density: f32,      // 44 - 48
    dynamic_viscosity: f32, // 48 - 52
    eos_stiffness: f32,     // 52 - 56
    eos_power: f32,         // 56 - 60
    _pad2: f32,             // 60 - 64
};

struct Particle
{
    position: vec3f,
    velocity: vec3f,
    C: mat3x3f,
    debug1: f32,
    debug2: f32,
    @size(8) _pad: vec2f,
}

struct GridNode
{
    vX: atomic<i32>,
    vY: atomic<i32>,
    vZ: atomic<i32>,
    mass: atomic<i32>,
}

fn toFixed(value: f32) -> i32 {
    return i32(value * 1e7);
}

fn toFloat(value: i32) -> f32 {
    return f32(value) * 1e-7;
}