#include "gmap.h"

struct Symbol {
	char *sym;
	char *filename;
	void *image;
} symbols[] = {
	{ "*", "Smiley.png", },
	{ "Airport", "Airport.png", },
	{ "Bike Trail", "RedBike.png", },
	{ "Bridge", "RedCircle.png", },
	{ "Car", "BlueCar.png", },
	{ "City (Medium)", "BlueBox.png", },
	{ "City (Small)", "NoFire.png", },
	{ "Crossing", "Crossing.png", },
	{ "Diamond, Green", "GreenBox.png", },
	{ "Diamond, Red", "RedBox.png", },
	{ "Flag, Blue", "BlueFlag.png", },
	{ "Flag, Green", "GreenFlag.png", },
	{ "Flag, Red", "RedFlag.png", },
	{ "Navaid, Black", "WhiteFlag.png", },
	{ "Pin, Blue", "GreenBall.png", },
	{ "Pin, Red", "RedBall.png", },
	{ "Radio Beacon", "Fire.png", },
	{ "Residence", "Tree.png", },
	{ "Skull and Crossbones", "Danger.png" },
	{ "Summit", "Clouds.png", },
	{ "Wrecker", "Anchor.png", },
};

#define IMAGES_BASE "images/32X32"

void *
get_image_for_symbol(char *sym) {
	struct Symbol *s = &symbols[0];
	int i;

	if(*sym) {
		for(i = 1; symbols[i].sym; i++) {
			if(!strcmp(sym, symbols[i].sym))
				s = &symbols[i];
		}
	}
	if(s->image == NULL) {
		char *tmp = g_build_filename(IMAGES_BASE, s->filename, NULL);
		s->image = cairo_image_surface_create_from_png(tmp);
		g_free(tmp);
	}
	return s->image;
}
