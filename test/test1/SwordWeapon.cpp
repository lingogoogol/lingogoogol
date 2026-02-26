#include "SwordWeapon.h"

SwordWeapon::SwordWeapon() {
	name = "Iron Sword";
	level_required = 1;
	weight = 5;
	type = 'w';
	attack_increment = 10;
	effects = "Attack+10";
	description = "A simple iron sword forged for beginners.";
}
