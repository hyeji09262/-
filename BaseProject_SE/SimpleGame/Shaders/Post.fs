#version 330
in vec2 v_UV;
uniform sampler2D u_Scene;
uniform vec2 u_Texel;
uniform float u_Time;
out vec4 FragColor;
void main() {
    vec3 scene=texture(u_Scene,v_UV).rgb;
    vec3 glow=vec3(0.0);
    // Small bright-pass blur: inexpensive bloom, intentionally softer than the geometry.
    for(int y=-2;y<=2;y++)for(int x=-2;x<=2;x++) {
        vec3 c=texture(u_Scene,v_UV+vec2(x,y)*u_Texel*3.0).rgb;
        glow+=max(c-vec3(0.58),vec3(0.0))/25.0;
    }
    vec3 color=scene+glow*0.6;
    color=mix(vec3(dot(color,vec3(0.2126,0.7152,0.0722))),color,1.08);
    color=color*vec3(1.04,1.00,1.06)+vec3(0.01,0.006,0.013);
    float vignette=smoothstep(0.18,0.8,length(v_UV-0.5));
    color*=1.0-vignette*0.24;
    float grain=fract(sin(dot(gl_FragCoord.xy,vec2(12.9898,78.233))+floor(u_Time*12.0))*43758.5453);
    color+=(grain-0.5)*0.006;
    FragColor=vec4(clamp(color,0.0,1.0),1.0);
}
