#include "ModelLight.h"

#include <glm/gtc/type_ptr.hpp>

#include "wow_enums.h"
#include "logger/Logger.h"

void ModelLight::init(GameFile* f, ModelLightDef& mld, std::vector<uint32>& global)
{
	tpos = pos = mld.pos;
	tdir = dir = glm::vec3(0, 1, 0); // no idea
	type = mld.type;
	parent = mld.bone;
	ambColor.init(mld.ambientColor, f, global);
	ambIntensity.init(mld.ambientIntensity, f, global);
	diffColor.init(mld.diffuseColor, f, global);
	diffIntensity.init(mld.diffuseIntensity, f, global);
	AttenStart.init(mld.attenuationStart, f, global);
	AttenEnd.init(mld.attenuationEnd, f, global);
	UseAttenuation.init(mld.useAttenuation, f, global);
}

void ModelLight::setup(size_t time, GLuint l)
{
	glm::vec4 ambcol(ambColor.getValue(0, time) * ambIntensity.getValue(0, time), 1.0f);
	glm::vec4 diffcol(diffColor.getValue(0, time) * diffIntensity.getValue(0, time), 1.0f);
	glm::vec4 p;
	if (type == MODELLIGHT_DIRECTIONAL)
	{
		// directional
		p = glm::vec4(tdir, 0.0f);
	}
	else if (type == MODELLIGHT_POINT)
	{
		// point
		p = glm::vec4(tpos, 1.0f);
	}
	else
	{
		p = glm::vec4(tpos, 1.0f);
		LOG_ERROR << "Light type" << type << "is unknown.";
	}
	//gLog("Light %d (%f,%f,%f) (%f,%f,%f) [%f,%f,%f]\n", l-GL_LIGHT4, ambcol.x, ambcol.y, ambcol.z, diffcol.x, diffcol.y, diffcol.z, p.x, p.y, p.z);
	glLightfv(l, GL_POSITION, glm::value_ptr(p));
	glLightfv(l, GL_DIFFUSE, glm::value_ptr(diffcol));
	glLightfv(l, GL_AMBIENT, glm::value_ptr(ambcol));
	glEnable(l);
}
