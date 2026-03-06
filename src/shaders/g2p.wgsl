@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read_write> particles: array<Particle>;
@group(1) @binding(1) var<storage, read_write> grid: array<GridNode>;

@compute @workgroup_size(64)
fn g2p(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= globals.particleCount) { return; }

    var p = particles[id.x];
    let gpos = p.position * globals.idX;
    let cell_idx = floor(gpos);
    let cell_diff = gpos - (cell_idx + 0.5);

    var w: array<vec2f, 3>;
    w[0] = 0.5 * (0.5 - cell_diff) * (0.5 - cell_diff);
    w[1] = 0.75 - cell_diff * cell_diff;
    w[2] = 0.5 * (0.5 + cell_diff) * (0.5 + cell_diff);

    p.velocity = vec2f(0.0);
    var B = mat2x2f(vec2f(0.0), vec2f(0.0));

    for (var gx = 0; gx < 3; gx++) {
        for (var gy = 0; gy < 3; gy++) {
            let weight = w[gx].x * w[gy].y;

            let cell = vec2f(
                cell_idx.x + f32(gx) - 1.0,
                cell_idx.y + f32(gy) - 1.0,
            );

            if (cell.x < 0.0 || cell.y < 0.0 ||
                cell.x >= f32(globals.gridSize) ||
                cell.y >= f32(globals.gridSize)) {
                continue;
            }

            let cell_dist = (cell + 0.5) - gpos;
            let ci = i32(cell.y) * i32(globals.gridSize) + i32(cell.x);

            // convert back to world space velocity
            let gv = vec2f(
                toFloat(atomicLoad(&grid[ci].vX)),
                toFloat(atomicLoad(&grid[ci].vY)),
            ) * globals.dX;

            let wv = gv * weight;
            p.velocity += wv;

            // APIC affine matrix accumulation
            B += mat2x2f(wv * cell_dist.x, wv * cell_dist.y);
        }
    }

    p.C = B * 4.0;
    //p.J *= (1.0 + globals.dt * (p.C[0][0] + p.C[1][1])); // trace(C) = div(v)

    // advect in world space
    p.position += p.velocity * globals.dt;
    p.debug1 = p.velocity.x;
    p.debug2 = p.velocity.y;

    // clamp to world bounds
    let lo = vec2f(1.0)  * globals.dX;
    let hi = globals.worldSize.xy - 2.0 * globals.dX;
    p.position = clamp(p.position, lo, hi);

    // soft wall repulsion (look-ahead)
    let k            = 3.0;
    let wall_stiffness = 0.3;
    let x_n          = p.position + p.velocity * globals.dt * k;
    let wall_min     = vec2f(3.0) * globals.dX;
    let wall_max     = globals.worldSize.xy - 4.0 * globals.dX;

    if (x_n.x < wall_min.x) { p.velocity.x += wall_stiffness * (wall_min.x - x_n.x); }
    if (x_n.x > wall_max.x) { p.velocity.x += wall_stiffness * (wall_max.x - x_n.x); }
    if (x_n.y < wall_min.y) { p.velocity.y += wall_stiffness * (wall_min.y - x_n.y); }
    if (x_n.y > wall_max.y) { p.velocity.y += wall_stiffness * (wall_max.y - x_n.y); }

    particles[id.x] = p;
}