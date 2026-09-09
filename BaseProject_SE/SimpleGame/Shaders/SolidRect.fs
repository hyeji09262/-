#version 330
in vec2 v_UV;
in vec4 v_Color;
uniform sampler2D u_Atlas;
uniform bool u_Textured;
out vec4 FragColor;
void main() {
    FragColor=v_Color;
    if(u_Textured) FragColor.a*=texture(u_Atlas,v_UV).a;
}
