struct VertexOut
{
    @builtin(position) position : vec4<f32>
};

struct Globals
{
    windowSize: vec2<f32>,
    worldSize: vec3<f32>,
    proj: mat4x4<f32>,
};

@group(0) @binding(0) var<uniform> globals: Globals;

@vertex
fn vs_main(@location(0) topLeft : vec2<f32>) -> VertexOut
{
    var out : VertexOut;
    
    out.position = globals.proj * vec4<f32>(topLeft, 0.0, 1.0);

    return out;
}

@fragment
fn fs_main() -> @location(0) vec4<f32>
{
    return vec4<f32>(1.0, 1.0, 1.0, 1.0);
}