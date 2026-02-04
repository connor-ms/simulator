// @vertex 
// fn vertexMain(@builtin(vertex_index) i : u32) ->
//     @builtin(position) vec4f {
//     const pos = array(vec2f(0, 1), vec2f(-1, -1), vec2f(1, -1));
//     return vec4f(pos[i], 0, 1);
// }

// @fragment 
// fn fragmentMain() -> @location(0) vec4f {
//     return vec4f(1, 0, 0, 1);
// }
struct VSOut {
    @builtin(position) pos : vec4<f32>,
    @location(0) uv : vec2<f32>,
};

@vertex
fn vs_main(
    @location(0) pos : vec2<f32>,
    @location(1) uv  : vec2<f32>
) -> VSOut {
    var out : VSOut;
    out.pos = vec4<f32>(pos, 0.0, 1.0);
    out.uv = uv * 2.0 - 1.0; // [-1, 1]
    return out;
}

@fragment
fn fs_main(@location(0) uv : vec2<f32>) -> @location(0) vec4<f32> {
    // ---- Bounding box ----
    let boxEdge = 0.9;
    let boxThickness = 0.02;

    let boxDist = max(abs(uv.x), abs(uv.y));
    let boxAlpha =
        smoothstep(boxEdge, boxEdge - boxThickness, boxDist);

    // ---- Circle ----
    let radius = 0.6;
    let d = length(uv);
    let edge = fwidth(d);
    let circleAlpha =
        1.0 - smoothstep(radius - edge, radius + edge, d);

    // Colors
    let boxColor = vec3<f32>(0.2, 0.8, 1.0);
    let circleColor = vec3<f32>(1.0, 0.5, 0.2);

    let color =
        boxColor * boxAlpha +
        circleColor * circleAlpha;

    let alpha = max(boxAlpha, circleAlpha);

    return vec4<f32>(color, alpha);
}