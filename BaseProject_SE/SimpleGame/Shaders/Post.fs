#version 330
in vec2 v_UV;
uniform sampler2D u_Scene;
uniform sampler2D u_Bloom;
uniform float u_Time;
out vec4 FragColor;

void main()
{
    vec3 scene = texture(u_Scene, v_UV).rgb;
    vec3 glow = texture(u_Bloom, v_UV).rgb;
    vec3 color = scene + glow * 0.35;
    color = mix(vec3(dot(color, vec3(0.2126, 0.7152, 0.0722))), color, 0.90);
    // Soft rose daylight preserves the silhouettes of nightmare enemies.
    color = mix(color, vec3(1.0, 0.90, 0.97), 0.10);
    float vignette = smoothstep(0.18, 0.8, length(v_UV - 0.5));
    color *= 1.0 - vignette * 0.035;
    float grain =
        fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233)) + floor(u_Time * 12.0)) * 43758.5453);
    color += (grain - 0.5) * 0.002;
    FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
