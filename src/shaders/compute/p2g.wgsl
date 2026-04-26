@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read_write> particles: array<Particle>;
@group(1) @binding(1) var<storage, read_write> grid: array<GridNode>;

@compute @workgroup_size(64)
fn p2g(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= globals.particleCount) { return; }

    let p = particles[id.x];

    let cell_idx = floor(p.position);
    let cell_diff = (p.position - cell_idx) - 0.5f;

    var w: array<vec3f, 3>;
    w[0] = 0.5f * (0.5f - cell_diff) * (0.5f - cell_diff);
    w[1] = 0.75f - cell_diff * cell_diff;
    w[2] = 0.5f * (0.5f + cell_diff) * (0.5f + cell_diff);

    let C = p.C;

    for (var gx = 0; gx < 3; gx++) {
        for (var gy = 0; gy < 3; gy++) {
            for (var gz = 0; gz < 3; gz++) {
                let weight = w[gx].x * w[gy].y * w[gz].z;

                let cell = vec3f(
                    cell_idx.x + f32(gx) - 1.0,
                    cell_idx.y + f32(gy) - 1.0,
                    cell_idx.z + f32(gz) - 1.0,
                );
                let cell_dist = (cell - p.position) + 0.5;

                let Q = C * cell_dist;

                // particle mass = 1
                let mass_contrib = weight * 1.0;
                let vel_contrib = mass_contrib * (p.velocity + Q);

                let ci = i32(cell.x) * i32(globals.gridSize.y) * i32(globals.gridSize.z) + 
                         i32(cell.y) * i32(globals.gridSize.z) +
                         i32(cell.z);

                atomicAdd(&grid[ci].mass, toFixed(mass_contrib));
                atomicAdd(&grid[ci].vX,   toFixed(vel_contrib.x));
                atomicAdd(&grid[ci].vY,   toFixed(vel_contrib.y));
                atomicAdd(&grid[ci].vZ,   toFixed(vel_contrib.z));
            }
        }
    }
}

@compute @workgroup_size(64)
fn p2g_2(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= globals.particleCount) { return; }

    var p = particles[id.x];
    
    let cell_idx = floor(p.position);
    let cell_diff = (p.position - cell_idx) - 0.5;

    var w: array<vec3f, 3>;
    w[0] = 0.5 * (0.5 - cell_diff) * (0.5 - cell_diff);
    w[1] = 0.75 - cell_diff * cell_diff;
    w[2] = 0.5 * (0.5 + cell_diff) * (0.5 + cell_diff);

    var density = 0.0;
    for (var gx = 0; gx < 3; gx++) {
        for (var gy = 0; gy < 3; gy++) {
            for (var gz = 0; gz < 3; gz++) {
                let weight = w[gx].x * w[gy].y * w[gz].z;

                let cell = vec3f(
                    cell_idx.x + f32(gx) - 1.0,
                    cell_idx.y + f32(gy) - 1.0,
                    cell_idx.z + f32(gz) - 1.0,
                );

                let ci = i32(cell.x) * i32(globals.gridSize.y) * i32(globals.gridSize.z) + 
                         i32(cell.y) * i32(globals.gridSize.z) +
                         i32(cell.z);

                density += toFloat(atomicLoad(&grid[ci].mass)) * weight;
            }
        }
    }

    if (density < 1e-6) { return; }

    let volume = 1.0 / density;
    let pressure = max(-0.0, globals.eos_stiffness * (pow(density / globals.rest_density, globals.eos_power) - 1.0));

    var stress = mat3x3f(
        -pressure, 0.0, 0.0,
        0.0, -pressure, 0.0,
        0.0, 0.0, -pressure,
    );

    var dudv = p.C;
    var strain = dudv + transpose(dudv);

    let viscosity_term = globals.dynamic_viscosity * strain;
    stress += viscosity_term;

    let eq16_term_0 = -volume * 4.0 * stress * globals.dt;

    for (var gx = 0; gx < 3; gx++) {
        for (var gy = 0; gy < 3; gy++) {
            for (var gz = 0; gz < 3; gz++) {
                let weight = w[gx].x * w[gy].y * w[gz].z;

                let cell = vec3f(
                    cell_idx.x + f32(gx) - 1.0,
                    cell_idx.y + f32(gy) - 1.0,
                    cell_idx.z + f32(gz) - 1.0,
                );

                let cell_dist = (cell - p.position) + 0.5;

                let ci = i32(cell.x) * i32(globals.gridSize.y) * i32(globals.gridSize.z) + 
                         i32(cell.y) * i32(globals.gridSize.z) +
                         i32(cell.z);

                let momentum = eq16_term_0 * weight * cell_dist;
                atomicAdd(&grid[ci].vX, toFixed(momentum.x));
                atomicAdd(&grid[ci].vY, toFixed(momentum.y));
                atomicAdd(&grid[ci].vZ, toFixed(momentum.z));
            }
        }
    }

    // p.debug1 = toFloat(atomicLoad(&grid[floor(p.position)].vX));
    // p.debug2 = toFloat(atomicLoad(&grid[floor(p.position)].vY));
    p.debug1 = pressure;
    p.debug2 = density;

    particles[id.x] = p;
}