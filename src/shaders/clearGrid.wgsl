@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read_write> particles: array<Particle>;
@group(1) @binding(1) var<storage, read_write> grid: array<GridNode>;

@compute @workgroup_size(8, 8)
fn clearGrid(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= globals.gridSize || id.y >= globals.gridSize) {
        return;
    }

    let index = id.x + id.y * globals.gridSize;

    atomicStore(&grid[index].mass, 0u);
    atomicStore(&grid[index].vX, 0u);
    atomicStore(&grid[index].vY, 0u);
}