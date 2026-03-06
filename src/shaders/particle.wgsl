struct VertexOut
{
    @builtin(position) position : vec4<f32>,
    @location(0) uv : vec2<f32>,
    @location(1) debug1: f32,
    @location(2) debug2: f32,
};

@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read> particles: array<Particle>;

@vertex
fn vs_main(@location(0) pos : vec2<f32>,
           @location(1) uv : vec2<f32>,
           @builtin(instance_index) instanceIndex: u32) -> VertexOut
{
    var out: VertexOut;
    let radius = vec2<f32>(0.5, 0.5);

    let particle = particles[instanceIndex];
    let worldPos = (particle.position + pos * radius) - (vec2f(globals.worldSize.x, globals.worldSize.y) * 0.5);

    out.position = globals.proj * vec4<f32>(worldPos, 0.0, 1.0);
    out.uv = uv;
    out.debug1 = particle.debug1;
    out.debug2 = particle.debug2;
    
    return out;
}

@fragment
fn fs_main(@location(0) uv : vec2<f32>,
           @location(1) debug1: f32,
           @location(2) debug2: f32) -> @location(0) vec4<f32>
{
    let dist = length(uv - vec2<f32>(0.5, 0.5));

    if (dist > 0.5)
    {
        discard;
    }

    let velocity = vec2f(debug1, debug2);

    let speed = length(velocity);
    let maxSpeed = 100.0; // tune to your simulation
    let t = clamp(speed / maxSpeed, 0.0, 1.0);

    // direction-based color from velocity
    let velDir = normalize(velocity) * 0.5 + 0.5; // remap -1..1 to 0..1
    //out.color = vec4<f32>();

    //return vec4<f32>(velDir.x, velDir.y, t, 1.0);
    return vec4<f32>(1, 0, 0, 1.0);
}