#version 330
in vec2 v_UV;
uniform float u_Time;
uniform float u_Phase;
uniform int u_Kind;
out vec4 FragColor;

void main()
{
    vec2 p = v_UV * 2.0 - 1.0;
    if (u_Kind == 0)
    {
        if (abs(p.x) + abs(p.y) > 1.0) discard;
        float wave = sin(v_UV.x * 12.0 + v_UV.y * 7.0 + u_Phase - u_Time * 1.8);
        float ripple = sin(v_UV.y * 24.0 + sin(v_UV.x * 9.0 + u_Time) + u_Phase);
        vec3 water = mix(vec3(.09,.23,.30), vec3(.19,.39,.44), wave * .5 + .5);
        water += smoothstep(.88,1.0,ripple) * vec3(.13,.20,.18);
        FragColor = vec4(water, 1);
    }
    else if (u_Kind == 1)
    {
        float up = 1.0 - v_UV.y;
        float sway = sin(up * 10.0 - u_Time * 7.0 + u_Phase) * .10 * up;
        float width = .38 * pow(1.0-up, .65) + .015;
        float flame = 1.0 - smoothstep(width * .35, width, abs(p.x * .5 - sway));
        flame *= smoothstep(0.0,.16,v_UV.y) * smoothstep(0.0,.12,up);
        float core = 1.0 - smoothstep(.015,width*.65,abs(p.x*.5-sway));
        vec3 color = mix(vec3(1,.22,.025),vec3(1,.91,.40),core*(1.0-up*.7));
        FragColor = vec4(color, flame * .92);
    }
    else
    {
        float radius = length(p);
        float core = exp(-radius*radius*11.0);
        float ring = exp(-pow((radius-.52-.045*sin(u_Time*14.0))*14.0,2.0));
        float alpha = (core + ring*.40) * (1.0-smoothstep(.8,1.0,radius));
        FragColor = vec4(mix(vec3(.15,.65,1),vec3(.8,1,1),core),alpha);
    }
}
