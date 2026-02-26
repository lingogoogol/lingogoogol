#ifndef OBJECT_H
#define OBJECT_H

#include <glm/glm.hpp>

#include "object_f.h"

class Object {
public:
	Object();
	void render();
private:
	glm::vec3 pos{};
};

#endif