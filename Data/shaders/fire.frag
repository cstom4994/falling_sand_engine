// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#version 330

// Fragment
#ifdef GL_ES
precision mediump float;
#endif

// GLSL 330
out vec4 fragColor;
#define gl_FragColor fragColor

float intensity=0.7;// light transmition coeficient <0,1>
const int txrsiz=1200;     // max texture size [pixels]
uniform sampler2D firemap;   // texture unit for light map
uniform vec2 texSize;
in vec2 texCoord;

vec3 rgb2hsv(vec3 c){
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

void main(){
    vec4 col = texture(firemap, vec2(texCoord.x, texCoord.y)); // GLSL 330
    
    vec4 sum = vec4(0.1);
    vec4 c = vec4(0);
    
    float num = 0.1;
    
    if(col.a > 0){
        sum += col * 4;
        num += 4.0;
    }
    
    c = texture(firemap, vec2(texCoord.x + 1.0/texSize.x, texCoord.y)); // GLSL 330
    if(c.a > 0) {
        sum += c * intensity;
        num += intensity;
    }
    
    c = texture(firemap, vec2(texCoord.x - 1.0/texSize.x, texCoord.y)); // GLSL 330
    if(c.a > 0) {
        sum += c * intensity;
        num += intensity;
    }
    
    c = texture(firemap, vec2(texCoord.x, texCoord.y + 1.0/texSize.y)); // GLSL 330
    if(c.a > 0) {
        sum += c * intensity;
        num += intensity;
    }
    
    c = texture(firemap, vec2(texCoord.x, texCoord.y - 1.0/texSize.y)); // GLSL 330
    if(c.a > 0) {
        sum += c * intensity;
        num += intensity;
    }
    
    sum.r /= num;
    sum.g /= num;
    sum.b /= num;
    sum.a /= num + 0.5;
    
    gl_FragColor = sum;
}

