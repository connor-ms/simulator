struct Particle {
    center: vec2<f32>,
    radius: f32,
    _pad: f32,
}

struct Globals
{
    height: f32,
    width: f32,
    _pad1: f32,
    _pad2: f32
};

@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read_write> particles: array<Particle>;

@compute @workgroup_size(64)
fn computeSomething(@builtin(global_invocation_id) id: vec3<u32>) {
    let i = id.x;

    if (particles[i].center.y > -1) {
        particles[i].center.y += -0.01;
    } else {
        particles[i].center.y = 1;
    }
}