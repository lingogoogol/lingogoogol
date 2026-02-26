#include "ShieldArmor.h"

ShieldArmor::ShieldArmor() {
	name = "Wooden Shield";
	level_required = 1;
	weight = 4;
	type = 'a';
	defense_increment = 8;
	effects = "Defense+8";
	description = "A basic shield made of wood. Offers minimal protection.";
}
