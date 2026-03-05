@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read_write> particles: array<Particle>;
@group(1) @binding(1) var<storage, read_write> grid: array<GridNode>;

@compute @workgroup_size(8, 8)
fn updateGrid(@builtin(global_invocation_id) id : vec3<u32>) {
    let x = id.x;
    let y = id.y;

    if (x >= globals.gridSize || y >= globals.gridSize) {
        return;
    }

    let index = y * globals.gridSize + x;

    let mass = toFloat(atomicLoad(&grid[index].mass));
    var vX = toFloat(atomicLoad(&grid[index].vX));
    var vY = toFloat(atomicLoad(&grid[index].vY));

    if (mass > 0.0) {
        vX /= mass;
        vY /= mass;

        // Note to self: may have issues if a non-square world is used?
        vY -= 10.0;// * globals.idX;

        let boundary = 2u;

        if (x < boundary || x > globals.gridSize - boundary) {
            vX = 0.0;
        }
        if (y < boundary || y > globals.gridSize - boundary) {
            vY = 0.0;
        }
    }

    atomicStore(&grid[index].vX, toFixed(vX));
    atomicStore(&grid[index].vY, toFixed(vY));
}