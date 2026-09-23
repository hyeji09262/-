#version 330
layout(location = 0) in vec4 a_Transform;
layout(location = 1) in vec2 a_Range;
uniform samplerBuffer u_ModelData;
uniform vec2 u_Viewport;
out vec4 v_Color;

void main()
{
    // Preserve instance order. Vertices beyond each cached mesh are fully clipped.
    if (gl_VertexID >= int(a_Range.y))
    {
        gl_Position = vec4(2.0, 2.0, 0.0, 1.0);
        v_Color = vec4(0.0);
        return;
    }
    int address = (int(a_Range.x) + gl_VertexID) * 2;
    vec4 first = texelFetch(u_ModelData, address);
    vec4 second = texelFetch(u_ModelData, address + 1);
    vec2 pixel = a_Transform.xy + first.xy * a_Transform.zw;
    gl_Position =
        vec4(pixel.x * 2.0 / u_Viewport.x - 1.0, 1.0 - pixel.y * 2.0 / u_Viewport.y, 0.0, 1.0);
    v_Color = vec4(first.zw, second.xy);
}
