struct VertexOut
{
    @builtin(position) position : vec4<f32>,
    @location(0) uv : vec2<f32>,
    @location(1) debug1: f32,
    @location(2) debug2: f32,
    @location(3) sphereCenter_view: vec3<f32>,
    @location(4) fragPos_view: vec3<f32>,
    @location(5) radius: f32,
};

struct Uniforms
{
    proj: mat4x4f,
    view: mat4x4f,
}

@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<storage, read> particles: array<Particle>;
@group(2) @binding(0) var<uniform> uniforms: Uniforms;

@vertex
fn vs_main(@location(0) pos : vec2<f32>,
           @location(1) uv : vec2<f32>,
           @builtin(instance_index) instanceIndex: u32) -> VertexOut
{
    var out: VertexOut;

    let radius = 0.5;
    let particle = particles[instanceIndex];

    let center_view = (uniforms.view * vec4<f32>(particle.position, 1.0)).xyz;

    let quadOffset = pos * radius;
    let vertPos_view = center_view + vec3<f32>(quadOffset, 0.0);

    out.position = uniforms.proj * vec4<f32>(vertPos_view, 1.0);
    out.uv = uv;
    out.debug1 = particle.debug1;
    out.debug2 = particle.debug2;
    out.sphereCenter_view = center_view;
    out.fragPos_view = vertPos_view;
    out.radius = radius;

    return out;
}

struct FragOut {
    @location(0) color: vec4<f32>,
    @builtin(frag_depth) depth: f32,
};

@fragment
fn fs_main(@builtin(position) fragCoord: vec4<f32>,
          @location(0) uv: vec2<f32>,
          @location(1) debug1: f32,
          @location(2) debug2: f32,
          @location(3) sphereCenter_view: vec3<f32>,
          @location(4) fragPos_view: vec3<f32>,
          @location(5) radius: f32) -> FragOut
{
    var out: FragOut;

    let rayOrigin = vec3<f32>(0.0, 0.0, 0.0);
    let rayDir = normalize(fragPos_view);

    let oc = rayOrigin - sphereCenter_view;
    let b = dot(rayDir, oc);
    let c = dot(oc, oc) - radius * radius;
    let disc = b * b - c;

    if (disc < 0.0) { discard; }

    let t = -b - sqrt(disc);
    if (t < 0.0) { discard; }

    let hitPos_view = rayOrigin + rayDir * t;

    let normal_view = normalize(hitPos_view - sphereCenter_view);

    let hitPos_clip = uniforms.proj * vec4<f32>(hitPos_view, 1.0);
    out.depth = hitPos_clip.z / hitPos_clip.w;

    let lightDir = normalize(vec3<f32>(1.0, 2.0, 1.0));
    let diffuse   = max(dot(normal_view, lightDir), 0.0);
    let ambient   = 0.15;

    let viewDir   = normalize(-hitPos_view);              // toward camera
    let halfVec   = normalize(lightDir + viewDir);
    let specular  = pow(max(dot(normal_view, halfVec), 0.0), 32.0) * 0.5;

    let pressure    = debug1;
    let maxPressure = 50.0;
    let tCol        = clamp(pressure / maxPressure, 0.0, 1.0);
    let baseColor   = vec3<f32>(tCol, tCol, 1.0);

    let litColor = baseColor * (ambient + diffuse) + vec3<f32>(specular);
    out.color = vec4<f32>(litColor, 1.0);

    return out;
}