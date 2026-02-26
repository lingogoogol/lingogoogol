#include "AxeWeapon.h"

AxeWeapon::AxeWeapon() {
	name = "Battle Axe";
	level_required = 3;
	weight = 10;
	type = 'w';
	attack_increment = 20;
	effects = "Attack+20, heavy swing";
	description = "A heavy axe designed to crush enemies with raw power.";
}
