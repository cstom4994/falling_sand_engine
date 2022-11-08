// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _SHADERGLSL_H_
#define _SHADERGLSL_H_

const char* glsl_frag_common = R"(
#version 150

in vec4 color;
in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D tex;

void main(void)
{
	fragColor = texture2D(tex, texCoord) * color;
}
)";

const char* glsl_vert_common = R"(
#version 150

attribute vec3 gpu_Vertex;
attribute vec2 gpu_TexCoord;
attribute vec4 gpu_Color;
uniform mat4 gpu_ModelViewProjectionMatrix;

varying vec4 color;
varying vec2 texCoord;

void main(void)
{
	color = gpu_Color;
	texCoord = vec2(gpu_TexCoord);
	gl_Position = gpu_ModelViewProjectionMatrix * vec4(gpu_Vertex, 1.0);
}
)";

const char* glsl_frag_water = R"(
// Copyright(c) 2022, KaoruXun

#version 150

#ifdef GL_ES
precision mediump float;
#endif

#extension GL_OES_standard_derivatives : enable

uniform sampler2D tex;
varying vec2 texCoord;

uniform float time;
uniform vec2 resolution;

uniform sampler2D mask;
uniform vec2 maskPos;
uniform vec2 maskSize;

uniform float scale;

uniform sampler2D flowTex;

uniform int overlay = 0;
uniform bool showFlow = true;
uniform bool pixelated = false;

//==================================================================
//https://github.com/stegu/webgl-noise/blob/master/src/classicnoise2D.glsl

//
// GLSL textureless classic 2D noise "cnoise",
// with an RSL-style periodic variant "pnoise".
// Author:  Stefan Gustavson (stefan.gustavson@liu.se)
// Version: 2011-08-22
//
// Many thanks to Ian McEwan of Ashima Arts for the
// ideas for permutation and gradient selection.
//
// Copyright (c) 2011 Stefan Gustavson. All rights reserved.
// Distributed under the MIT license. See LICENSE file.
// https://github.com/stegu/webgl-noise
//

vec4 mod289(vec4 x)
{
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x)
{
  return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

vec2 fade(vec2 t) {
  return t*t*t*(t*(t*6.0-15.0)+10.0);
}

// Classic Perlin noise
float cnoise(vec2 P)
{
    vec4 Pi = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);
    vec4 Pf = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);
    Pi = mod289(Pi); // To avoid truncation effects in permutation
    vec4 ix = Pi.xzxz;
    vec4 iy = Pi.yyww;
    vec4 fx = Pf.xzxz;
    vec4 fy = Pf.yyww;

    vec4 i = permute(permute(ix) + iy);

    vec4 gx = fract(i * (1.0 / 41.0)) * 2.0 - 1.0 ;
    vec4 gy = abs(gx) - 0.5 ;
    vec4 tx = floor(gx + 0.5);
    gx = gx - tx;

    vec2 g00 = vec2(gx.x,gy.x);
    vec2 g10 = vec2(gx.y,gy.y);
    vec2 g01 = vec2(gx.z,gy.z);
    vec2 g11 = vec2(gx.w,gy.w);

    vec4 norm = taylorInvSqrt(vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11)));
    g00 *= norm.x;  
    g01 *= norm.y;  
    g10 *= norm.z;  
    g11 *= norm.w;  

    float n00 = dot(g00, vec2(fx.x, fy.x));
    float n10 = dot(g10, vec2(fx.y, fy.y));
    float n01 = dot(g01, vec2(fx.z, fy.z));
    float n11 = dot(g11, vec2(fx.w, fy.w));

    vec2 fade_xy = fade(Pf.xy);
    vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
    float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
    return 2.3 * n_xy;
}
//==================================================================

