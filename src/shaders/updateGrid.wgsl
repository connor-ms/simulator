@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read_write> particles: array<Particle>;
@group(1) @binding(1) var<storage, read_write> grid: array<GridNode>;

@compute @workgroup_size(64)
fn updateGrid(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= arrayLength(&grid)) { return; }

    let m = atomicLoad(&grid[id.x].mass);
    if (m <= 0) { return; }

    var fv = vec2f(
        toFloat(atomicLoad(&grid[id.x].vX)),
        toFloat(atomicLoad(&grid[id.x].vY)),
    );
    fv /= toFloat(m);

    // gravity in grid-space: g_grid = g_world * idX
    fv.y += globals.gravity * globals.dt * globals.idX;

    // grid cell coords from flat index
    let gs = i32(globals.gridSize);
    let cx = i32(id.x) / gs;
    let cy = i32(id.x) % gs;

    // boundary: 2-cell border inside grid
    let max_c = gs - 3;
    if (cx < 2 || cx > max_c) { fv.x = 0.0; }
    if (cy < 2 || cy > max_c) { fv.y = 0.0; }

    atomicStore(&grid[id.x].vX, toFixed(fv.x));
    atomicStore(&grid[id.x].vY, toFixed(fv.y));
}