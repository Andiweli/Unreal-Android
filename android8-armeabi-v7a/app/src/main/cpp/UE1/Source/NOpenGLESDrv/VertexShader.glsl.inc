R""(

attribute highp vec3 aPosition; // UNREAL_ANDROID_MALI_SHADER_PRECISION_V121
varying highp vec3 vPosition; // UNREAL_ANDROID_MALI_SHADER_PRECISION_V121
#ifdef SF_Texture0
attribute highp vec2 aTexCoord0; // UNREAL_ANDROID_MALI_SHADER_PRECISION_V121
varying mediump vec2 vTexCoord0;
#endif
#ifdef SF_Texture1
attribute highp vec2 aTexCoord1; // UNREAL_ANDROID_MALI_SHADER_PRECISION_V121
varying mediump vec2 vTexCoord1;
#endif
#ifdef SF_Texture2
attribute highp vec2 aTexCoord2; // UNREAL_ANDROID_MALI_SHADER_PRECISION_V121
varying mediump vec2 vTexCoord2;
#endif
#ifdef SF_Texture3
attribute highp vec2 aTexCoord3; // UNREAL_ANDROID_MALI_SHADER_PRECISION_V121
varying mediump vec2 vTexCoord3;
#endif
#ifdef SF_VtxColor
attribute lowp vec4 aVtxColor;
varying lowp vec4 vVtxColor;
#endif
#ifdef SF_VtxFog
attribute lowp vec4 aVtxFog;
varying lowp vec4 vVtxFog;
#endif

uniform highp mat4 uMtx; // UNREAL_ANDROID_MALI_SHADER_PRECISION_V121

void main()
{
#ifdef SF_Texture0
	vTexCoord0 = aTexCoord0;
#endif
#ifdef SF_Texture1
	vTexCoord1 = aTexCoord1;
#endif
#ifdef SF_Texture2
	vTexCoord2 = aTexCoord2;
#endif
#ifdef SF_Texture3
	vTexCoord3 = aTexCoord3;
#endif
#ifdef SF_VtxColor
	vVtxColor = aVtxColor;
#endif
#ifdef SF_VtxFog
	vVtxFog = aVtxFog;
#endif
#ifdef SF_Detail
	vPosition = aPosition;
#endif
	gl_Position = uMtx * vec4(aPosition, 1.0);
}

)""
