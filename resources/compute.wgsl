// @group(0) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
// @group(0) @binding(1) var<storage,read_write> outputBuffer: array<f32,64>;

// @compute @workgroup_size(32, 1, 1) 
// fn computeSomething(@builtin(global_invocation_id) id: vec3<u32>) {
//     outputBuffer[id.x] = inputBuffer[id.x] + 2;
// }

@group(0) @binding(0) var<storage, read_write> positions: array<vec2<f32>>;

@compute @workgroup_size(64)
fn computeSomething(@builtin(global_invocation_id) id: vec3<u32>) {
    let idx = id.x;
    // Compute new positions
    positions[idx] = vec2<f32>(sin(f32(idx)), cos(f32(idx)));
}