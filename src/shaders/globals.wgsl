/*
 * This file is included by default when loading a shader
 * via Util::loadShaderModule(). This prevents the need to
 * redefine these structs in each file, at the expense
 * of syntax highlighting not working.
 */

struct Globals
{
    mousePos: vec2<f32>,
    isMouseDown: i32,
    _pad2: i32,
    gridSize: u32,
    dt: f32,
    particleCount: u32,
    gravity: f32,
    rest_density: f32,
    dynamic_viscosity: f32,
    eos_stiffness: f32,
    eos_power: f32,
};

struct Particle
{
    position: vec2f,
    velocity: vec2f,
    C: mat2x2f,
    debug1: f32,
    debug2: f32,
}

struct GridNode
{
    vX: atomic<i32>,
    vY: atomic<i32>,
    mass: atomic<i32>,
}

fn toFixed(value: f32) -> i32 {
    return i32(value * 1e7);
}

fn toFloat(value: i32) -> f32 {
    return f32(value) * 1e-7;
}