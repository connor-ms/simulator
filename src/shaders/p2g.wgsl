@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read_write> particles: array<Particle>;
@group(1) @binding(1) var<storage, read_write> grid: array<GridNode>;

@compute @workgroup_size(64)
fn p2g(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= globals.particleCount) { return; }

    let p = particles[id.x];
    let gridPos = p.position * globals.idX;
    let cell_idx = floor(gridPos);
    let cell_diff = gridPos - (cell_idx + 0.5);

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
            let cell_dist  = (cell + 0.5) - gridPos;
            let Q          = p.C * cell_dist;

            // particle mass = 1
            let mass_contrib = weight * 1;
            let vel_contrib = mass_contrib * (p.velocity * globals.idX + Q);

            let ci = i32(cell.y) * i32(globals.gridSize) + i32(cell.x);

            atomicAdd(&grid[ci].mass, toFixed(mass_contrib));
            atomicAdd(&grid[ci].vX,   toFixed(vel_contrib.x));
            atomicAdd(&grid[ci].vY,   toFixed(vel_contrib.y));
        }
    }
}

@compute @workgroup_size(64)
fn p2g_2(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= globals.particleCount) { return; }

    let p = particles[id.x];
    let gpos = p.position * globals.idX;
    let cell_idx = floor(gpos);
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
            if (cell.x < 0.0 || cell.y < 0.0 ||
                cell.x >= f32(globals.gridSize) ||
                cell.y >= f32(globals.gridSize)) {
                continue;
            }
            let ci   = i32(cell.y) * i32(globals.gridSize) + i32(cell.x);
            density += toFloat(atomicLoad(&grid[ci].mass));
        }
    }

    let density_safe = max(density, 1e-6);
let volume = 1.0 / density_safe;
    //let volume = 1.0 / density;
    let pressure = max(-0.1f, globals.eos_stiffness * (pow(density / globals.rest_density, globals.eos_power) - 1.0));

    var stress = mat2x2f(
        -pressure, 0.0,
        0.0, -pressure
    );

    var dudv = p.C;
    var strain = dudv;
    var trace = strain[1][0] + strain[0][1];
    strain[0][1] = trace;
    strain[1][0] = trace;

    let viscosity_term = globals.dynamic_viscosity * strain;
    stress += viscosity_term;

    let eq16_term_0 = -volume * 4.0 * stress * globals.dt;

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

            let momentum = eq16_term_0 * weight * cell_dist;
            atomicAdd(&grid[ci].vX, toFixed(momentum.x));
            atomicAdd(&grid[ci].vY, toFixed(momentum.y));
        }
    }
}