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
    let radius = vec2<f32>(2, 2);

    let particle = particles[instanceIndex];
    let worldPos = (particle.position + pos * radius) - (vec2f(f32(globals.gridSize), f32(globals.gridSize)) * 0.5);

    out.position = globals.proj * vec4<f32>(worldPos, 0.0, 1.0);
    out.uv = uv;
    out.debug1 = particle.debug1;
    out.debug2 = particle.debug2;
    
    return out;
}

@fragment
fn fs_main(@builtin(position) position: vec4<f32>,
           @location(0) uv : vec2<f32>,
           @location(1) debug1: f32,
           @location(2) debug2: f32) -> @location(0) vec4<f32>
{
    let dist = length(uv - vec2<f32>(0.5, 0.5));

    if (dist > 0.5)
    {
        discard;
    }

    // let velocity = vec2f(debug1, debug2);

    // let speed = length(velocity);
    // let maxSpeed = 50.0;
    // let t = clamp(speed / maxSpeed, 0.0, 1.0);

    // let velDir = normalize(velocity) * 0.5 + 0.5;

    // return vec4<f32>(velDir.x, velDir.y, t, 1.0);

    // if (globals.isMouseDown == 1) {
    //     let dist = vec2f(position.x, position.y) - globals.mousePos;
    //     if (dot(dist, dist) < (10 * 10)) {
    //         return vec4<f32>(1, 1, 1, 1.0);
    //     }
    // }

    let pressure = debug1;
    let density = debug2;

    let maxPressure = 100.0;
    let t = clamp(pressure / maxPressure, 0.0, 1.0);

    return vec4<f32>(t, t,1, 1.0);
}