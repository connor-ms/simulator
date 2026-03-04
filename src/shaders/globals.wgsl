struct Globals
{
    windowSize: vec2<f32>,
    worldSize: vec3<f32>,
    proj: mat4x4<f32>,
    gridSize: u32,
    dX: f32,
    idX: f32,
    dt: f32,
    particleMass: f32,
    particleCount: u32,
};

struct Particle
{
    position: vec2f,
    velocity: vec2f,
    C: mat2x2f,
    J: f32,
}

struct GridNode
{
    vX: atomic<i32>,
    vY: atomic<i32>,
    mass: atomic<i32>,
}

// note: will save 2 decimal places
fn toFixed(value: f32) -> i32 {
    return i32(value * 1e2);
}

fn toFloat(value: i32) -> f32 {
    return f32(value) * 1e-2;
}