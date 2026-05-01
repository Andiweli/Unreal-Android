R""(

varying mediump vec3 vPosition;
#ifdef SF_Texture0
varying mediump vec2 vTexCoord0;
#endif
#ifdef SF_Texture1
varying mediump vec2 vTexCoord1;
#endif
#ifdef SF_Texture2
varying mediump vec2 vTexCoord2;
#endif
#ifdef SF_Texture3
varying mediump vec2 vTexCoord3;
#endif
#ifdef SF_VtxColor
varying lowp vec4 vVtxColor;
#endif
#ifdef SF_VtxFog
varying lowp vec4 vVtxFog;
#endif

uniform lowp float uBrightness;
uniform mediump float uBrightnessScale;
uniform mediump float uWorldGamma;
uniform mediump float uWorldShadowLift;

#ifdef SF_Texture0
uniform sampler2D uTexture0;
#endif
#ifdef SF_Texture1
uniform sampler2D uTexture1;
#endif
#ifdef SF_Texture2
uniform sampler2D uTexture2;
#endif
#ifdef SF_Texture3
uniform sampler2D uTexture3;
#endif

#ifndef LIGHTMAP_OVERBRIGHT
#define LIGHTMAP_OVERBRIGHT 1.0
#endif


void main()
{
	lowp vec4 Out = vec4(1.0);

#ifdef SF_Texture0
	lowp vec4 Texel0 = texture2D(uTexture0, vTexCoord0);
	Out = Texel0;
#endif

#ifdef SF_AlphaTest
	if( Out.a < 0.5 )
		discard;
#endif

#ifdef SF_Texture1
	lowp vec4 Texel1 = texture2D(uTexture1, vTexCoord1);
#ifdef SF_Lightmap
	Out.rgb *= Texel1.rgb * 2.0 * LIGHTMAP_OVERBRIGHT;
#else
	Out *= Texel1;
#endif
#endif

#ifdef SF_Texture2
	lowp vec4 Texel2 = texture2D(uTexture2, vTexCoord2);
#ifdef SF_Fogmap
	Texel2 *= 2.0;
	Out.rgb = Texel2.rgb + Out.rgb * (1.0 - Texel2.a);
	Out.a = Texel2.a;
#else
	Out *= Texel2;
#endif
#endif

#ifdef SF_Texture3
	lowp vec4 Texel3 = texture2D(uTexture3, vTexCoord3) * 2.0;
#ifdef SF_Detail
	// TODO: XOpenGLDrv does some RGB->HSV->RGB magic on detail maps, we just * 2 instead
	lowp float DetailScale = clamp(0.65 - vPosition.z / 380.0, 0.0, 1.0);
	Texel3 = mix(vec4(1.0), Texel3, DetailScale);
	Out.rgb *= Texel3.rgb;
#else
	Out.rgb *= Texel3.rgb;
#endif
#endif

#ifdef SF_VtxColor
	Out *= vVtxColor;
#endif

#ifdef SF_VtxFog
	Out.rgb = vVtxFog.rgb + Out.rgb * (1.0 - vVtxFog.a);
#endif

	// Brightness is back to the original UE1-style linear slider.
	// Extra Android tuning is separate: uWorldGamma/uWorldShadowLift only affect
	// lightmapped world geometry, never sprite/effect/HUD paths.
	mediump float BrightScale = (0.5 + 1.5 * uBrightness) * uBrightnessScale;
	Out.rgb *= BrightScale;

#ifdef SF_Lightmap
	mediump vec3 ClampedRgb = clamp(Out.rgb, 0.0, 1.0);
	mediump float ShadowLuma = max(max(ClampedRgb.r, ClampedRgb.g), ClampedRgb.b);
	mediump float ShadowMask = 1.0 - smoothstep(0.20, 0.85, ShadowLuma);
	mediump float SafeWorldGamma = max(uWorldGamma, 0.01);
	mediump vec3 GammaRgb = pow(ClampedRgb, vec3(1.0 / SafeWorldGamma));
	Out.rgb = mix(Out.rgb, GammaRgb, ShadowMask);
	Out.rgb += uWorldShadowLift * ShadowMask * (1.0 - ClampedRgb);
#endif

	gl_FragColor = Out;
}

)""
