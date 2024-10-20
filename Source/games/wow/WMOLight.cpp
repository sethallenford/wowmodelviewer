#include "WMOLight.h"
#include <glm/gtc/type_ptr.hpp>
#include "GameFile.h"
#include "GL/glew.h"

void WMOLight::setup(GLint light)
{
	// not used right now -_-

	const GLfloat LightAmbient[] = {0, 0, 0, 1.0f};
	const GLfloat LightPosition[] = {pos.x, pos.y, pos.z, 0.0f};

	glLightfv(light, GL_AMBIENT, LightAmbient);
	glLightfv(light, GL_DIFFUSE, glm::value_ptr(fcolor));
	glLightfv(light, GL_POSITION, LightPosition);

	glEnable(light);
}

void WMOLight::setupOnce(GLint light, glm::vec3 dir, glm::vec3 lcol)
{
	glm::vec4 position(dir, 0);
	//glm::vec4 position(0,1,0,0);

	glm::vec4 ambient = glm::vec4(lcol * 0.3f, 1);
	//glm::vec4 ambient = glm::vec4(0.101961f, 0.062776f, 0, 1);
	glm::vec4 diffuse = glm::vec4(lcol, 1);
	//glm::vec4 diffuse = glm::vec4(0.439216f, 0.266667f, 0, 1);

	glLightfv(light, GL_AMBIENT, glm::value_ptr(ambient));
	glLightfv(light, GL_DIFFUSE, glm::value_ptr(diffuse));
	glLightfv(light, GL_POSITION, glm::value_ptr(position));

	glEnable(light);
}

void WMOLight::init(GameFile& f)
{
	/*
	I haven't quite figured out how WoW actually does lighting, as it seems much smoother than
	the regular vertex lighting in my screenshots. The light paramters might be range or attenuation
	information, or something else entirely. Some WMO groups reference a lot of lights at once.
	The WoW client (at least on my system) uses only one light, which is always directional.
	Attenuation is always (0, 0.7, 0.03). So I suppose for models/doodads (both are M2 files anyway)
	it selects an appropriate light to turn on. Global light is handled similarly. Some WMO textures
	(BLP files) have specular maps in the alpha channel, the pixel shader renderpath uses these.
	Still don't know how to determine direction/color for either the outdoor light or WMO local lights... :)
	*/

	char pad;
	f.read(&lighttype, 1); // LightType
	f.read(&type, 1);
	f.read(&useatten, 1);
	f.read(&pad, 1);
	f.read(&color, 4);
	f.read(&pos, 12);
	f.read(&intensity, 4);
	f.read(&attenStart, 4);
	f.read(&attenEnd, 4);
	f.read(unk, 4 * 3);
	// Seems to be -1, -0.5, X, where X changes from model to model. Guard Tower: 2.3611112, GoldshireInn: 5.8888888, Duskwood_TownHall: 5
	f.read(&r, 4);

	pos = glm::vec3(pos.x, pos.z, -pos.y);

	// rgb? bgr? hm
	const float fa = ((color & 0xff000000) >> 24) / 255.0f;
	const float fr = ((color & 0x00ff0000) >> 16) / 255.0f;
	const float fg = ((color & 0x0000ff00) >> 8) / 255.0f;
	const float fb = ((color & 0x000000ff)) / 255.0f;

	fcolor = glm::vec4(fr, fg, fb, fa);
	fcolor *= intensity;
	fcolor.w = 1.0f;

	/*
	// light logging
	gLog("Light %08x @ (%4.2f,%4.2f,%4.2f)\t %4.2f, %4.2f, %4.2f, %4.2f, %4.2f, %4.2f, %4.2f\t(%d,%d,%d,%d)\n",
	color, pos.x, pos.y, pos.z, intensity,
	unk[0], unk[1], unk[2], unk[3], unk[4], r,
	type[0], type[1], type[2], type[3]);
	*/
}
