// Copyright (C) 2014 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in Irrlicht.h

#include "FixedPipelineRenderer.h"

#include "IVideoDriver.h"

namespace irr
{
namespace video
{

// Base callback

COpenGL3MaterialBaseCB::COpenGL3MaterialBaseCB() :
	FirstUpdateBase(true), WVPMatrixID(-1), WVMatrixID(-1), NMatrixID(-1), GlobalAmbientID(-1), MaterialAmbientID(-1), MaterialDiffuseID(-1), MaterialEmissiveID(-1), MaterialSpecularID(-1), MaterialShininessID(-1),
	FogEnableID(-1), FogTypeID(-1), FogColorID(-1), FogStartID(-1),
	FogEndID(-1), FogDensityID(-1), ThicknessID(-1), LightEnable(false), MaterialAmbient(SColorf(0.f, 0.f, 0.f)), MaterialDiffuse(SColorf(0.f, 0.f, 0.f)), MaterialEmissive(SColorf(0.f, 0.f, 0.f)), MaterialSpecular(SColorf(0.f, 0.f, 0.f)),
	MaterialShininess(0.f), FogEnable(0), FogType(1), FogColor(SColorf(0.f, 0.f, 0.f, 1.f)), FogStart(0.f), FogEnd(0.f), FogDensity(0.f), Thickness(1.f)
{
}

void COpenGL3MaterialBaseCB::OnSetMaterial(const SMaterial& material)
{
	LightEnable = material.Lighting;
	MaterialAmbient = SColorf(material.AmbientColor);
	MaterialDiffuse = SColorf(material.DiffuseColor);
	MaterialEmissive = SColorf(material.EmissiveColor);
	MaterialSpecular = SColorf(material.SpecularColor);
	MaterialShininess = material.Shininess;

	FogEnable = material.FogEnable ? 1 : 0;

	Thickness = (material.Thickness > 0.f) ? material.Thickness : 1.f;
}

void COpenGL3MaterialBaseCB::OnSetConstants(IMaterialRendererServices* services, s32 userData)
{
	IVideoDriver* driver = services->getVideoDriver();

	if (FirstUpdateBase)
	{
		WVPMatrixID = services->getVertexShaderConstantID("uWVPMatrix");
		WVMatrixID = services->getVertexShaderConstantID("uWVMatrix");
		NMatrixID = services->getVertexShaderConstantID("uNMatrix");
		GlobalAmbientID = services->getVertexShaderConstantID("uGlobalAmbient");
		MaterialAmbientID = services->getVertexShaderConstantID("uMaterialAmbient");
		MaterialDiffuseID = services->getVertexShaderConstantID("uMaterialDiffuse");
		MaterialEmissiveID = services->getVertexShaderConstantID("uMaterialEmissive");
		MaterialSpecularID = services->getVertexShaderConstantID("uMaterialSpecular");
		MaterialShininessID = services->getVertexShaderConstantID("uMaterialShininess");
		FogEnableID = services->getVertexShaderConstantID("uFogEnable");
		FogTypeID = services->getVertexShaderConstantID("uFogType");
		FogColorID = services->getVertexShaderConstantID("uFogColor");
		FogStartID = services->getVertexShaderConstantID("uFogStart");
		FogEndID = services->getVertexShaderConstantID("uFogEnd");
		FogDensityID = services->getVertexShaderConstantID("uFogDensity");
		ThicknessID = services->getVertexShaderConstantID("uThickness");

		FirstUpdateBase = false;
	}

	const core::matrix4 W = driver->getTransform(ETS_WORLD);
	const core::matrix4 V = driver->getTransform(ETS_VIEW);
	const core::matrix4 P = driver->getTransform(ETS_PROJECTION);

	core::matrix4 Matrix = P * V * W;
	services->setPixelShaderConstant(WVPMatrixID, Matrix.pointer(), 16);

	Matrix = V * W;
	services->setPixelShaderConstant(WVMatrixID, Matrix.pointer(), 16);

	Matrix.makeInverse();
	services->setPixelShaderConstant(NMatrixID, Matrix.getTransposed().pointer(), 16);

	services->setPixelShaderConstant(FogEnableID, &FogEnable, 1);

	if (FogEnable)
	{
		SColor TempColor(0);
		E_FOG_TYPE TempType = EFT_FOG_LINEAR;
		bool TempPerFragment = false;
		bool TempRange = false;

		driver->getFog(TempColor, TempType, FogStart, FogEnd, FogDensity, TempPerFragment, TempRange);

		FogType = (s32)TempType;
		FogColor = SColorf(TempColor);

		services->setPixelShaderConstant(FogTypeID, &FogType, 1);
		services->setPixelShaderConstant(FogColorID, reinterpret_cast<f32*>(&FogColor), 4);
		services->setPixelShaderConstant(FogStartID, &FogStart, 1);
		services->setPixelShaderConstant(FogEndID, &FogEnd, 1);
		services->setPixelShaderConstant(FogDensityID, &FogDensity, 1);
	}

	services->setPixelShaderConstant(ThicknessID, &Thickness, 1);
}

// EMT_SOLID + EMT_TRANSPARENT_ADD_COLOR + EMT_TRANSPARENT_ALPHA_CHANNEL + EMT_TRANSPARENT_VERTEX_ALPHA

COpenGL3MaterialSolidCB::COpenGL3MaterialSolidCB() :
	FirstUpdate(true), TMatrix0ID(-1), AlphaRefID(-1), TextureUsage0ID(-1), TextureUnit0ID(-1), AlphaRef(0.5f), TextureUsage0(0), TextureUnit0(0)
{
}

void COpenGL3MaterialSolidCB::OnSetMaterial(const SMaterial& material)
{
	COpenGL3MaterialBaseCB::OnSetMaterial(material);

	AlphaRef = material.MaterialTypeParam;
	TextureUsage0 = (material.TextureLayer[0].Texture) ? 1 : 0;
}

void COpenGL3MaterialSolidCB::OnSetConstants(IMaterialRendererServices* services, s32 userData)
{
	COpenGL3MaterialBaseCB::OnSetConstants(services, userData);

	IVideoDriver* driver = services->getVideoDriver();

	if (FirstUpdate)
	{
		TMatrix0ID = services->getVertexShaderConstantID("uTMatrix0");
		AlphaRefID = services->getVertexShaderConstantID("uAlphaRef");
		TextureUsage0ID = services->getVertexShaderConstantID("uTextureUsage0");
		TextureUnit0ID = services->getVertexShaderConstantID("uTextureUnit0");

		FirstUpdate = false;
	}

	core::matrix4 Matrix = driver->getTransform(ETS_TEXTURE_0);
	services->setPixelShaderConstant(TMatrix0ID, Matrix.pointer(), 16);

	services->setPixelShaderConstant(AlphaRefID, &AlphaRef, 1);
	services->setPixelShaderConstant(TextureUsage0ID, &TextureUsage0, 1);
	services->setPixelShaderConstant(TextureUnit0ID, &TextureUnit0, 1);
}

// EMT_SOLID_2_LAYER + EMT_DETAIL_MAP

COpenGL3MaterialSolid2CB::COpenGL3MaterialSolid2CB() :
	FirstUpdate(true), TMatrix0ID(-1), TMatrix1ID(-1), TextureUsage0ID(-1), TextureUsage1ID(-1), TextureUnit0ID(-1), TextureUnit1ID(-1),
	TextureUsage0(0), TextureUsage1(0), TextureUnit0(0), TextureUnit1(1)
{
}

void COpenGL3MaterialSolid2CB::OnSetMaterial(const SMaterial& material)
{
	COpenGL3MaterialBaseCB::OnSetMaterial(material);

	TextureUsage0 = (material.TextureLayer[0].Texture) ? 1 : 0;
	TextureUsage1 = (material.TextureLayer[1].Texture) ? 1 : 0;
}

void COpenGL3MaterialSolid2CB::OnSetConstants(IMaterialRendererServices* services, s32 userData)
{
	COpenGL3MaterialBaseCB::OnSetConstants(services, userData);

	IVideoDriver* driver = services->getVideoDriver();

	if (FirstUpdate)
	{
		TMatrix0ID = services->getVertexShaderConstantID("uTMatrix0");
		TMatrix1ID = services->getVertexShaderConstantID("uTMatrix1");
		TextureUsage0ID = services->getVertexShaderConstantID("uTextureUsage0");
		TextureUsage1ID = services->getVertexShaderConstantID("uTextureUsage1");
		TextureUnit0ID = services->getVertexShaderConstantID("uTextureUnit0");
		TextureUnit1ID = services->getVertexShaderConstantID("uTextureUnit1");

		FirstUpdate = false;
	}

	core::matrix4 Matrix = driver->getTransform(ETS_TEXTURE_0);
	services->setPixelShaderConstant(TMatrix0ID, Matrix.pointer(), 16);

	Matrix = driver->getTransform(E_TRANSFORMATION_STATE(ETS_TEXTURE_0 + 1));
	services->setPixelShaderConstant(TMatrix1ID, Matrix.pointer(), 16);

	services->setPixelShaderConstant(TextureUsage0ID, &TextureUsage0, 1);
	services->setPixelShaderConstant(TextureUsage1ID, &TextureUsage1, 1);
	services->setPixelShaderConstant(TextureUnit0ID, &TextureUnit0, 1);
	services->setPixelShaderConstant(TextureUnit1ID, &TextureUnit1, 1);
}

// EMT_LIGHTMAP + EMT_LIGHTMAP_ADD + EMT_LIGHTMAP_M2 + EMT_LIGHTMAP_M4

COpenGL3MaterialLightmapCB::COpenGL3MaterialLightmapCB(float modulate) :
	FirstUpdate(true), TMatrix0ID(-1), TMatrix1ID(-1), ModulateID(-1), TextureUsage0ID(-1), TextureUsage1ID(-1), TextureUnit0ID(-1), TextureUnit1ID(-1),
	Modulate(modulate), TextureUsage0(0), TextureUsage1(0), TextureUnit0(0), TextureUnit1(1)
{
}

void COpenGL3MaterialLightmapCB::OnSetMaterial(const SMaterial& material)
{
	COpenGL3MaterialBaseCB::OnSetMaterial(material);

	TextureUsage0 = (material.TextureLayer[0].Texture) ? 1 : 0;
	TextureUsage1 = (material.TextureLayer[1].Texture) ? 1 : 0;
}

void COpenGL3MaterialLightmapCB::OnSetConstants(IMaterialRendererServices* services, s32 userData)
{
	COpenGL3MaterialBaseCB::OnSetConstants(services, userData);

	IVideoDriver* driver = services->getVideoDriver();

	if (FirstUpdate)
	{
		TMatrix0ID = services->getVertexShaderConstantID("uTMatrix0");
		TMatrix1ID = services->getVertexShaderConstantID("uTMatrix1");
		ModulateID = services->getVertexShaderConstantID("uModulate");
		TextureUsage0ID = services->getVertexShaderConstantID("uTextureUsage0");
		TextureUsage1ID = services->getVertexShaderConstantID("uTextureUsage1");
		TextureUnit0ID = services->getVertexShaderConstantID("uTextureUnit0");
		TextureUnit1ID = services->getVertexShaderConstantID("uTextureUnit1");

		FirstUpdate = false;
	}

	core::matrix4 Matrix = driver->getTransform(ETS_TEXTURE_0);
	services->setPixelShaderConstant(TMatrix0ID, Matrix.pointer(), 16);

	Matrix = driver->getTransform(E_TRANSFORMATION_STATE(ETS_TEXTURE_0 + 1));
	services->setPixelShaderConstant(TMatrix1ID, Matrix.pointer(), 16);

	services->setPixelShaderConstant(ModulateID, &Modulate, 1);
	services->setPixelShaderConstant(TextureUsage0ID, &TextureUsage0, 1);
	services->setPixelShaderConstant(TextureUsage1ID, &TextureUsage1, 1);
	services->setPixelShaderConstant(TextureUnit0ID, &TextureUnit0, 1);
	services->setPixelShaderConstant(TextureUnit1ID, &TextureUnit1, 1);
}

// EMT_SPHERE_MAP + EMT_REFLECTION_2_LAYER + EMT_TRANSPARENT_REFLECTION_2_LAYER

COpenGL3MaterialReflectionCB::COpenGL3MaterialReflectionCB() :
	FirstUpdate(true), TMatrix0ID(-1), TextureUsage0ID(-1), TextureUsage1ID(-1), TextureUnit0ID(-1), TextureUnit1ID(-1),
	TextureUsage0(0), TextureUsage1(0), TextureUnit0(0), TextureUnit1(1)
{
}

void COpenGL3MaterialReflectionCB::OnSetMaterial(const SMaterial& material)
{
	COpenGL3MaterialBaseCB::OnSetMaterial(material);

	TextureUsage0 = (material.TextureLayer[0].Texture) ? 1 : 0;
	TextureUsage1 = (material.TextureLayer[1].Texture) ? 1 : 0;
}

void COpenGL3MaterialReflectionCB::OnSetConstants(IMaterialRendererServices* services, s32 userData)
{
	COpenGL3MaterialBaseCB::OnSetConstants(services, userData);

	IVideoDriver* driver = services->getVideoDriver();

	if (FirstUpdate)
	{
		TMatrix0ID = services->getVertexShaderConstantID("uTMatrix0");
		TextureUsage0ID = services->getVertexShaderConstantID("uTextureUsage0");
		TextureUsage1ID = services->getVertexShaderConstantID("uTextureUsage1");
		TextureUnit0ID = services->getVertexShaderConstantID("uTextureUnit0");
		TextureUnit1ID = services->getVertexShaderConstantID("uTextureUnit1");

		FirstUpdate = false;
	}

	core::matrix4 Matrix = driver->getTransform(ETS_TEXTURE_0);
	services->setPixelShaderConstant(TMatrix0ID, Matrix.pointer(), 16);

	services->setPixelShaderConstant(TextureUsage0ID, &TextureUsage0, 1);
	services->setPixelShaderConstant(TextureUsage1ID, &TextureUsage1, 1);
	services->setPixelShaderConstant(TextureUnit0ID, &TextureUnit0, 1);
	services->setPixelShaderConstant(TextureUnit1ID, &TextureUnit1, 1);
}

// EMT_ONETEXTURE_BLEND

COpenGL3MaterialOneTextureBlendCB::COpenGL3MaterialOneTextureBlendCB() :
	FirstUpdate(true), TMatrix0ID(-1), BlendTypeID(-1), TextureUsage0ID(-1), TextureUnit0ID(-1), BlendType(0), TextureUsage0(0), TextureUnit0(0)
{
}

void COpenGL3MaterialOneTextureBlendCB::OnSetMaterial(const SMaterial& material)
{
	COpenGL3MaterialBaseCB::OnSetMaterial(material);

	BlendType = 0;

	E_BLEND_FACTOR srcRGBFact,dstRGBFact,srcAlphaFact,dstAlphaFact;
	E_MODULATE_FUNC modulate;
	u32 alphaSource;
	unpack_textureBlendFuncSeparate(srcRGBFact, dstRGBFact, srcAlphaFact, dstAlphaFact, modulate, alphaSource, material.MaterialTypeParam);

	if (textureBlendFunc_hasAlpha(srcRGBFact) || textureBlendFunc_hasAlpha(dstRGBFact) || textureBlendFunc_hasAlpha(srcAlphaFact) || textureBlendFunc_hasAlpha(dstAlphaFact))
	{
		if (alphaSource == EAS_VERTEX_COLOR)
		{
			BlendType = 1;
		}
		else if (alphaSource == EAS_TEXTURE)
		{
			BlendType = 2;
		}
	}

	TextureUsage0 = (material.TextureLayer[0].Texture) ? 1 : 0;
}

void COpenGL3MaterialOneTextureBlendCB::OnSetConstants(IMaterialRendererServices* services, s32 userData)
{
	COpenGL3MaterialBaseCB::OnSetConstants(services, userData);

	IVideoDriver* driver = services->getVideoDriver();

	if (FirstUpdate)
	{
		TMatrix0ID = services->getVertexShaderConstantID("uTMatrix0");
		BlendTypeID = services->getVertexShaderConstantID("uBlendType");
		TextureUsage0ID = services->getVertexShaderConstantID("uTextureUsage0");
		TextureUnit0ID = services->getVertexShaderConstantID("uTextureUnit0");

		FirstUpdate = false;
	}

	core::matrix4 Matrix = driver->getTransform(ETS_TEXTURE_0);
	services->setPixelShaderConstant(TMatrix0ID, Matrix.pointer(), 16);

	services->setPixelShaderConstant(BlendTypeID, &BlendType, 1);
	services->setPixelShaderConstant(TextureUsage0ID, &TextureUsage0, 1);
	services->setPixelShaderConstant(TextureUnit0ID, &TextureUnit0, 1);
}

}
}
