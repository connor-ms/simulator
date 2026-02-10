struct VertexOut
{
    @builtin(position) position : vec4<f32>
};

@vertex
fn vs_main(@location(0) topLeft : vec2<f32>) -> VertexOut
{
    var out : VertexOut;
    out.position = vec4<f32>(topLeft, 0.0, 1.0);
    
    return out;
}

@fragment
fn fs_main() -> @location(0) vec4<f32>
{
    return vec4<f32>(1.0, 1.0, 1.0, 1.0);
}