void main( void ) {
    // this shader is applied to the background layer when it is drawn to the main buffer
    // gl_FragCoord is the pixel on screen
    // worldPos is the coordinate of the material in world space (from 0 to 1) (can be fractional since this shader is in screen space)
    // clworldPos (clipped worldPos) is worldPos but floored to a pixel material coordinate in world space (from 0 to 1)
    
    vec2 worldPos = (vec2(gl_FragCoord.x, (resolution.y / scale) - gl_FragCoord.y) - vec2(maskPos.x, maskPos.y)) / maskSize;
    
    // early exit if not in world
    if(worldPos.x < 0.0 || worldPos.y < 0.0 || worldPos.x >= 1.0 || worldPos.y >= 1.0){
        gl_FragColor = texture2D(tex, texCoord);
        return;
    }
    
    vec4 worldCol = texture2D(mask, vec2(worldPos.x, worldPos.y));
    
    // early exit if air
    if(worldCol.a == 0.0){
        gl_FragColor = texture2D(tex, texCoord);
        return;
    }
    
    // early exit if fully opaque
    if(worldCol.a > 0.99){
        return;
    }
    
    
    // apply the shader
    
    vec2 clworldPos = floor(worldPos * (maskSize/scale)) / (maskSize/scale);
    
    if(!pixelated) {
        clworldPos = worldPos;
    }
    
    // calculate flow
    
    vec4 flowCol = texture2D(flowTex, worldPos);
    
    float angle = 0;
    float speed = 0.0;

    if(showFlow){
        vec2 flow = vec2(flowCol.r, flowCol.g) - vec2(0.5);
        
        // clamp flow to only a few values so we get larger "chunks" with the same value
        // (this helps since the borders between values are not continuous)
        flow.r = int(flow.r * 10.0) / 10.0;
        flow.g = int(flow.g * 10.0) / 10.0;
        
        flowCol.r = flow.r + 0.5;
        flowCol.g = flow.g + 0.5;
        
        speed = length(flow.xy);
        angle = degrees(atan(flow.y, flow.x)) - 90.0;
    }
    
    // calculate positions for large and small layer
    vec2 position1 = vec2(clworldPos.x, -clworldPos.y);
    vec2 position2 = vec2(clworldPos.x, -clworldPos.y);
    
    position1 += time * 0.05 * speed * vec2(sin(radians(angle)), cos(radians(angle)));
    position2 += time * 0.1 * speed * vec2(sin(radians(angle)), cos(radians(angle)));

    // calculate noise position
    float noiseScale = pixelated ? 1.25 : 1.0;

    vec2 noisecoord1 = position1.xy * 300000.0 / noiseScale / resolution.x;
    vec2 noisecoord2 = position2.xy * 450000.0 / noiseScale / resolution.y + 4.0;
    
    vec2 motion1 = vec2(time * 0.15, time * -0.4);
    vec2 motion2 = vec2(time * -0.25, time * 0.25);
    
    // calculate noise layers
    vec2 distort1 = vec2(cnoise(noisecoord1 + motion1), cnoise(noisecoord2 + motion1));
    vec2 distort2 = vec2(cnoise(noisecoord1 + motion2), cnoise(noisecoord2 + motion2));
    
    vec2 distort_sum = (distort1 + distort2 * (speed + 0.25) * 5.0) / ((3.0 / scale) * 180.0);
    distort_sum /= 3.0;
    vec2 dstPlace = texCoord;
    
    //vec2 distortion = vec2(sin(time*2.0 + (clworldPos.y * maskSize.y / scale) / 5.0) * 4.0, 0.0);
    vec2 distortion = distort_sum;
    vec2 distorted_pos = clworldPos + distortion;
    vec2 dp = (distorted_pos * maskSize + maskPos) * scale / resolution;

    if(overlay == 1) {
        // flow map
        gl_FragColor = vec4(flowCol.rgb, 1.0);
    }else if(overlay == 2){
        // distortion
        gl_FragColor = vec4(length(distort_sum) * 150.0);
    }else{
        // normal output
        gl_FragColor = texture2D(tex, dp) * (1.0 + length(distort_sum) * (10.0 + speed * 15.0));
    }
}
)";

const char* glsl_frag_waterFlow = R"(
#version 150

#ifdef GL_ES
precision mediump float;
#endif

#extension GL_OES_standard_derivatives : enable

uniform sampler2D tex;
varying vec2 texCoord;

uniform vec2 resolution;

void main( void ) {
    //if(mod(time, 0.1) > 0.05) discard;

    vec2 worldPos = gl_FragCoord.xy / resolution;
    
    vec4 worldCol = texture2D(tex, vec2(worldPos.x, worldPos.y));
    if(worldCol.a > 0){
        
        vec4 flowCol = texture2D(tex, worldPos);
        
        float range = 16.0;
        float numSamples = 1.0;
        for(float xx = -range/2; xx <= range/2; xx += 1.0){
            for(float yy = -range/2; yy <= range/2; yy += 1.0){
                vec4 flowColScan = texture2D(tex, vec2(worldPos.x + xx/resolution.x, worldPos.y + yy/resolution.y));
                
                // invalid
                if(flowColScan.r == 0.0 && flowColScan.g == 0.0) continue;
                if(flowColScan.a == 0.0) continue;
                
                flowColScan.a = 0.0;
                
                //vec2 flow = vec2(flowColScan.r, flowColScan.g) - vec2(0.5);

                //speed += length(flow.xy);
                //angle += degrees(atan2(flow.y, flow.x)) - 90.0;
                vec2 flow2 = vec2(flowColScan.r, flowColScan.g) - vec2(0.5);
                float weight = flow2.x * flow2.x + flow2.y * flow2.y;
                flowCol += flowColScan * weight;
                numSamples += weight;
            }
        }
    
        flowCol /= numSamples;
        
        //angle /= (range * 2) * (range * 2);
        //speed /= (range * 2) * (range * 2);
        
        //flowCol = vec4(0.5, 0.0, 0.0, 0.0);
        //flowCol += vec4(0.5);
        
        vec2 flow = vec2(flowCol.r, flowCol.g) - vec2(0.5);
        //flowCol -= vec4(0.5);
        
        flowCol.r = flow.r + 0.5;
        flowCol.g = flow.g + 0.5;
        
        gl_FragColor = flowCol;
        
    }else{
        gl_FragColor = vec4(0.0);
    }

    //gl_FragColor = vec4(1, 0, 0, 1);
    
}
)";

