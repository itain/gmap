/*
 * mapset.c
 * Copyright (C) 2007 Itai Nahshon
 */

#include "gmap.h"
#include <libxml/xmlmemory.h>

static xmlAttrPtr
xmlNewIntProp(xmlNodePtr node, const xmlChar *name, int value) {
	char str[256];
	snprintf(str, sizeof(str), "%d", value);
	return xmlNewProp(node, name, BAD_CAST str);
}

static xmlAttrPtr
xmlNewFloatProp(xmlNodePtr node, const xmlChar *name, double value) {
	char str[256];
	snprintf(str, sizeof(str), "%.9f", value);
	return xmlNewProp(node, name, BAD_CAST str);
}

static xmlAttrPtr
xmlNewBoolProp(xmlNodePtr node, const xmlChar *name, bool value) {
	return xmlNewProp(node, name, value ? BAD_CAST "True" : BAD_CAST "False");
}

struct MapSet *
new_mapset() {
	struct MapSet *mapset;
	mapset = (struct MapSet *)gmap_malloc(sizeof(struct MapSet));
	mapset->name = NULL;
	mapset->WKT = NULL;
	mapset->filename = NULL;
	mapset->description = NULL;
	mapset->basedir = NULL;
	mapset->count = 0;
	mapset->maps = NULL;
	mapset->left = -1;
	mapset->right = -1;
	mapset->top = -1;
	mapset->bottom = -1;
	mapset->dirty = TRUE;
	mapset->preferred_scale = 1.0;	/* arbitrary = 1 unit/pixel */
	mapset->max_scale = 0.0;
	mapset->min_scale = 0.0;

	return mapset;
}

void
free_mapset(struct MapSet *mapset) {
	int i;
	gmap_free(mapset->name);
	gmap_free(mapset->description);
	gmap_free(mapset->WKT);
	gmap_free(mapset->filename);
	for(i = 0; i < mapset->count; i++) {
		/* free from cache */
		map_uncache(&mapset->maps[i]);
		gmap_free(mapset->maps[i].filename);
		gmap_free(mapset->maps[i].fullpath);
		gmap_free(mapset->maps[i].calibration_points);
	}
	gmap_free(mapset->maps);
	gmap_free(mapset);
}

struct Map *
new_map(struct MapSet *mapset, const char *filename) {
	struct Map *mp;
	mapset->maps = (struct Map *)gmap_realloc(mapset->maps, (mapset->count+1)*sizeof(struct Map));
	mp = &mapset->maps[mapset->count];
	mp->filename = get_relative_filename(filename, mapset->basedir);
	// mp->filename = gmap_strdup(filename);
	// g_message("rel = '%s'", mp->filename);
	mp->n_calibration_points = 0;
	mp->calibration_points = NULL;
	mp->visible = FALSE;
	mp->cached = NULL;
	mp->cachedDS = NULL;
	mp->mapset = mapset;
	mp->fullpath = NULL;
	mp->width = 0;
	mp->height = 0;
	mp->bpp = 0;
	mp->Rect.x = 0;
	mp->Rect.y = 0;
	mp->Rect.w = 0;
	mp->Rect.h = 0;
	mp->CropPoints = 0;
	mp->Crop = NULL;
	set_unity_geotransform(mp->GeoTransform);

	mapset->count++;
	mapset->dirty = TRUE;

	return mp;
}

static double
xmlGetDoubleProp(xmlNodePtr node, const xmlChar *name, double defvalue) {
	xmlChar *str = xmlGetProp(node, name);
	double ret = defvalue;
	if(str) {
		sscanf((char *)str, "%lf", &ret);
		xmlFree(str);
	}
	return ret;
}

static int
xmlGetIntProp(xmlNodePtr node, const xmlChar *name, int defvalue) {
	xmlChar *str = xmlGetProp(node, name);
	int ret = defvalue;
	if(str) {
		sscanf((char *)str, "%d", &ret);
		xmlFree(str);
	}
	return ret;
}

static bool
xmlGetBoolProp(xmlNodePtr node, const xmlChar *name, bool defvalue) {
	xmlChar *str = xmlGetProp(node, name);
	bool ret = defvalue;
	if(str) {
		ret = !g_ascii_strcasecmp((gchar *)str, "true");
		xmlFree(str);
	}
	return ret;
}

