@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read_write> particles: array<Particle>;
@group(1) @binding(1) var<storage, read_write> grid: array<GridNode>;

@compute @workgroup_size(64)
fn p2g(@builtin(global_invocation_id) id: vec3<u32>) {
    let pid = id.x;

    if (pid >= globals.particleCount) {
        return;
    }

    let p = particles[pid];

    // Convert particle position to grid space
    let cell = p.position * globals.idX;// floor(p.position * globals.idX);
    let base = vec2<f32>(floor(cell - 0.5));
    let fx = cell - vec2<f32>(base);

    // Quadratic B-spline weights
    let w = array<vec2<f32>, 3>(
        0.5 * pow(1.5 - fx, vec2<f32>(2.0)),
        0.75 - pow(fx - 1.0, vec2<f32>(2.0)),
        0.5 * pow(fx - 0.5, vec2<f32>(2.0))
    );

    // Loop over 3x3 neighboring grid nodes
    for (var i = 0; i < 3; i++) {
        for (var j = 0; j < 3; j++) {

            let weight = w[i].x * w[j].y;

            let gx = base.x + f32(i);
            let gy = base.y + f32(j);

            if (gx < 0 || gy < 0 ||
                gx >= f32(globals.gridSize) ||
                gy >= f32(globals.gridSize)) {
                continue;
            }

            let gridIndex = u32(gy) * globals.gridSize + u32(gx);
            let dx = (vec2<f32>(gx, gy) + 0.5) * globals.dX - p.position;

            let momentum = 1 * weight * (p.velocity + p.C * dx);

            atomicAdd(&grid[gridIndex].mass, toFixed(weight * 1));
            atomicAdd(&grid[gridIndex].vX, toFixed(momentum.x));
            atomicAdd(&grid[gridIndex].vY, toFixed(momentum.y));
        }
    }
}