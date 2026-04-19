struct VertexOut
{
    @builtin(position) position : vec4<f32>,
    @location(0) uv : vec2<f32>,
};

struct Line
{
    p1: vec3<f32>,
    p2: vec3<f32>,
    //p3: vec3<f32>,
    thickness: f32,
};

struct Uniforms
{
    proj: mat4x4f,
    view: mat4x4f,
}

@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read> lines: array<Line>;
@group(2) @binding(0) var<uniform> uniforms: Uniforms;

@vertex
fn vs_main(@location(0) pos : vec2<f32>,
           @location(1) uv : vec2<f32>,
           @builtin(instance_index) instanceIndex: u32) -> VertexOut
{
    var out: VertexOut;

    let line = lines[instanceIndex];
    let p1 = line.p1;
    let p2 = line.p2;

    let delta = p2 - p1;
    let length = length(delta);
    let dir = normalize(delta);
 
    let perp = normalize(vec3<f32>(-dir.y, dir.x, 0.0));

    let t = pos.x + 0.5;
    let along = dir * (t * length);
    let offset = perp * (pos.y * line.thickness);

    let worldPos = p1 + along + offset;

    out.position = uniforms.proj * uniforms.view * vec4<f32>(worldPos, 1.0);
    out.uv = uv;

    return out;
}

@fragment
fn fs_main(@location(0) uv : vec2<f32>) -> @location(0) vec4<f32>
{
    return vec4<f32>(1.0, 1.0, 1.0, 1.0);
}