#version 450
layout(set=0,binding=0) uniform Parameters {
    vec4 axis[3], eye, color, surface, metal, shape;
};
layout(set=1,binding=0) uniform sampler2D baseTexture;
layout(set=2,binding=0) uniform sampler2D normalRoughnessTexture;
layout(set=3,binding=0) uniform sampler2D emissiveMetallicTexture;
layout(set=4,binding=0) uniform sampler2D probeAtlas;
layout(location=0) in vec2 uv;
layout(location=1) in vec3 objectPosition;
layout(location=2) in vec3 objectNormal;
layout(location=3) in vec4 objectTangent;
layout(location=0) out vec4 outColor;
vec3 unitOr(vec3 v,vec3 fallback) {
    float scale=max(max(abs(v.x),abs(v.y)),abs(v.z));
    return scale<1e-12?fallback:normalize(v/scale);
}
vec3 radiance(vec3 r,float level) {
    vec3 a=abs(r);
    float face; vec2 p;
    if(a.x>=a.y && a.x>=a.z) {
        face=r.x>=0.0?0.0:1.0;
        p=vec2(r.x>=0.0?-r.y:r.y,-r.z)/a.x;
    } else if(a.y>=a.z) {
        face=r.y>=0.0?2.0:3.0;
        p=vec2(r.y>=0.0?r.x:-r.x,-r.z)/a.y;
    } else {
        face=r.z>=0.0?4.0:5.0;
        p=vec2(-r.y,r.z>=0.0?r.x:-r.x)/a.z;
    }
    vec2 pixel=1.0/vec2(textureSize(probeAtlas,0));
    vec2 corner=vec2(face/6.0,level/5.0);
    vec2 coord=corner+(p*.5+.5)/vec2(6,5);
    return texture(probeAtlas,clamp(coord,corner+pixel*.5,corner+1.0/vec2(6,5)-pixel*.5)).rgb;
}
void main() {
    vec3 n=unitOr(objectNormal,vec3(0,0,1));
    vec4 nr=texture(normalRoughnessTexture,uv);
    vec3 t=objectTangent.xyz;
    float handedness=objectTangent.w;
    vec3 dx=dFdx(objectPosition),dy=dFdy(objectPosition);
    vec2 ux=dFdx(uv),uy=dFdy(uv);
    if(abs(handedness)<.5) {
        float det=ux.x*uy.y-ux.y*uy.x;
        t=(dx*uy.y-dy*ux.y)*sign(det);
        vec3 b=(dy*ux.x-dx*uy.x)*sign(det);
        handedness=dot(cross(n,t),b)<0.0?-1.0:1.0;
    }
    vec3 fallback=abs(n.z)<.99?unitOr(cross(vec3(0,0,1),n),vec3(1,0,0)):vec3(1,0,0);
    t=unitOr(t-n*dot(t,n),fallback);
    vec3 mapped=unitOr(vec3((nr.xy*2.0-1.0)*surface.y,nr.z*2.0-1.0),vec3(0,0,1));
    n=unitOr(mat3(t,cross(n,t)*handedness,n)*mapped,n);
    if(!gl_FrontFacing) n=-n;
    mat3 model=mat3(axis[0].xyz,axis[1].xyz,axis[2].xyz);
    n=unitOr(transpose(inverse(model))*n,vec3(0,0,1));
    vec3 v=unitOr(model*(eye.xyz-objectPosition),n);
    vec3 r=reflect(-v,n);
    float roughness=clamp(nr.a*surface.x,0.0,1.0);
    float level=roughness*4.0;
    vec3 reflected=mix(radiance(r,floor(level)),radiance(r,ceil(level)),fract(level));
    vec3 base=texture(baseTexture,uv).rgb*color.rgb;
    float metallic=clamp(texture(emissiveMetallicTexture,uv).a*metal.x,0.0,1.0);
    vec3 f0=mix(vec3(.04),base,metallic);
    vec3 fresnel=f0+(1.0-f0)*pow(1.0-max(dot(n,v),0.0),5.0);
    // ponytail: local radiance with Schlick response, no parallax correction;
    // box projection belongs with authored influence volumes when needed.
    outColor=vec4(reflected*fresnel*shape.z,0.0);
}