struct MapSet *
mapset_from_doc(xmlDocPtr doc) {
	xmlNodePtr root, cur;
	struct MapSet *mapset;

	root = xmlDocGetRootElement(doc);
	if (root == NULL) {
		/* XXX */
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(doc);
		return NULL;
	}

	if (xmlStrcmp(root->name, BAD_CAST "MapSet")) {
		/* XXX */
		fprintf(stderr,"document of the wrong type, root node != gpx");
		xmlFreeDoc(doc);
		return NULL;
	}

	/* OK, we have a mapset! */
	mapset = new_mapset();
	cur = root->xmlChildrenNode;
	while (cur != NULL) {
		if (!xmlStrcmp(cur->name, BAD_CAST "Name")) {
			xmlChar *str = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			mapset_set_name(mapset, (char *)str);
			xmlFree(str);
		}
		else  if (!xmlStrcmp(cur->name, BAD_CAST "Description")) {
			xmlChar *str = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			mapset_set_description(mapset, (char *)str);
			xmlFree(str);
		}
		else  if (!xmlStrcmp(cur->name, BAD_CAST "Attachments")) {
		}
		else  if (!xmlStrcmp(cur->name, BAD_CAST "BaseDir")) {
			xmlChar *str = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			mapset_set_basedir(mapset, (char *)str);
			xmlFree(str);
		}
		else  if (!xmlStrcmp(cur->name, BAD_CAST "Projection")) {
			xmlChar *str = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			mapset_set_projection(mapset, (char *)str);
			xmlFree(str);
		}
		else  if (!xmlStrcmp(cur->name, BAD_CAST "Bounds")) {
			mapset->left = xmlGetDoubleProp(cur, BAD_CAST "Left", 0.0);
			mapset->right = xmlGetDoubleProp(cur, BAD_CAST "Right", 0.0);
			mapset->top = xmlGetDoubleProp(cur, BAD_CAST "Top", 0.0);
			mapset->bottom = xmlGetDoubleProp(cur, BAD_CAST "Bottom", 0.0);
		}
		else  if (!xmlStrcmp(cur->name, BAD_CAST "Scale")) {
			mapset->preferred_scale = xmlGetDoubleProp(cur, BAD_CAST "Default", 1.0);
			mapset->max_scale = xmlGetDoubleProp(cur, BAD_CAST "Max", 0.0);
			mapset->min_scale = xmlGetDoubleProp(cur, BAD_CAST "Min", 0.0);
		}
		else  if (!xmlStrcmp(cur->name, BAD_CAST "Map")) {
			xmlNodePtr c;
			xmlChar *filename = xmlGetProp(cur, BAD_CAST "Filename");
			struct Map *map = new_map(mapset, (char *)filename);
			xmlFree(filename);

			c = cur->xmlChildrenNode;
			while(c != NULL) {
				if (!xmlStrcmp(c->name, BAD_CAST "ImageSize")) {
					map->width = xmlGetIntProp(c, BAD_CAST "Width", 0);
					map->height = xmlGetIntProp(c, BAD_CAST "Height", 0);
					map->bpp = xmlGetIntProp(c, BAD_CAST "BPP", 0);
				}
				else if (!xmlStrcmp(c->name, BAD_CAST "GeoTransform")) {
					map->GeoTransform[0] = xmlGetDoubleProp(c, BAD_CAST "V0", 0.0);
					map->GeoTransform[1] = xmlGetDoubleProp(c, BAD_CAST "V1", 0.0);
					map->GeoTransform[2] = xmlGetDoubleProp(c, BAD_CAST "V2", 0.0);
					map->GeoTransform[3] = xmlGetDoubleProp(c, BAD_CAST "V3", 0.0);
					map->GeoTransform[4] = xmlGetDoubleProp(c, BAD_CAST "V4", 0.0);
					map->GeoTransform[5] = xmlGetDoubleProp(c, BAD_CAST "V5", 0.0);
				}
				else if (!xmlStrcmp(c->name, BAD_CAST "Calibration")) {
					xmlNodePtr cp = c->xmlChildrenNode;
					while(cp != NULL) {
						if (!xmlStrcmp(cp->name, BAD_CAST "CalibrationPoint")) {
							struct cpoint cpoint;
							cpoint.mapx = xmlGetDoubleProp(cp, BAD_CAST "MapX", 0.0);
							cpoint.mapy = xmlGetDoubleProp(cp, BAD_CAST "MapY", 0.0);
							cpoint.geox = xmlGetDoubleProp(cp, BAD_CAST "GeoX", 0.0);
							cpoint.geoy = xmlGetDoubleProp(cp, BAD_CAST "GeoY", 0.0);
							cpoint.valid = xmlGetBoolProp(cp, BAD_CAST "Valid", TRUE);
							map_add_cpoint(mapset, map, &cpoint);
						}
						cp = cp->next;
					}
				}
				else if (!xmlStrcmp(c->name, BAD_CAST "Rect")) {
					map->Rect.x = xmlGetIntProp(c, BAD_CAST "X", 0);
					map->Rect.y = xmlGetIntProp(c, BAD_CAST "Y", 0);
					map->Rect.w = xmlGetIntProp(c, BAD_CAST "Width", 0);
					map->Rect.h = xmlGetIntProp(c, BAD_CAST "Height", 0);
				}
				else if (!xmlStrcmp(c->name, BAD_CAST "Crop")) {
					map->CropPoints = 0;
					xmlNodePtr cp = c->xmlChildrenNode;
					while(cp != NULL) {
						if (!xmlStrcmp(cp->name, BAD_CAST "Point")) {
							double x, y;
							x = xmlGetIntProp(cp, BAD_CAST "X", 0);
							y = xmlGetIntProp(cp, BAD_CAST "Y", 0);
							map_add_crop_point(mapset, map, map->CropPoints, x, y);
						}
						cp = cp->next;
					}
				}
				c = c->next;
			}
			if(map->Rect.w > 0 && map->Rect.h  > 0) {
				map->visible = TRUE;
				if(map->CropPoints <= 0) {
					map_set_croprect(mapset, map, map->Rect.x, map->Rect.y, map->Rect.w, map->Rect.h);
				}
			}
		}

		cur = cur->next;
	}

	xmlFreeDoc(doc);

	mapset->dirty = FALSE;
	return mapset;
}

