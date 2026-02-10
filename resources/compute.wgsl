struct Particle {
    center: vec2<f32>,
    radius: f32,
    _pad: f32,
}

@group(0) @binding(0) var<storage, read_write> particles: array<Particle>;

@compute @workgroup_size(64)
fn computeSomething(@builtin(global_invocation_id) id: vec3<u32>) {
    let i = id.x;
    particles[i].center += vec2<f32>(0.01, 0);
}