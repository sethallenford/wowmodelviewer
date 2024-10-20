#include "liquid.h"
#include "Game.h"
#include "GameFile.h"
#include "shaders.h"
#include "video.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

struct LiquidVertex
{
	//unsigned char c[4];
	unsigned short w1, w2;
	float h;
};

void Liquid::initFromTerrain(GameFile& f, int flags)
{
	texRepeats = 4.0f;
	/*
	flags:
	8 - ocean
	4 - river
	16 - magma
	*/
	ydir = 1.0f;
	if (flags & 16)
	{
		// magma:
		initTextures(wxT("XTextures\\lava\\lava"), 1, 30);
		type = 0; // not colored
	}
	else if (flags & 4)
	{
		// river/lake
		initTextures(wxT("XTextures\\river\\lake_a"), 1, 30); // TODO: rivers etc.?
		type = 2; // dynamic colors
		shader = 0;
	}
	else
	{
		// ocean
		initTextures(wxT("XTextures\\ocean\\ocean_h"), 1, 30);
		/*
		type = 1; // static color
		col = glm::vec3(0.0f, 0.1f, 0.4f); // TODO: figure out real ocean colors?
		*/
		type = 2;
		shader = 0;
	}
	initGeometry(f);
	trans = false;
}

void Liquid::initFromWMO(GameFile& f, WMOMaterial& mat, bool indoor)
{
	texRepeats = 4.0f;
	ydir = -1.0f;

	initGeometry(f);

	trans = false;

	// tmpflag is the flags value for the last drawn tile
	if (tmpflag & 1)
	{
		initTextures(wxT("XTextures\\slime\\slime"), 1, 30);
		type = 0;
		texRepeats = 2.0f;
	}
	else if (tmpflag & 2)
	{
		initTextures(wxT("XTextures\\lava\\lava"), 1, 30);
		type = 0;
	}
	else
	{
		initTextures(wxT("XTextures\\river\\lake_a"), 1, 30);
		if (indoor)
		{
			trans = true;
			type = 1;
			col = glm::vec3(((mat.color2 & 0xFF0000) >> 16) / 255.0f, ((mat.color2 & 0xFF00) >> 8) / 255.0f,
			                (mat.color2 & 0xFF) / 255.0f);
			shader = 0;
		}
		else
		{
			type = 2; // outdoor water (...?)
			shader = 0;
		}
	}

	/*
	// HACK: this is just...wrong
	// TODO: figure out proper way to identify liquid types
	const char *texname = video.textures.items[mat.tex]->name.c_str();
	char *pos = strstr(texname, "Slime");
	if (pos!=0) {
	  // slime
	  initTextures("XTextures\\slime\\slime", 1, 30);
	  type = 0;
	  texRepeats = 4.0f;
	} else {
	  if (mat.transparent == 1) {
	    // lava?
	    initTextures("XTextures\\lava\\lava", 1, 30);
	    type = 0;
	  } else {
	    // water?
	    initTextures("XTextures\\river\\lake_a", 1, 30);
	    type = 1;
	    col = glm::vec3( ((mat.col2&0xFF0000)>>16)/255.0f, ((mat.col2&0xFF00)>>8)/255.0f, (mat.col2&0xFF)/255.0f);
	  }
	}
	*/
}

