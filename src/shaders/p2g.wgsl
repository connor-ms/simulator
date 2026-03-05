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
    let gridPos = p.position * globals.idX;
    let base = vec2<i32>(floor(gridPos - vec2<f32>(0.5)));
    let fx = gridPos - vec2<f32>(base) - vec2<f32>(0.5);

    // Quadratic B-spline weights
    var w: array<vec2<f32>, 3>;
    w[0] = 0.5 * pow(vec2<f32>(0.5) - fx, vec2<f32>(2.0));
    w[1] = vec2<f32>(0.75) - fx * fx;
    w[2] = 0.5 * pow(vec2<f32>(0.5) + fx, vec2<f32>(2.0));

    // Loop over 3x3 neighboring grid nodes
    for (var i = 0; i < 3; i++) {
        for (var j = 0; j < 3; j++) {

            let weight = w[i].x * w[j].y;
            let cell = base + vec2<i32>(i - 1, j - 1);

            if (cell.x < 0 || cell.y < 0 ||
                cell.x >= i32(globals.gridSize) ||
                cell.y >= i32(globals.gridSize)) {
                continue;
            }

            let gridIndex = u32(cell.y) * globals.gridSize + u32(cell.x);
            
            let cellDist = (vec2<f32>(cell) + vec2<f32>(0.5)) * globals.dX - p.position;
            let Q = p.C * cellDist;

            let massContrib = weight * 1;
            let momentum = massContrib * (p.velocity + Q);

            atomicAdd(&grid[gridIndex].mass, toFixed(massContrib));
            atomicAdd(&grid[gridIndex].vX, toFixed(momentum.x));
            atomicAdd(&grid[gridIndex].vY, toFixed(momentum.y));
        }
    }
}

@compute @workgroup_size(64)
fn p2g_2(@builtin(global_invocation_id) id: vec3<u32>) {
    let pid = id.x;

    if (pid >= globals.particleCount) {
        return;
    }

    let p = particles[pid];

    // Convert particle position to grid space
    let gridPos = p.position * globals.idX;
    let base = vec2<i32>(floor(gridPos - vec2<f32>(0.5)));
    let fx = gridPos - vec2<f32>(base) - vec2<f32>(0.5);

    var density = 0.0;

    var w: array<vec2<f32>, 3>;
    w[0] = 0.5 * pow(vec2<f32>(0.5) - fx, vec2<f32>(2.0));
    w[1] = vec2<f32>(0.75) - fx * fx;
    w[2] = 0.5 * pow(vec2<f32>(0.5) + fx, vec2<f32>(2.0));

    for (var i = 0; i < 3; i++) {
        for (var j = 0; j < 3; j++) {

            let weight = w[i].x * w[j].y;
            let cell = base + vec2<i32>(i - 1, j - 1);

            if (cell.x < 0 || cell.y < 0 ||
                cell.x >= i32(globals.gridSize) ||
                cell.y >= i32(globals.gridSize)) {
                continue;
            }

            let gridIndex = u32(cell.y) * globals.gridSize + u32(cell.x);
            let mass = toFloat(atomicLoad(&grid[gridIndex].mass));

            density += mass * weight;
        }
    }

    //density /= (globals.dX * globals.dX);

    //let volume = 1 / density;
    let volume = clamp(1.0 / density, 0.0, 10.0);

    let pressure = clamp(
    globals.eos_stiffness * (pow(density / globals.rest_density, globals.eos_power) - 1.0),
    -0.1, 100.0
);

    var stress = mat2x2<f32>(
        -pressure, 0.0,
        0.0, -pressure
    );

    stress += globals.dynamic_viscosity * p.C;
    let eq_term = -volume * 4.0 * globals.idX * globals.idX * stress * globals.dt;

    for (var gx:i32 = 0; gx < 3; gx++) {
        for (var gy:i32 = 0; gy < 3; gy++) {

            let weight = w[gx].x * w[gy].y;
            let node = base + vec2<i32>(gx-1, gy-1);

            if (node.x < 0 || node.y < 0 ||
                node.x >= i32(globals.gridSize) ||
                node.y >= i32(globals.gridSize)) {
                continue;
            }

            let idx = u32(node.y) * globals.gridSize + u32(node.x);

            let cell_world =
                (vec2<f32>(node) + vec2<f32>(0.5)) * globals.dX;

            let cell_dist = cell_world - p.position;

            let momentum = (eq_term * weight) * cell_dist;

            atomicAdd(&grid[idx].vX, toFixed(momentum.x));
            atomicAdd(&grid[idx].vY, toFixed(momentum.y));
        }
    }
}