const char* glsl_frag_fire = R"(
#version 150

// Fragment
#ifdef GL_ES
precision mediump float;
#endif

float intensity=0.7;// light transmition coeficient <0,1>
const int txrsiz=1200;     // max texture size [pixels]
uniform sampler2D firemap;   // texture unit for light map
uniform vec2 texSize;
varying vec2 texCoord;

vec3 rgb2hsv(vec3 c){
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

void main(){
    vec4 col = texture2D(firemap, vec2(texCoord.x, texCoord.y));
    
    vec4 sum = vec4(0.1);
    vec4 c = vec4(0);
    
    float num = 0.1;
    
    if(col.a > 0){
        sum += col * 4;
        num += 4.0;
    }
    
    c = texture2D(firemap, vec2(texCoord.x + 1.0/texSize.x, texCoord.y));
    if(c.a > 0) {
        sum += c * intensity;
        num += intensity;
    }
    
    c = texture2D(firemap, vec2(texCoord.x - 1.0/texSize.x, texCoord.y));
    if(c.a > 0) {
        sum += c * intensity;
        num += intensity;
    }
    
    c = texture2D(firemap, vec2(texCoord.x, texCoord.y + 1.0/texSize.y));
    if(c.a > 0) {
        sum += c * intensity;
        num += intensity;
    }
    
    c = texture2D(firemap, vec2(texCoord.x, texCoord.y - 1.0/texSize.y));
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

)";
const char* glsl_frag_fire2 = R"(
#version 150

// Fragment
#ifdef GL_ES
precision mediump float;
#endif

float intensity=1.0;// light transmition coeficient <0,1>
const int txrsiz=1200;     // max texture size [pixels]
uniform sampler2D firemap;   // texture unit for light map
uniform vec2 texSize;
varying vec2 texCoord;

void main(){
    vec4 col = texture2D(firemap, vec2(texCoord.x, texCoord.y));
    vec4 sum = vec4(0);
    
    float num = 0.1;
    
    for(int xx = -3; xx <= 3; xx++){
        for(int yy = -3; yy <= 3; yy++){
            vec4 c = texture2D(firemap, vec2(texCoord.x + xx/texSize.x, texCoord.y + yy/texSize.y));
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


)";

const char* glsl_frag_newLighting = R"(
// Copyright(c) 2022, KaoruXun All rights reserved.

#version 150

// Fragment
#ifdef GL_ES
precision mediump float;
#endif

uniform bool simpleOnly = false;
uniform bool emission = true;
uniform bool dithering = false;
uniform float lightingQuality = 0.5;
uniform float inside = 0.0;

uniform float minX = 0.0;
uniform float minY = 0.0;
uniform float maxX = 0.0;
uniform float maxY = 0.0;


uniform sampler2D txrmap;   // texture unit for light map
uniform sampler2D emitmap;
uniform vec2 texSize;
varying vec2 texCoord;
uniform vec2 t0;

float light(vec4 col){
    return 1.0 - col.a;
}

vec4 light2(vec2 coord){
    vec4 col = texture2D(txrmap, coord);
    return vec4(1.0 - vec3(col.a), 1.0);
}

vec4 lightEmit(vec2 coord){
    if(!emission) return vec4(0.0);
    vec4 emit = texture2D(emitmap, coord);
    return emit * emit.a;
}

float brightnessContrast(float value, float brightness, float contrast){
    return (value - 0.5) * contrast + 0.5 + brightness;
}

vec4 brightnessContrast2(vec4 value, float brightness, float contrast){
    return vec4((value.rgb - vec3(0.5)) * contrast + 0.5 + brightness, 1.0);
}

const float DITHER_NOISE = 0.004;

// based on https://shader-tutorial.dev/advanced/color-banding-dithering/ (MIT license)
float random(vec2 coords) {
   return fract(sin(dot(coords.xy, vec2(12.9898,78.233))) * 43758.5453);
}

void main(){
    if(texCoord.x < (minX / texSize.x) || texCoord.x > (maxX / texSize.x) || texCoord.y < (minY / texSize.y) || texCoord.y > (maxY / texSize.y)){
        // only basic lighting outside visible area
        float dst2 = distance(texCoord * texSize * vec2(0.75, 1.0), t0 * texSize * vec2(0.75, 1.0)) / 3000.0;
        
        float dark = clamp(1.0 - dst2 * 3.5 * inside, 0.0, 1.0);
        // gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0 - dark);
        gl_FragColor = vec4(vec3(dark), 1.0);
    }else{
        float dst = distance(texCoord * texSize * vec2(0.75, 1.0), t0 * texSize * vec2(0.75, 1.0)) / 3000.0;
        float distBr = 1.0 - dst * 3.5 * inside;
        if(distBr <= -1.0){
            gl_FragColor = vec4(0.5, 0.0, 0.0, 1.0);
        }else{
            distBr = clamp(distBr, 0.0, 1.0);
            vec4 olcol = texture2D(txrmap, texCoord);
            float distNr = 1.0 - clamp(dst * 10.0, 0.0, 1.0);
            distNr *= 1.0;
            if(!simpleOnly && olcol.a > 0){
                // actual tile
            
                vec4 lcol = light2(texCoord);
                vec4 ecol = lightEmit(texCoord);
                vec4 emitCol = vec4(0.0);
                
                float nDirs = 10.0 + int(16.0 * lightingQuality);
                float nSteps = 5.0 + int(10.0 * lightingQuality);
                vec2 rad = 64.0 / texSize.xy;
                
                for(float deg = 0.0; deg < 3.1415927 * 2; deg += (3.1415927 * 2) / nDirs){
                    for(float i = 1.0 / nSteps; i <= 1.0; i += 1.0 / nSteps){
                        lcol += light2(texCoord + vec2(cos(deg), sin(deg)) * rad * i);	
                        if(emission) ecol += lightEmit(texCoord + vec2(cos(deg), sin(deg)) * rad * i);	
                    }
                }
                
                lcol.rgb /= nSteps * nDirs - 15.0;
                if(emission) ecol.rgb /= nSteps * nDirs - 15.0;
                
                vec4 c = brightnessContrast2(lcol, 0.2, 0.9) * distBr;
                if(dithering){
                    c += vec4(mix(-DITHER_NOISE * 5, DITHER_NOISE * 5, random(gl_FragCoord.xy / texSize))) * c * (1.0 - lightingQuality * 0.8);
                }
                vec4 brr = clamp(c, 0.0, 1.0);
                brr = mix(brr, vec4(1.0), distNr);
                
                if(emission) brr += pow(ecol, vec4(0.6));
                //gl_FragColor = mix(vec4(vec3(0.0), 1.0), olcol, brr); // mix orig color
                gl_FragColor = brr; // b/w
                //gl_FragColor = vec4(vec3(0.0), 1.0 - brr); // transparent black
            }else{
                // no tile (background)
            
                //float c = clamp(mix(distBr, 1.0, distNr), 0.0, 1.0);
                float c = clamp(distBr, 0.0, 1.0);
                if(dithering){
                    c += mix(-DITHER_NOISE, DITHER_NOISE, random(gl_FragCoord.xy / texSize));
                }
                //c = floor(c * 100.0) / 100.0;
                vec4 col = vec4(vec3(c), 1.0);
            
                if(!simpleOnly && emission){
                    vec4 ecol = lightEmit(texCoord);
                    vec4 emitCol = vec4(0.0);
                    
                    float nDirs = 10.0 + int(16.0 * lightingQuality);
                    float nSteps = 5.0 + int(10.0 * lightingQuality);
                    vec2 rad = 64.0 / texSize.xy;
                    
                    for(float deg = 0.0; deg < 3.1415927 * 2; deg += (3.1415927 * 2) / nDirs){
                        for(float i = 1.0 / nSteps; i <= 1.0; i += 1.0 / nSteps){
                            if(emission) ecol += lightEmit(texCoord + vec2(cos(deg), sin(deg)) * rad * i);	
                        }
                    }
                    ecol.rgb /= nSteps * nDirs - 15.0;
                    
                    col += pow(ecol, vec4(0.6));
                }
            
                //gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0 - clamp(mix(distBr, 1.0, distNr), 0.0, 1.0));
                gl_FragColor = col;
                //gl_FragColor = vec4(vec3(0.0), 1.0 - clamp(mix(distBr, 1.0, distNr), 0.0, 1.0));
            }
        }
    }
    gl_FragColor.a = 1.0;
}
)";

#endif