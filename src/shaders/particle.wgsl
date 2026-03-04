struct VertexOut
{
    @builtin(position) position : vec4<f32>,
    @location(0) uv : vec2<f32>,
};

@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read> particles: array<Particle>;

@vertex
fn vs_main(@location(0) pos : vec2<f32>,
           @location(1) uv : vec2<f32>,
           @builtin(instance_index) instanceIndex: u32) -> VertexOut
{
    var out: VertexOut;
    let radius = vec2<f32>(5, 5);

    let particle = particles[instanceIndex];
    let worldPos = (particle.position + pos * radius) - (vec2f(globals.worldSize.x, globals.worldSize.y) * 0.5);

    out.position = globals.proj * vec4<f32>(worldPos, 0.0, 1.0);
    out.uv = uv;
    
    return out;
}

@fragment
fn fs_main(@location(0) uv : vec2<f32>) -> @location(0) vec4<f32>
{
    let dist = length(uv - vec2<f32>(0.5, 0.5));

    if (dist > 0.5)
    {
        discard;
    }
    
    return vec4<f32>(1.0, 0.0, 0.0, 1.0);
}