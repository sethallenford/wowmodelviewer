#include "ModelRenderPass.h"
#include "ModelColor.h"
#include "ModelTransparency.h"
#include "TextureAnim.h"
#include "video.h"
#include "wow_enums.h"
#include "WoWModel.h"

#include "logger/Logger.h"

#include "GL/glew.h"
#include "glm/gtc/type_ptr.hpp"

ModelRenderPass::ModelRenderPass(WoWModel* m, int geo):
	useTex2(false), useEnvMap(false), cull(false), trans(false),
	unlit(false), noZWrite(false), billboard(false),
	texanim(-1), color(-1), opacity(-1), blendmode(-1), tex(INVALID_TEX),
	swrap(false), twrap(false), ocol(0.0f, 0.0f, 0.0f, 0.0f), ecol(0.0f, 0.0f, 0.0f, 0.0f),
	model(m), geoIndex(geo), specialTex(-1)
{
}

void ModelRenderPass::deinit()
{
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (noZWrite)
		glDepthMask(GL_TRUE);

	if (texanim != -1)
	{
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}

	if (unlit)
		glEnable(GL_LIGHTING);

	//if (billboard)
	//  glPopMatrix();

	if (cull)
		glDisable(GL_CULL_FACE);

	if (useEnvMap)
	{
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
	}

	if (swrap)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

	if (twrap)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	/*
	  if (useTex2)
	  {
	    glDisable(GL_TEXTURE_2D);
	    glActiveTextureARB(GL_TEXTURE0);
	  }
	 */

	if (opacity != -1 || color != -1)
	{
		const GLfloat czero[4] = {0.0f, 0.0f, 0.0f, 1.0f};
		glMaterialfv(GL_FRONT, GL_EMISSION, czero);

		//glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		//glMaterialfv(GL_FRONT, GL_AMBIENT, ocol);
		//ocol = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		//glMaterialfv(GL_FRONT, GL_DIFFUSE, ocol);
	}
}

bool ModelRenderPass::init()
{
	// May as well check that we're going to render the geoset before doing all this crap.
	if (!model || geoIndex == -1 || !model->geosets[geoIndex]->display)
		return false;

	// COLOUR
	// Get the colour and transparency and check that we should even render
	ocol = glm::vec4(1.0f, 1.0f, 1.0f, model->trans);
	ecol = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

	// emissive colors
	if (color != -1 && color < static_cast<int16>(model->colors.size()) && model->colors[color].color.uses(0))
	{
		/* Alfred 2008.10.02 buggy opacity make model invisible, TODO */
		const glm::vec3 c = model->colors[color].color.getValue(0, model->animtime);
		if (model->colors[color].opacity.uses(model->anim))
			ocol.w = model->colors[color].opacity.getValue(model->anim, model->animtime);

		if (unlit)
		{
			ocol.x = c.x;
			ocol.y = c.y;
			ocol.z = c.z;
		}
		else
			ocol.x = ocol.y = ocol.z = 0;

		ecol = glm::vec4(c, ocol.w);
		glMaterialfv(GL_FRONT, GL_EMISSION, glm::value_ptr(ecol));
	}

	// opacity
	if (opacity != -1 &&
		opacity < static_cast<int16>(model->transparency.size()) &&
		model->transparency[opacity].trans.uses(0))
	{
		// Alfred 2008.10.02 buggy opacity make model invisible, TODO
		ocol.w *= model->transparency[opacity].trans.getValue(0, model->animtime);
	}

	// exit and return false before affecting the opengl render state
	if (!((ocol.w > 0) && (color == -1 || ecol.w > 0)))
		return false;

	// TEXTURE
	// bind to our texture
	const GLuint texId = model->getGLTexture(tex);
	if (texId != INVALID_TEX)
		glBindTexture(GL_TEXTURE_2D, texId);

	// ALPHA BLENDING
	// blend mode

	switch (blendmode)
	{
	case BM_OPAQUE: // 0
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case BM_TRANSPARENT: // 1
		glEnable(GL_ALPHA_TEST);
		glBlendFunc(GL_ONE, GL_ZERO);
		break;
	case BM_ALPHA_BLEND: // 2
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case BM_ADDITIVE: // 3
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_COLOR, GL_ONE);
		break;
	case BM_ADDITIVE_ALPHA: // 4
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		break;
	case BM_MODULATE: // 5
		glEnable(GL_BLEND);
		glBlendFunc(GL_DST_COLOR, GL_ZERO);
		break;
	case BM_MODULATEX2: // 6
		glEnable(GL_BLEND);
		glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
		break;
	case BM_7: // 7, new in WoD
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		break;
	default:
		LOG_ERROR << "Unknown blendmode:" << blendmode;
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	if (cull)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);
	// no writing to the depth buffer.
	if (noZWrite)
		glDepthMask(GL_FALSE);
	else
		glDepthMask(GL_TRUE);

	// Texture wrapping around the geometry
	if (swrap)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	if (twrap)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Environmental mapping, material, and effects
	if (useEnvMap)
	{
		// Turn on the 'reflection' shine, using 18.0f as that is what WoW uses based on the reverse engineering
		// This is now set in InitGL(); - no need to call it every render.
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 18.0f);

		// env mapping
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);

		const GLint maptype = GL_SPHERE_MAP;
		//const GLint maptype = GL_REFLECTION_MAP_ARB;

		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, maptype);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, maptype);
	}

	if (texanim != -1 &&
		texanim < static_cast<int16>(model->texAnims.size()))
	{
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();

		model->texAnims[texanim].setup(texanim);
	}

	// color
	glColor4fv(glm::value_ptr(ocol));
	//glMaterialfv(GL_FRONT, GL_SPECULAR, ocol);

	// don't use lighting on the surface
	if (unlit)
		glDisable(GL_LIGHTING);

	if (blendmode <= 1 && ocol.w < 1.0f)
		glEnable(GL_BLEND);

	return true;
}

