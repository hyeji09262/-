#version 330
in vec2 v_UV;
uniform sampler2D u_Source;
uniform vec2 u_Texel;
uniform int u_Pass;
out vec4 FragColor;

void main()
{
    vec3 color = vec3(0.0);
    if (u_Pass == 0)
    {
        for (int y = -1; y <= 1; y += 2)
        {
            for (int x = -1; x <= 1; x += 2)
            {
                vec3 sampleColor = texture(u_Source, v_UV + vec2(x, y) * u_Texel * 1.5).rgb;
                color += max(sampleColor - vec3(.58), vec3(0.0)) * .25;
            }
        }
    }
    else
    {
        vec2 axis = u_Pass == 1 ? vec2(u_Texel.x, 0.0) : vec2(0.0, u_Texel.y);
        color = texture(u_Source, v_UV).rgb * .40262;
        color += (texture(u_Source, v_UV + axis).rgb + texture(u_Source, v_UV - axis).rgb) * .24420;
        color +=
            (texture(u_Source, v_UV + axis * 2.0).rgb + texture(u_Source, v_UV - axis * 2.0).rgb) *
            .05449;
    }
    FragColor = vec4(color, 1.0);
}