struct MapSet *
mapset_from_file(const char *filename) {
	xmlDocPtr doc;
	struct MapSet *mapset;

	doc = xmlReadFile(filename, NULL, XML_PARSE_NOBLANKS|XML_PARSE_NOXINCNODE|XML_PARSE_NONET|XML_PARSE_NOENT);
	if (doc == NULL ) {
		/* XXX */
		fprintf(stderr,"GPX Document %s not parsed successfully.\n", filename);
		return NULL;
	}
	mapset = mapset_from_doc(doc);

	if(mapset->basedir == NULL) {
		char *basedir = g_path_get_dirname (filename);
		mapset_set_basedir(mapset, basedir);
		g_free(basedir);
	}
	mapset->filename = get_absolute_filename(filename);
	mapset->dirty = FALSE;

	return mapset;
}

void
mapset_set_name(struct MapSet *mapset, const char *str) {
	if(mapset->name)
		gmap_free(mapset->name);
	mapset->name = gmap_strdup(str);
	mapset->dirty = TRUE;
}

void
mapset_set_description(struct MapSet *mapset, const char *str) {
	if(mapset->description)
		gmap_free(mapset->description);
	mapset->description = gmap_strdup(str);
	mapset->dirty = TRUE;
}

void
mapset_set_basedir(struct MapSet *mapset, const char *str) {
	if(mapset->basedir)
		gmap_free(mapset->basedir);
	mapset->basedir = gmap_strdup(str);
	mapset->dirty = TRUE;
}

void
mapset_set_projection(struct MapSet *mapset, const char *str) {
	if(mapset->WKT)
		gmap_free(mapset->WKT);
	mapset->WKT = gmap_strdup(str);
	mapset->dirty = TRUE;
}

