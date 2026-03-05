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
    let base = vec2<f32>(floor(cell - 0.5));
    let fx = cell - vec2<f32>(base);

    // Quadratic B-spline weights
    let w = array<vec2<f32>, 3>(
        0.5 * pow(1.5 - fx, vec2<f32>(2.0)),
        0.75 - pow(fx - 1.0, vec2<f32>(2.0)),
        0.5 * pow(fx - 0.5, vec2<f32>(2.0))
    );

    var newVelocity = vec2<f32>(0.0);
    var newC = mat2x2<f32>(vec2<f32>(0.0), vec2<f32>(0.0));

    // Interpolate from 3x3 neighbors
    for (var i = 0; i < 3; i++) {
        for (var j = 0; j < 3; j++) {

            let gx = base.x + f32(i);
            let gy = base.y + f32(j);

            if (gx < 0 || gy < 0 ||
                gx >= f32(globals.gridSize) ||
                gy >= f32(globals.gridSize)) {
                continue;
            }

            let weight = w[i].x * w[j].y;
            let index = u32(gy) * globals.gridSize + u32(gx);

            //let gridVel = grid[index].velocity;
            let gvX = toFloat(atomicLoad(&grid[index].vX));
            let gvY = toFloat(atomicLoad(&grid[index].vY));
            let gridVel = vec2f(gvX, gvY);

            let dx = (vec2<f32>(gx, gy) + 0.5) * globals.dX - p.position;

            newVelocity += weight * gridVel;

            newC += 4.0 * globals.idX * weight * mat2x2<f32>(
                gridVel.x * dx.x, gridVel.x * dx.y,
                gridVel.y * dx.x, gridVel.y * dx.y
            );
        }
    }

    // Update particle velocity
    p.velocity = newVelocity;
    p.C = newC;
    p.J *= (1.0 + globals.dt * (p.C[0][0] + p.C[1][1]));

    // Advect particle
    p.position += globals.dt * p.velocity;

    p.position = clamp(
        p.position,
        vec2<f32>(0.0),
        vec2<f32>(globals.worldSize.x, globals.worldSize.y)
    );

    particles[pid] = p;
}