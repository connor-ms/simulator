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

    let gridPos = p.position * globals.idX;
    let base = vec2<i32>(gridPos);
    let fx = gridPos - vec2<f32>(base) - vec2<f32>(0.5);

    // Quadratic B-spline weights
    var w: array<vec2<f32>, 3>;
    w[0] = 0.5 * pow(vec2<f32>(0.5) - fx, vec2<f32>(2.0));
    w[1] = vec2<f32>(0.75) - fx * fx;
    w[2] = 0.5 * pow(vec2<f32>(0.5) + fx, vec2<f32>(2.0));

    p.velocity = vec2<f32>(0.0);
    var B = mat2x2<f32>();

    // Interpolate from 3x3 neighbors
    for (var i = 0; i < 3; i++) {
        for (var j = 0; j < 3; j++) {

            let cell = base + vec2<i32>(i - 1, j - 1);

            if (cell.x < 0 || cell.y < 0 ||
                cell.x >= i32(globals.gridSize) ||
                cell.y >= i32(globals.gridSize)) {
                continue;
            }

            let weight = w[i].x * w[j].y;
            let index = u32(cell.y) * globals.gridSize + u32(cell.x);

            let gvX = toFloat(atomicLoad(&grid[index].vX));
            let gvY = toFloat(atomicLoad(&grid[index].vY));
            let gridVel = vec2<f32>(gvX, gvY);

            let dist = (vec2<f32>(cell) - gridPos) + vec2<f32>(0.5);

            p.velocity += weight * gridVel;
            
            let term = mat2x2<f32>(
                weight * gridVel * dist.x,
                weight * gridVel * dist.y
            );

            B += term;
        }
    }

    p.C = B * 4.0;

    // Advect particle
    p.position += globals.dt * p.velocity * globals.dX;

    p.position = clamp(
        p.position,
        vec2<f32>(0.0),
        vec2<f32>(globals.worldSize.x, globals.worldSize.y)
    );

    particles[pid] = p;
}