/* save as XML */
int
mapset_save(struct MapSet *mapset, char *filename) {
	int i;
	xmlNodePtr n1, n2, n3;
	xmlDocPtr doc;
	xmlNodePtr root;

	if(filename != NULL) {
		gmap_free(mapset->filename);
		filename = gmap_strdup(filename);
	}
	else {
		filename = mapset->filename;
	}

	xmlIndentTreeOutput = 1;

	doc = xmlNewDoc(BAD_CAST "1.0");
	root = xmlNewDocNode(doc, NULL, BAD_CAST "MapSet", NULL);
	xmlDocSetRootElement(doc, root);

	n1 = xmlNewChild(root, NULL, BAD_CAST "Name", BAD_CAST mapset->name);
	if(mapset->description != NULL) {
		n1 = xmlNewChild(root, NULL, BAD_CAST "Description", BAD_CAST mapset->description);
	}
	n1 = xmlNewChild(root, NULL, BAD_CAST "Attachments", NULL);
	if(mapset->basedir != NULL) {
		char *basedir = g_path_get_dirname(filename);
		if(strcmp(basedir, mapset->basedir)) {
			n1 = xmlNewChild(root, NULL, BAD_CAST "BaseDir", BAD_CAST mapset->basedir);
		}
		g_free(basedir);
	}
	n1 = xmlNewChild(root, NULL, BAD_CAST "Projection", BAD_CAST mapset->WKT);

	n1 = xmlNewChild(root, NULL, BAD_CAST "Bounds", NULL);
	xmlNewFloatProp(n1, BAD_CAST "Left", mapset->left);
	xmlNewFloatProp(n1, BAD_CAST "Right", mapset->right);
	xmlNewFloatProp(n1, BAD_CAST "Top", mapset->top);
	xmlNewFloatProp(n1, BAD_CAST "Bottom", mapset->bottom);

	n1 = xmlNewChild(root, NULL, BAD_CAST "Scale", NULL);
	xmlNewFloatProp(n1, BAD_CAST "Default", mapset->preferred_scale);
	if(mapset->max_scale != 0.0)
		xmlNewFloatProp(n1, BAD_CAST "Max", mapset->max_scale);
	if(mapset->min_scale != 0.0)
		xmlNewFloatProp(n1, BAD_CAST "Max", mapset->min_scale);


	for(i = 0; i < mapset->count; i++) {

		n1 = xmlNewChild(root, NULL, BAD_CAST "Map", NULL);
		xmlNewProp(n1, BAD_CAST "Filename", BAD_CAST mapset->maps[i].filename);

		n2 = xmlNewChild(n1, NULL, BAD_CAST "ImageSize", NULL);
		xmlNewIntProp(n2, BAD_CAST "Width", mapset->maps[i].width);
		xmlNewIntProp(n2, BAD_CAST "Height", mapset->maps[i].height);
		xmlNewIntProp(n2, BAD_CAST "BPP", mapset->maps[i].bpp);
		
		n2 = xmlNewChild(n1, NULL, BAD_CAST "GeoTransform", NULL);
		xmlNewFloatProp(n2, BAD_CAST "V0", mapset->maps[i].GeoTransform[0]);
		xmlNewFloatProp(n2, BAD_CAST "V1", mapset->maps[i].GeoTransform[1]);
		xmlNewFloatProp(n2, BAD_CAST "V2", mapset->maps[i].GeoTransform[2]);
		xmlNewFloatProp(n2, BAD_CAST "V3", mapset->maps[i].GeoTransform[3]);
		xmlNewFloatProp(n2, BAD_CAST "V4", mapset->maps[i].GeoTransform[4]);
		xmlNewFloatProp(n2, BAD_CAST "V5", mapset->maps[i].GeoTransform[5]);

		if(mapset->maps[i].n_calibration_points > 0) {
			int j;
			n2 = xmlNewChild(n1, NULL, BAD_CAST "Calibration", NULL);
			
			for(j = 0; j < mapset->maps[i].n_calibration_points; j++) {
				n3 = xmlNewChild(n2, NULL, BAD_CAST "CalibrationPoint", NULL);
				xmlNewFloatProp(n3, BAD_CAST "MapX", mapset->maps[i].calibration_points[j].mapx);
				xmlNewFloatProp(n3, BAD_CAST "MapY", mapset->maps[i].calibration_points[j].mapy);
				xmlNewFloatProp(n3, BAD_CAST "GeoX", mapset->maps[i].calibration_points[j].geox);
				xmlNewFloatProp(n3, BAD_CAST "GeoY", mapset->maps[i].calibration_points[j].geoy);
				xmlNewBoolProp(n3, BAD_CAST "Valid", mapset->maps[i].calibration_points[j].valid);
			}
		}

		n2 = xmlNewChild(n1, NULL, BAD_CAST "Rect", NULL);

		xmlNewIntProp(n2, BAD_CAST "X", mapset->maps[i].Rect.x);
		xmlNewIntProp(n2, BAD_CAST "Y", mapset->maps[i].Rect.y);
		xmlNewIntProp(n2, BAD_CAST "Width", mapset->maps[i].Rect.w);
		xmlNewIntProp(n2, BAD_CAST "Height", mapset->maps[i].Rect.h);

		if(mapset->maps[i].CropPoints > 0) {
			int j;
			n2 = xmlNewChild(n1, NULL, BAD_CAST "Crop", NULL);

			for(j = 0; j < mapset->maps[i].CropPoints; j++) {
				n3 = xmlNewChild(n2, NULL, BAD_CAST "Point", NULL);
				xmlNewFloatProp(n3, BAD_CAST "X", mapset->maps[i].Crop[j].X);
				xmlNewFloatProp(n3, BAD_CAST "Y", mapset->maps[i].Crop[j].Y);
			}
		}
	}

	if(xmlSaveFormatFile(filename, doc, TRUE) < 0)
		fprintf(stderr, "Cannot save file %s\n", "out.xml");

	/* After successful save */
	if(mapset->filename)
		gmap_free(mapset->filename);
	mapset->filename = get_absolute_filename(filename);
	mapset->dirty = FALSE;
	return 0;
}

