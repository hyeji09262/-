#version 330
layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_UV;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in vec3 a_Effect;
out vec2 v_UV;
out vec4 v_Color;
flat out vec3 v_Effect;

void main()
{
    gl_Position = vec4(a_Position, 0.0, 1.0);
    v_UV = a_UV;
    v_Color = a_Color;
    v_Effect = a_Effect;
}
