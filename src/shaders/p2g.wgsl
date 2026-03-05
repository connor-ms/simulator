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
    let base = vec2<i32>(gridPos);
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
            
            let cellDist = (vec2<f32>(cell) - gridPos) + vec2<f32>(0.5);
            let Q = p.C * cellDist;

            let massContrib = weight * 1;
            let momentum = massContrib * (p.velocity + Q);

            atomicAdd(&grid[gridIndex].mass, toFixed(massContrib));
            atomicAdd(&grid[gridIndex].vX, toFixed(momentum.x));
            atomicAdd(&grid[gridIndex].vY, toFixed(momentum.y));
        }
    }
}