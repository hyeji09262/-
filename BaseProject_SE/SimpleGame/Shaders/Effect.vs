#version 330
uniform vec4 u_Rect;
out vec2 v_UV;
void main()
{
    const vec2 corners[6] = vec2[6](vec2(0,0), vec2(1,0), vec2(1,1),
                                  vec2(0,0), vec2(1,1), vec2(0,1));
    v_UV = corners[gl_VertexID];
    gl_Position = vec4(u_Rect.xy + v_UV * u_Rect.zw, 0, 1);
}
