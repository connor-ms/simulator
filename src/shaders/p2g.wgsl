@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read_write> particles: array<Particle>;
@group(1) @binding(1) var<storage, read_write> grid: array<GridNode>;

@compute @workgroup_size(64)
fn p2g(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= globals.particleCount) { return; }

    let p        = particles[id.x];
    let gpos     = p.position * globals.idX;         // world → grid space
    let cell_idx = floor(gpos);
    let cell_diff = gpos - (cell_idx + 0.5);

    var w: array<vec2f, 3>;
    w[0] = 0.5 * (0.5 - cell_diff) * (0.5 - cell_diff);
    w[1] = 0.75 - cell_diff * cell_diff;
    w[2] = 0.5 * (0.5 + cell_diff) * (0.5 + cell_diff);

    for (var gx = 0; gx < 3; gx++) {
        for (var gy = 0; gy < 3; gy++) {
            let weight = w[gx].x * w[gy].y;

            let cell = vec2f(
                cell_idx.x + f32(gx) - 1.0,
                cell_idx.y + f32(gy) - 1.0,
            );
            let cell_dist  = (cell + 0.5) - gpos;
            let Q          = p.C * cell_dist;

            // particle mass = 1
            let mass_contrib = weight;
            // velocity is world-space, scale to grid-space before scattering
            let vel_contrib  = mass_contrib * (p.velocity * globals.idX + Q);

            let ci = i32(cell.x) * i32(globals.gridSize) + i32(cell.y);

            atomicAdd(&grid[ci].mass, toFixed(mass_contrib));
            atomicAdd(&grid[ci].vX,   toFixed(vel_contrib.x));
            atomicAdd(&grid[ci].vY,   toFixed(vel_contrib.y));
        }
    }
}

@compute @workgroup_size(64)
fn p2g_2(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= globals.particleCount) { return; }

    let p         = particles[id.x];
    let gpos      = p.position * globals.idX;
    let cell_idx  = floor(gpos);
    let cell_diff = gpos - (cell_idx + 0.5);

    var w: array<vec2f, 3>;
    w[0] = 0.5 * (0.5 - cell_diff) * (0.5 - cell_diff);
    w[1] = 0.75 - cell_diff * cell_diff;
    w[2] = 0.5 * (0.5 + cell_diff) * (0.5 + cell_diff);

    // --- density estimate from grid mass ---
    var density = 0.0;
    for (var gx = 0; gx < 3; gx++) {
        for (var gy = 0; gy < 3; gy++) {
            let weight = w[gx].x * w[gy].y;
            let cell   = vec2f(
                cell_idx.x + f32(gx) - 1.0,
                cell_idx.y + f32(gy) - 1.0,
            );
            let ci   = i32(cell.x) * i32(globals.gridSize) + i32(cell.y);
            density += toFloat(atomicLoad(&grid[ci].mass)) * weight;
        }
    }

    // volume = 1 / density  (particle mass = 1)
    let volume   = 1.0 / density;
    let pressure = max(0.0, globals.eos_stiffness
                     * (pow(density / globals.rest_density, globals.eos_power) - 1.0));

    // 2-D stress (mat2x2)
    var stress = mat2x2f(-pressure, 0.0,
                          0.0, -pressure);
    let strain = p.C + transpose(p.C);
    stress    += globals.dynamic_viscosity * strain;

    let eq16 = -volume * 4.0 * stress * globals.dt;

    // --- scatter momentum ---
    for (var gx = 0; gx < 3; gx++) {
        for (var gy = 0; gy < 3; gy++) {
            let weight    = w[gx].x * w[gy].y;
            let cell      = vec2f(
                cell_idx.x + f32(gx) - 1.0,
                cell_idx.y + f32(gy) - 1.0,
            );
            let cell_dist = (cell + 0.5) - gpos;
            let ci        = i32(cell.x) * i32(globals.gridSize) + i32(cell.y);
            let momentum  = eq16 * weight * cell_dist;
            atomicAdd(&grid[ci].vX, toFixed(momentum.x));
            atomicAdd(&grid[ci].vY, toFixed(momentum.y));
        }
    }
}