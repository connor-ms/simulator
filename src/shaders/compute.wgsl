@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read_write> particles: array<Particle>;

@compute @workgroup_size(64)
fn computeSomething(@builtin(global_invocation_id) id: vec3<u32>) {
    let i = id.x;

    if (particles[i].position.y > -globals.worldSize.y) {
        particles[i].position.y += -1;
    } else {
        particles[i].position.y = globals.worldSize.y;
    }
}