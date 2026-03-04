@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read_write> particles: array<Particle>;
@group(1) @binding(1) var<storage, read_write> grid: array<GridNode>;

@compute @workgroup_size(64)
fn g2p(@builtin(global_invocation_id) id : vec3<u32>) {
    let pid = id.x;

    if (pid >= globals.particleCount) {
        return;
    }

    var p = particles[pid];

    // Convert to grid space
    let cell = p.position * globals.idX;
    let base = vec2<i32>(floor(cell - 0.5));
    let fx = cell - vec2<f32>(base);

    // Quadratic B-spline weights
    let w = array<vec2<f32>, 3>(
        0.5 * pow(1.5 - fx, vec2<f32>(2.0)),
        0.75 - pow(fx - 1.0, vec2<f32>(2.0)),
        0.5 * pow(fx - 0.5, vec2<f32>(2.0))
    );

    var newVelocity = vec2<f32>(0.0);

    // Interpolate from 3x3 neighbors
    for (var i = 0; i < 3; i++) {
        for (var j = 0; j < 3; j++) {

            let gx = base.x + i;
            let gy = base.y + j;

            if (gx < 0 || gy < 0 ||
                gx >= i32(globals.gridSize) ||
                gy >= i32(globals.gridSize)) {
                continue;
            }

            let weight = w[i].x * w[j].y;
            let index = u32(gy) * globals.gridSize + u32(gx);

            //let gridVel = grid[index].velocity;
            let gvX = toFloat(atomicLoad(&grid[index].vX));
            let gvY = toFloat(atomicLoad(&grid[index].vY));
            let gridVel = vec2f(gvX, gvY);

            newVelocity += weight * gridVel;
        }
    }

    // Update particle velocity
    p.velocity = newVelocity;

    // Advect particle
    p.position += globals.dt * p.velocity;

    // Optional: clamp to domain
    // p.position = clamp(
    //     p.position,
    //     vec2<f32>(0.0),
    //     vec2<f32>(1.0)
    // );

    particles[pid] = p;
}