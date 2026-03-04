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
    vX: atomic<u32>,
    vY: atomic<u32>,
    mass: atomic<u32>,
}