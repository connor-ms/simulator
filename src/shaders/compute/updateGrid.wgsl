@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read_write> particles: array<Particle>;
@group(1) @binding(1) var<storage, read_write> grid: array<GridNode>;

@compute @workgroup_size(8, 8, 8)
fn updateGrid(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= globals.gridSize.x || id.y >= globals.gridSize.y || id.z >= globals.gridSize.z) { 
        return; 
    }

    let ci = i32(id.x) * i32(globals.gridSize.y) * i32(globals.gridSize.z) + 
             i32(id.y) * i32(globals.gridSize.z) +
             i32(id.z);

    let m = toFloat(atomicLoad(&grid[ci].mass));
    if (m <= 0.0) { return; }

    var fv = vec3f(
        toFloat(atomicLoad(&grid[ci].vX)),
        toFloat(atomicLoad(&grid[ci].vY)),
        toFloat(atomicLoad(&grid[ci].vZ)),
    );
    
    fv /= m;

    fv.y += (globals.gravity * globals.dt);

    //let max_c = i32(globals.gridSize) - 3;
    if (i32(id.x) < 2 || (id.x) > globals.gridSize.x - 3) { fv.x = 0.0; }
    if (i32(id.y) < 2 || (id.y) > globals.gridSize.y - 3) { fv.y = 0.0; }
    if (i32(id.z) < 2 || (id.z) > globals.gridSize.z - 3) { fv.z = 0.0; }

    atomicStore(&grid[ci].vX, toFixed(fv.x));
    atomicStore(&grid[ci].vY, toFixed(fv.y));
    atomicStore(&grid[ci].vZ, toFixed(fv.z));
}