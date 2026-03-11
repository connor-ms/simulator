@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read_write> particles: array<Particle>;
@group(1) @binding(1) var<storage, read_write> grid: array<GridNode>;

@compute @workgroup_size(64)
fn p2g(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= globals.particleCount) { return; }

    let p = particles[id.x];

    let cell_idx = floor(p.position);
    let cell_diff = (p.position - cell_idx) - 0.5f;

    var w: array<vec2f, 3>;
    w[0] = 0.5f * (0.5f - cell_diff) * (0.5f - cell_diff);
    w[1] = 0.75f - cell_diff * cell_diff;
    w[2] = 0.5f * (0.5f + cell_diff) * (0.5f + cell_diff);

    for (var gx = 0; gx < 3; gx++) {
        for (var gy = 0; gy < 3; gy++) {
            let weight = w[gx].x * w[gy].y;

            let cell = floor(vec2f(
                cell_idx.x + f32(gx) - 1.0,
                cell_idx.y + f32(gy) - 1.0,
            ));
            let cell_dist  = (cell - p.position) + 0.5;
            let Q          = p.C * cell_dist;

            // particle mass = 1
            let mass_contrib = weight * 1.0;
            let vel_contrib = mass_contrib * (p.velocity + Q);

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

    var p = particles[id.x];
    
    let cell_idx = floor(p.position);
    let cell_diff = (p.position - cell_idx) - 0.5;

    var w: array<vec2f, 3>;
    w[0] = 0.5 * (0.5 - cell_diff) * (0.5 - cell_diff);
    w[1] = 0.75 - cell_diff * cell_diff;
    w[2] = 0.5 * (0.5 + cell_diff) * (0.5 + cell_diff);

    var density = 0.0;
    for (var gx = 0; gx < 3; gx++) {
        for (var gy = 0; gy < 3; gy++) {
            let weight = w[gx].x * w[gy].y;
            let cell   = floor(vec2f(
                cell_idx.x + f32(gx) - 1.0,
                cell_idx.y + f32(gy) - 1.0,
            ));
            
            let ci   = i32(cell.x) * i32(globals.gridSize) + i32(cell.y);
            density += toFloat(atomicLoad(&grid[ci].mass)) * weight;
        }
    }

    let volume = 1.0 / density;
    let pressure = max(-0.1, globals.eos_stiffness * (pow(density / globals.rest_density, globals.eos_power) - 1.0));

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

            let cell_dist = (cell - p.position) + 0.5;
            let ci = i32(cell.x) * i32(globals.gridSize) + i32(cell.y);

            let momentum = eq16_term_0 * weight * cell_dist;
            atomicAdd(&grid[ci].vX, toFixed(momentum.x));
            atomicAdd(&grid[ci].vY, toFixed(momentum.y));
        }
    }

    // p.debug1 = toFloat(atomicLoad(&grid[floor(p.position)].vX));
    // p.debug2 = toFloat(atomicLoad(&grid[floor(p.position)].vY));
    p.debug1 = pressure;
    p.debug2 = density;

    particles[id.x] = p;
}