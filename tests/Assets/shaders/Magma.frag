#version 460

// Buffers
// -----------------------------------------------------------------------------------------------------------------------

// In
layout (location = 0)  in vec3 v_FragPos;
layout (location = 1)  in vec3 v_Normal;
layout (location = 2)  in vec2 v_UV;
layout (location = 3)  in float v_LinearDepth;
layout (location = 4)  in float v_Time;

// Out
layout (location = 0) out vec4 out_color;
layout (location = 1) out vec4 out_positions;
layout (location = 2) out vec4 out_normals;
layout (location = 3) out vec4 out_materials;

vec3 fetchNormalMap() 
{
   return normalize(v_Normal);
}

layout (std140, binding = 102) uniform MaterialBuffer
{
   vec4 albedo;
   vec4 time;

} material;

// random2 function by Patricio Gonzalez
vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

// Value Noise by Inigo Quilez - iq/2013
// https://www.shadertoy.com/view/lsf3WH
float noise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    vec2 u = f*f*(3.0-2.0*f);

    return mix( mix( dot( random2(i + vec2(0.0,0.0) ), f - vec2(0.0,0.0) ), 
                     dot( random2(i + vec2(1.0,0.0) ), f - vec2(1.0,0.0) ), u.x),
                mix( dot( random2(i + vec2(0.0,1.0) ), f - vec2(0.0,1.0) ), 
                     dot( random2(i + vec2(1.0,1.0) ), f - vec2(1.0,1.0) ), u.x), u.y);
}

vec3 magmaFunc(vec3 color, vec2 uv, float detail, float power,
              float colorMul, float glowRate, bool animate, float noiseAmount)
{
    vec3 rockColor = vec3(0.09 + abs(sin(v_Time * .75)) * .03, 0.02, .02);
    float minDistance = 1.;
    uv *= detail;
    
    vec2 cell = floor(uv);
    vec2 frac = fract(uv);
    
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
        	vec2 cellDir = vec2(float(i), float(j));
            vec2 randPoint = random2(cell + cellDir);
            randPoint += noise(uv) * noiseAmount;
            randPoint = animate ? 0.5 + 0.5 * sin(v_Time * .35 + 6.2831 * randPoint) : randPoint;
            minDistance = min(minDistance, length(cellDir + randPoint - frac));
        }
    }
    	
    float powAdd = sin(uv.x * 2. + v_Time * glowRate) + sin(uv.y * 2. + v_Time * glowRate);
	vec3 outColor = vec3(color * pow(minDistance, power + powAdd * .95) * colorMul);
    outColor.rgb = mix(rockColor, outColor.rgb, minDistance);
    return outColor;
}

void main()
{
    vec2 uv = v_UV;
    uv.x += v_Time * .01;
    vec4 fragColor = vec4(0.);
    fragColor.rgb += magmaFunc(vec3(1.5, .8, 0.), uv, 3.,  2.5, 1.15, 1.5, false, 1.5);
    fragColor.rgb += magmaFunc(vec3(1.5, 0., 0.), uv, 6., 3., .4, 1., false, 0.);
    fragColor.rgb += magmaFunc(vec3(1.5, .4, 0.), uv, 8., 4., .2, 1.9, true, 0.8);
    
	vec3 N = fetchNormalMap(); 	
    out_color = fragColor;
    out_positions = vec4(v_FragPos, v_LinearDepth);
    out_normals = vec4(N, 1.0);
    out_materials = vec4(0.2, 0.7, 1.0, 1.0);
}