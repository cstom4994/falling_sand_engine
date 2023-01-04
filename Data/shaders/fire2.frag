// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#version 330

// Fragment
#ifdef GL_ES
precision mediump float;
#endif

// GLSL 330
out vec4 fragColor;
#define gl_FragColor fragColor

float intensity=1.0;// light transmition coeficient <0,1>
const int txrsiz=1200;     // max texture size [pixels]
uniform sampler2D firemap;   // texture unit for light map
uniform vec2 texSize;
in vec2 texCoord;

void main(){
    vec4 col = texture(firemap, vec2(texCoord.x, texCoord.y)); // GLSL 330
    vec4 sum = vec4(0);
    
    float num = 0.1;
    
    for(int xx = -3; xx <= 3; xx++){
        for(int yy = -3; yy <= 3; yy++){
            vec4 c = texture(firemap, vec2(texCoord.x + xx/texSize.x, texCoord.y + yy/texSize.y)); // GLSL 330
            float dist = (abs(float(xx)) + abs(float(yy))) / 2.0 + 1;
            dist = 1.0;
            
            float alphaFactor = 1.0;
            
            if(col.a == 0){
                if(c.a < 0.75){
                    alphaFactor = c.a;
                }else{
                    alphaFactor = 0.75;
                }
            }else if(col.a == 1.0){
                if(c.a > 0.1){
                    alphaFactor = c.a;
                }else{
                    alphaFactor = 0.1;
                }
            }
            
            sum += vec4(c.rgb, c.a / dist) * intensity / dist * alphaFactor;
            num += intensity / dist * alphaFactor;
        }
    }
    sum.r /= num;
    sum.g /= num;
    sum.b /= num;
    sum.a /= num;
    
    float r = col.r + sum.r;
    float g = col.g + sum.g;
    float b = col.b + sum.b;
    float a = col.a + sum.a;
    
    vec4 col1 = col * 0.25 + sum;
    
    gl_FragColor = col1;
    
}