void Liquid::initGeometry(GameFile& f)
{
	// assume: f is at the appropriate starting position

	const LiquidVertex* map = reinterpret_cast<LiquidVertex*>(f.getPointer());
	const unsigned char* flags = (unsigned char*)(f.getPointer() + (xtiles + 1) * (ytiles + 1) * sizeof(LiquidVertex));

	// generate vertices
	glm::vec3* verts = new glm::vec3[(xtiles + 1) * (ytiles + 1)];
	float* Col = new float[(xtiles + 1) * (ytiles + 1)];

	for (int j = 0; j < ytiles + 1; j++)
	{
		for (int i = 0; i < xtiles + 1; i++)
		{
			const int p = j * (xtiles + 1) + i;
			float h = map[p].h;
			if (h > 100000) h = pos.y;
			verts[p] = glm::vec3(pos.x + tilesize * i, h, pos.z + ydir * tilesize * j);
			Col[p] = (map[p].w1 / 255.0f) * 0.5f + 0.5f;
		}
	}

	dlist = glGenLists(1);
	glNewList(dlist, GL_COMPILE);

	// TODO: handle light/dark liquid colors
	glNormal3f(0, 1, 0);
	glBegin(GL_QUADS);
	// draw tiles
	for (ssize_t j = 0; j < ytiles; j++)
	{
		for (int i = 0; i < xtiles; i++)
		{
			const unsigned char F = flags[j * xtiles + i];
			if ((F & 8) == 0)
			{
				tmpflag = F;
				// 15 seems to be "don't draw"
				const int p = j * (xtiles + 1) + i;

				// HACK: pack the vertex color selection into the 2nd texture coordinate
				//float fc;

				glTexCoord3f(i / texRepeats, j / texRepeats, Col[p]);
				glVertex3fv(glm::value_ptr(verts[p]));

				glTexCoord3f((i + 1) / texRepeats, j / texRepeats, Col[p + 1]);
				glVertex3fv(glm::value_ptr(verts[p + 1]));

				glTexCoord3f((i + 1) / texRepeats, (j + 1) / texRepeats, Col[p + xtiles + 1 + 1]);
				glVertex3fv(glm::value_ptr(verts[p + xtiles + 1 + 1]));

				glTexCoord3f(i / texRepeats, (j + 1) / texRepeats, Col[p + xtiles + 1]);
				glVertex3fv(glm::value_ptr(verts[p + xtiles + 1]));
			}
		}
	}
	glEnd();

	/*
	// debug triangles:
	//glColor4f(1,1,1,1);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glBegin(GL_TRIANGLES);
	for (ssize_t j=0; j<ytiles+1; j++) {
	  for (size_t i=0; i<xtiles+1; i++) {
	    size_t p = j*(xtiles+1)+i;
	    glm::vec3 v = verts[p];
	    //short s = *( (short*) (f.getPointer() + p*8) );
	    //float f = s / 255.0f;
	    //glColor4f(f,(1.0f-f),0,1);
	    unsigned char c[4];
	    c[0] = 255-map[p].c[3];
	    c[1] = 255-map[p].c[2];
	    c[2] = 255-map[p].c[1];
	    c[3] = map[p].c[0];
	    glColor4ubv(c);
  
	    glVertex3fv(v + glm::vec3(-0.5f, 1.0f, 0));
	    glVertex3fv(v + glm::vec3(0.5f, 1.0f, 0));
	    glVertex3fv(v + glm::vec3(0.0f, 2.0f, 0));
	  }
	}
	glEnd();
	glColor4f(1,1,1,1);
	glEnable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	*/


	/*
	// temp: draw outlines
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINE_LOOP);
	glm::vec3 wx = glm::vec3(tilesize*xtiles,0,0);
	glm::vec3 wy = glm::vec3(0,0,tilesize*ytiles*ydir);
	glColor4f(1,0,0,1);
	glVertex3fv(pos);
	glColor4f(1,1,1,1);
	glVertex3fv(pos+wx);
	glVertex3fv(pos+wx+wy);
	glVertex3fv(pos+wy);
	glEnd();
	glEnable(GL_TEXTURE_2D);
	*/

	glEndList();
	delete[] verts;
	delete[] Col;

	/*
	// only log .wmo
	//if (ydir > 0) return;
	// LOGGING: debug info
	std::string slq;
	char buf[32];
	for (ssize_t j=0; j<ytiles+1; j++) {
	  slq = "";
	  for (size_t i=0; i<xtiles+1; i++) {
	    //short ival[2];
	    unsigned int ival;
	    float fval;
	    f.read(&ival, 4);
	    f.read(&fval, 4);
	    //sprintf(buf, "%4d,%4d ", ival[0],ival[1]);slq.append(buf);
	    sprintf(buf, "%08x ", ival);slq.append(buf);
	  }
	  gLog("%s\n", slq.c_str());
	}
	slq = "";
	for (size_t i=0; i<ytiles*xtiles; i++) {
	  unsigned char bval;
	  f.read(&bval,1);
	  if (bval==15) {
	          sprintf(buf,"    ");      
	  } else sprintf(buf, "%3d ", bval);
	  slq.append(buf);
	  if ( ((i+1)%xtiles) == 0 ) slq.append("\n");
	}
	gLog("%s",slq.c_str());
	*/
}

void Liquid::draw()
{
	return;

	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
	//size_t texidx = (size_t)(gWorld->animtime / 60.0f) % textures.size();
	size_t texidx = 0;
	glBindTexture(GL_TEXTURE_2D, textures[texidx]);

	const float tcol = trans ? 0.9f : 1.0f;
	if (trans)
	{
		glEnable(GL_BLEND);
		glDepthMask(GL_FALSE);
	}

	if (video.supportShaders && (shader >= 0))
	{
		// SHADER-BASED
		glm::vec3 col2;
		waterShaders[shader]->bind();
		if (type == 2)
		{
			//col = gWorld->skies->colorSet[WATER_COLOR_LIGHT];
			//col2 = gWorld->skies->colorSet[WATER_COLOR_DARK];
			col2 = col;
		}
		else
		{
			col2 = col;
		}

		glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, col.x, col.y, col.z, tcol);
		glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1, col2.x, col2.y, col2.z, tcol);

		glCallList(dlist);
		waterShaders[shader]->unbind();
	}
	else
	{
		// FIXED-FUNCTION

		if (type == 0) glColor4f(1, 1, 1, tcol);
		else
		{
			if (type == 2)
			{
				// dynamic color lookup! ^_^
				//col = gWorld->skies->colorSet[WATER_COLOR_LIGHT]; // TODO: add variable water color
			}
			glColor4f(col.x, col.y, col.z, tcol);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
			// TODO: check if ARB_texture_env_add is supported? :(
		}
		glCallList(dlist);

		if (type != 0) glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}

	glDepthFunc(GL_LEQUAL);
	glColor4f(1, 1, 1, 1);
	if (trans)
	{
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}
}

void Liquid::initTextures(wxString basename, int first, int last)
{
	wxString buf;
	for (ssize_t i = first; i <= last; i++)
	{
		buf = wxString::Format(wxT("%s.%d.blp"), basename.c_str(), i);
		const int tex = TEXTUREMANAGER.add(GAMEDIRECTORY.getFile(QString::fromWCharArray(buf.c_str())));
		textures.push_back(tex);
	}
}

Liquid::~Liquid()
{
	for (const unsigned int texture : textures)
	{
		TEXTUREMANAGER.del(texture);
	}
}
