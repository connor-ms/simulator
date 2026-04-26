@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read_write> particles: array<Particle>;
@group(1) @binding(1) var<storage, read_write> grid: array<GridNode>;

@compute @workgroup_size(64)
fn g2p(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= globals.particleCount) { return; }

    var p = particles[id.x];
    
    let cell_idx = floor(p.position);
    let cell_diff = (p.position - cell_idx) - 0.5f;

    var w: array<vec3f, 3>;
    w[0] = 0.5f * (0.5f - cell_diff) * (0.5f - cell_diff);
    w[1] = 0.75f - cell_diff * cell_diff;
    w[2] = 0.5f * (0.5f + cell_diff) * (0.5f + cell_diff);

    p.velocity = vec3f(0.0);
    var B = mat3x3f(vec3f(0.0), vec3f(0.0), vec3f(0.0));

    for (var gx = 0; gx < 3; gx++) {
        for (var gy = 0; gy < 3; gy++) {
            for (var gz = 0; gz < 3; gz++) {
                let weight = w[gx].x * w[gy].y * w[gz].z;

                let cell = floor(vec3f(
                    cell_idx.x + f32(gx) - 1.0,
                    cell_idx.y + f32(gy) - 1.0,
                    cell_idx.z + f32(gz) - 1.0,
                ));

                let cell_dist = (cell - p.position) + 0.5;

                let ci = i32(cell.x) * i32(globals.gridSize.y) * i32(globals.gridSize.z) + 
                         i32(cell.y) * i32(globals.gridSize.z) +
                         i32(cell.z);

                let gv = vec3f(
                    toFloat(atomicLoad(&grid[ci].vX)),
                    toFloat(atomicLoad(&grid[ci].vY)),
                    toFloat(atomicLoad(&grid[ci].vZ)),
                );

                let wv = gv * weight;
                p.velocity += wv;

                B += mat3x3f(wv * cell_dist.x, wv * cell_dist.y, wv * cell_dist.z);
            }
        }
    }

    p.C = B * 4.0;

    p.position += p.velocity * globals.dt;
    // p.debug1 = p.velocity.x;
    // p.debug2 = p.velocity.y;

    // clamp to world bounds
    let lo = vec3f(1.0);
    let hi = vec3f(f32(globals.gridSize.x - 2), f32(globals.gridSize.y - 2), f32(globals.gridSize.z - 2));
    p.position = clamp(p.position, lo, hi);

    // if (globals.isMouseDown == 1) {
    //     let dist = p.position - globals.mousePos;
    //     if (dot(dist, dist) < (10 * 10)) {
    //         let force = normalize(dist);
    //         p.velocity += force;
    //     }
    // }

    let k = 3.0;
    let wall_stiffness = 0.3;
    let x_n = p.position + p.velocity * globals.dt * k;
    let wall_min = vec3f(1.0);
    let wall_max = hi;

    if (x_n.x < wall_min.x) { p.velocity.x += wall_stiffness * (wall_min.x - x_n.x); }
    if (x_n.x > wall_max.x) { p.velocity.x += wall_stiffness * (wall_max.x - x_n.x); }
    if (x_n.y < wall_min.y) { p.velocity.y += wall_stiffness * (wall_min.y - x_n.y); }
    if (x_n.y > wall_max.y) { p.velocity.y += wall_stiffness * (wall_max.y - x_n.y); }
    if (x_n.z < wall_min.z) { p.velocity.z += wall_stiffness * (wall_min.z - x_n.z); }
    if (x_n.z > wall_max.z) { p.velocity.z += wall_stiffness * (wall_max.z - x_n.z); }

    particles[id.x] = p;
}