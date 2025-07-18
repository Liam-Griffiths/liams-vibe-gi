#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D inputTexture;

// FXAA Parameters
const float FXAA_SPAN_MAX = 8.0;
const float FXAA_REDUCE_MUL = 1.0/8.0;
const float FXAA_REDUCE_MIN = 1.0/128.0;

float rgb2luma(vec3 rgb) {
    return sqrt(dot(rgb, vec3(0.299, 0.587, 0.114)));
}

void main() {
    vec2 texelSize = 1.0 / textureSize(inputTexture, 0);
    
    // Sample the center pixel and its neighbors
    vec3 rgbNW = texture(inputTexture, TexCoords + vec2(-1.0, -1.0) * texelSize).rgb;
    vec3 rgbNE = texture(inputTexture, TexCoords + vec2( 1.0, -1.0) * texelSize).rgb;
    vec3 rgbSW = texture(inputTexture, TexCoords + vec2(-1.0,  1.0) * texelSize).rgb;
    vec3 rgbSE = texture(inputTexture, TexCoords + vec2( 1.0,  1.0) * texelSize).rgb;
    vec3 rgbM  = texture(inputTexture, TexCoords).rgb;
    
    // Convert to luminance
    float lumaNW = rgb2luma(rgbNW);
    float lumaNE = rgb2luma(rgbNE);
    float lumaSW = rgb2luma(rgbSW);
    float lumaSE = rgb2luma(rgbSE);
    float lumaM  = rgb2luma(rgbM);
    
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    
    // Calculate edge direction
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    
    dir = min(vec2(FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX), dir * rcpDirMin)) * texelSize;
    
    // Sample along the edge direction
    vec3 rgbA = 0.5 * (
        texture(inputTexture, TexCoords + dir * (1.0/3.0 - 0.5)).rgb +
        texture(inputTexture, TexCoords + dir * (2.0/3.0 - 0.5)).rgb);
    
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(inputTexture, TexCoords + dir * (0.0/3.0 - 0.5)).rgb +
        texture(inputTexture, TexCoords + dir * (3.0/3.0 - 0.5)).rgb);
    
    float lumaB = rgb2luma(rgbB);
    
    // Choose the better result
    if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
        FragColor = vec4(rgbA, 1.0);
    } else {
        FragColor = vec4(rgbB, 1.0);
    }
} 