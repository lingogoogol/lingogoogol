#include <glad/glad.h>

#include "tile.h"

Tile::Tile() :light{} {
	glGenTextures(1, &tex);
	return;
}