int
mapset_map_reorder(struct MapSet *mapset, int mapnum, int delta) {
	struct Map tmp;
	int i;

	if(mapnum + delta >= mapset->count)
		delta = mapset->count-mapnum-1;
	else if(mapnum + delta < 0)
		delta = -mapnum;

	if(delta == 0)
		return delta;

	tmp = mapset->maps[mapnum];

	if(delta > 0) {
		for(i = mapnum; i < mapnum+delta; i++)
			mapset->maps[i] = mapset->maps[i+1];
	}
	else {
		for(i = mapnum; i > mapnum+delta; i--)
			mapset->maps[i] = mapset->maps[i-1];
	}

	mapset->maps[mapnum+delta] = tmp;
	mapset->dirty = TRUE;

	return delta;
}

int
map_add_cpoint(struct MapSet *mapset, struct Map *map, const struct cpoint *cp) {
	struct cpoint *p;
	p = (struct cpoint *)gmap_realloc(map->calibration_points, (map->n_calibration_points+1) * sizeof(struct cpoint));
	if(p == NULL) {
		/* XXX */
		return map->n_calibration_points;
	}
	p[map->n_calibration_points] = *cp;

	map->calibration_points = p;
	map->n_calibration_points++;

	mapset->dirty = TRUE;

	return -1 + map->n_calibration_points;
}

void
map_add_crop_point(struct MapSet *mapset, struct Map *map, int pos, double x, double y) {
	int i;
	map->CropPoints = map->CropPoints + 1;
	map->Crop = gmap_realloc(map->Crop, map->CropPoints * sizeof(struct Point));
	
	for(i = map->CropPoints; --i > pos;) {
		map->Crop[i].X = map->Crop[i-1].X;
		map->Crop[i].Y = map->Crop[i-1].Y;
	}
	map->Crop[pos].X = x;
	map->Crop[pos].Y = y;
}

void
map_set_croprect(struct MapSet *mapset, struct Map *map, double x, double y, double w, double h) {
	if(map->Crop)
		gmap_free(map->Crop);

	map->CropPoints = 4;
	map->Crop = gmap_malloc(map->CropPoints * sizeof(struct MapPoint));

	map->Crop[0].X = x;
	map->Crop[0].Y = y;
	map->Crop[1].X = x + w;
	map->Crop[1].Y = y;
	map->Crop[2].X = x + w;
	map->Crop[2].Y = y + h;
	map->Crop[3].X = x;
	map->Crop[3].Y = y + h;
}

