// DMG Dot Matrix Shader
// Initial version built by Status_Librarian_313
// shared on Reddit: https://www.reddit.com/r/trimui/s/CfQup5U7ek
// Modified by sinedied (http://github.com/sinedied/)

#pragma parameter dmg_edge_alpha "Grid opacity" 0.3 0.0 1.0 0.01
#pragma parameter dmg_brightness_correction "Brightness correction" 1.2 0.5 2.0 0.01
#pragma parameter dmg_grid_lightness "Grid lightness" 1.0 0.0 1.0 0.01
#pragma parameter dmg_gamma "Gamma" 1.4 0.5 2.0 0.1

#ifdef VERTEX
attribute vec4 VertexCoord, TexCoord;
varying vec4 TEX0;
uniform mat4 MVPMatrix;

void main() {
   TEX0 = TexCoord;
   gl_Position = MVPMatrix * VertexCoord;
}
#else
precision highp float;

uniform sampler2D Texture;
uniform vec2 OutputSize;
uniform vec2 TextureSize;
uniform float dmg_edge_alpha;
uniform float dmg_brightness_correction;
uniform float dmg_grid_lightness;
uniform float dmg_gamma;

varying vec4 TEX0;

void main() {
    vec2 screenCoord = TEX0.xy * OutputSize;
    vec2 texelSize = OutputSize / TextureSize;

    float lineWidth = 1.0;
    float edgeX = step(texelSize.x - lineWidth, mod(screenCoord.x, texelSize.x));
    float edgeY = step(texelSize.y - lineWidth, mod(screenCoord.y, texelSize.y));
    float gridMask = max(edgeX, edgeY);

    vec3 color = texture2D(Texture, TEX0.xy).rgb * dmg_brightness_correction;
    vec3 gridColor = vec3(dmg_grid_lightness);

    vec3 finalColor = mix(color, gridColor, gridMask * dmg_edge_alpha);
    finalColor = pow(clamp(finalColor, 0.0, 1.0), vec3(dmg_gamma));
    gl_FragColor = vec4(finalColor, 1.0);
}
#endif
