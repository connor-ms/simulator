@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read_write> particles: array<Particle>;
@group(1) @binding(1) var<storage, read_write> grid: array<GridNode>;

@compute @workgroup_size(8, 8, 8)
fn updateGrid(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= globals.gridSize || id.y >= globals.gridSize || id.z >= globals.gridSize) { 
        return; 
    }

    let ci = id.x * globals.gridSize * globals.gridSize +
             id.y * globals.gridSize +
             id.z;

    let m = toFloat(atomicLoad(&grid[ci].mass));
    if (m <= 0.0) { return; }

    var fv = vec3f(
        toFloat(atomicLoad(&grid[ci].vX)),
        toFloat(atomicLoad(&grid[ci].vY)),
        toFloat(atomicLoad(&grid[ci].vZ)),
    );
    
    fv /= m;

    fv.y += (globals.gravity * globals.dt);

    let max_c = i32(globals.gridSize) - 3;
    if (i32(id.x) < 2 || i32(id.x) > max_c) { fv.x = 0.0; }
    if (i32(id.y) < 2 || i32(id.y) > max_c) { fv.y = 0.0; }
    if (i32(id.z) < 2 || i32(id.z) > max_c) { fv.z = 0.0; }

    atomicStore(&grid[ci].vX, toFixed(fv.x));
    atomicStore(&grid[ci].vY, toFixed(fv.y));
    atomicStore(&grid[ci].vZ, toFixed(fv.z));
}