int
map_remove_cpoint(struct MapSet *mapset, struct Map *map, int which) {
	int i;

	for(i = which; i < map->n_calibration_points-1; i++) {
		map->calibration_points[i] = map->calibration_points[i+1];
	}
	map->n_calibration_points--;

	mapset->dirty = TRUE;

	return map->n_calibration_points;
}

void
mapset_recalculate_bounds(struct MapSet *mapset) {
	int i;
	double left, right, top, bottom;
	double xx, yy;
	bool first = TRUE;

	for(i = 0; i < mapset->count; i++) {
		struct Map *map = &mapset->maps[i];
		if(!map->visible)	/* it's not playing a role */
			continue;

		/* calculate 4 corners */
		xx = map->GeoTransform[0] + map->Rect.x * map->GeoTransform[1] + map->Rect.y * map->GeoTransform[2];
		yy = map->GeoTransform[3] + map->Rect.x * map->GeoTransform[4] + map->Rect.y * map->GeoTransform[5];
		left = right = xx;
		top = bottom = yy;

		xx = map->GeoTransform[0] + (map->Rect.x + map->Rect.w) * map->GeoTransform[1] + map->Rect.y * map->GeoTransform[2];
		yy = map->GeoTransform[3] + (map->Rect.x + map->Rect.w) * map->GeoTransform[4] + map->Rect.y * map->GeoTransform[5];
		left = MIN(left, xx);
		right = MAX(right, xx);
		top = MAX(top, yy);
		bottom = MIN(bottom, yy);

		xx = map->GeoTransform[0] + map->Rect.x * map->GeoTransform[1] + (map->Rect.y + map->Rect.h)  * map->GeoTransform[2];
		yy = map->GeoTransform[3] + map->Rect.x * map->GeoTransform[4] + (map->Rect.y + map->Rect.h) * map->GeoTransform[5];
		left = MIN(left, xx);
		right = MAX(right, xx);
		top = MAX(top, yy);
		bottom = MIN(bottom, yy);

		xx = map->GeoTransform[0] + (map->Rect.x + map->Rect.w) * map->GeoTransform[1] + (map->Rect.y + map->Rect.h)  * map->GeoTransform[2];
		yy = map->GeoTransform[3] + (map->Rect.x + map->Rect.w) * map->GeoTransform[4] + (map->Rect.y + map->Rect.h) * map->GeoTransform[5];
		left = MIN(left, xx);
		right = MAX(right, xx);
		top = MAX(top, yy);
		bottom = MIN(bottom, yy);

		if(first) {
			mapset->left = left;
			mapset->right = right;
			mapset->top = top;
			mapset->bottom = bottom;
			first = FALSE;
		}
		else {
			mapset->left = MIN(mapset->left,left);
			mapset->right = MAX(mapset->right, right);
			mapset->top = MAX(mapset->top, top);
			mapset->bottom = MIN(mapset->bottom, bottom);
		}
	}
}

void
mapset_recalculate_preferred_scale(struct MapSet *mapset) {
	int i;
	double scale = -1;
	double f;
	for(i = 0; i < mapset->count; i++) {
		struct Map *map = &mapset->maps[i];
		if(!map->visible)	/* it's not playing a role */
			continue;
		
		f = hypot(map->GeoTransform[1], map->GeoTransform[2]);
		if(scale < 0 || scale > f)
			scale = f;
		f = hypot(map->GeoTransform[4], map->GeoTransform[5]);
		if(scale < 0 || scale > f)
			scale = f;
	}
	mapset->preferred_scale = scale;
}

#if 0
void
test1(struct MapSet *mapset) {
	int i, j;

	for(i = 0; i < mapset->count; i++) {

		struct Map *map = &mapset->maps[i];

		printf("%s\n", map->filename);

		// map_calc_GeoReference(map);
		map_calc_GeoReference_error(map);

		for(j = 0; j < map->n_calibration_points; j++) {
			printf("\t%f ; %f\n", map->calibration_points[j].errx, map->calibration_points[j].erry);
		}
	}
}
#endif