void ModelRenderPass::render(bool animated)
{
	const ModelGeosetHD* geoset = model->geosets[geoIndex];
	// we don't want to render completely transparent parts
	// render
	if (animated)
	{
		//glDrawElements(GL_TRIANGLES, p.indexCount, GL_UNSIGNED_SHORT, indices + p.indexStart);
		// a GDC OpenGL Performace Tuning paper recommended glDrawRangeElements over glDrawElements
		// I can't notice a difference but I guess it can't hurt
		if (video.supportVBO && video.supportDrawRangeElements)
		{
			glDrawRangeElements(GL_TRIANGLES, geoset->vstart, geoset->vstart + geoset->vcount, geoset->icount,
			                    GL_UNSIGNED_SHORT, &model->indices[geoset->istart]);
		}
		else
		{
			glBegin(GL_TRIANGLES);
			for (size_t k = 0, b = geoset->istart; k < geoset->icount; k++, b++)
			{
				const uint32 a = model->indices[b];
				glNormal3fv(glm::value_ptr(model->normals[a]));
				glTexCoord2fv(glm::value_ptr(model->origVertices[a].texcoords));
				glVertex3fv(glm::value_ptr(model->vertices[a]));
				/*
				if (geoset->id == 2401 && k < 10)
				{
				  LOG_INFO << "b" << b;
				  LOG_INFO << "a" << model->indices[b] << a;
				  LOG_INFO << "model->normals[a]" << model->normals[a].x << model->normals[a].y << model->normals[a].z;
				  LOG_INFO << "model->vertices[a]" << model->vertices[a].x << model->vertices[a].y << model->vertices[a].z;
				}
				*/
			}
			glEnd();
		}
	}
	else
	{
		glBegin(GL_TRIANGLES);
		for (size_t k = 0, b = geoset->istart; k < geoset->icount; k++, b++)
		{
			const uint16 a = model->indices[b];
			glNormal3fv(glm::value_ptr(model->normals[a]));
			glTexCoord2fv(glm::value_ptr(model->origVertices[a].texcoords));
			glVertex3fv(glm::value_ptr(model->vertices[a]));
		}
		glEnd();
	}
}
