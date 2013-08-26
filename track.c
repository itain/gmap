/*
 * track.c
 * Copyright (C) 2007 Itai Nahshon
 *
 */
#include "gmap.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

struct TrackSet *
new_trackset() {
	struct TrackSet *ret;

	ret = (struct TrackSet *)gmap_malloc(sizeof(struct TrackSet));
	ret->count = 0;
	ret->tracks = NULL;
	return ret;
}

struct Track *
new_track(char *name, struct TrackSet *trackset) {
	struct Track *ret;
	trackset->tracks = (struct Track *)gmap_realloc(trackset->tracks, (1 + trackset->count) * sizeof(struct Track));
	ret = &trackset->tracks[trackset->count];
	trackset->count++;
	ret->count = 0;
	ret->tracksegments = NULL;
	ret->color = 0x00000000;
	ret->width = 2;
	ret->dashes = NULL;
	ret->name = gmap_strdup(name);
	ret->number = 0;
	ret->color = g_random_int() & 0x00FFFFFF;
	return ret;
}

struct TrackSeg *
new_trackseg(struct Track *track) {
	struct TrackSeg *ret;
	track->tracksegments = (struct TrackSeg *)gmap_realloc(track->tracksegments, (1 + track->count) * sizeof(struct TrackSeg));
	ret = &track->tracksegments[track->count];
	track->count++;
	ret->count = 0;
	ret->trackpoints = NULL;
	return ret;
}

// static int point_serial = 0;
struct TrackPoint *
new_trackpoint(struct TrackSeg *trackseg) {
	struct TrackPoint *ret;
	trackseg->trackpoints = (struct TrackPoint *)gmap_realloc(trackseg->trackpoints, (1 + trackseg->count) * sizeof(struct TrackPoint));
	ret = &trackseg->trackpoints[trackseg->count];
	trackseg->count++;
	ret->time.tv_sec = ret->time.tv_usec = 0;
	// ret->serial = point_serial++;
	return ret;
}

static xmlChar *
GetName(xmlDocPtr doc, xmlNodePtr cur) {
	cur = cur->xmlChildrenNode;
	while(cur != NULL) {
		if(!xmlStrcmp(cur->name, BAD_CAST "name"))
			return xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		cur = cur->next;
	}
	return NULL;
}

bool
load_from_gpx(char *filename, struct TrackSet **trkset, struct RouteSet **routeset, struct WayPointSet **waypointset) {
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlNodePtr cur;
	xmlChar *lon, *lat, *name, *tmp;


	*trkset = NULL;
	*routeset = NULL;
	*waypointset = NULL;

	doc = xmlReadFile(filename, NULL, XML_PARSE_NOBLANKS|XML_PARSE_NOXINCNODE|XML_PARSE_NONET|XML_PARSE_NOENT);

	if (doc == NULL ) {
		fprintf(stderr,"GPX Document %s not parsed successfully.\n", filename);
		return FALSE;
	}

	root = xmlDocGetRootElement(doc);

	if (root == NULL) {
		fprintf(stderr,"GPX Document %s is empty\n", filename);
		xmlFreeDoc(doc);
		return FALSE;
	}

	if (xmlStrcmp(root->name, BAD_CAST "gpx")) {
		fprintf(stderr,"Document %s is not a GPX file\n", filename);
		xmlFreeDoc(doc);
		return FALSE;
	}

	cur = root->xmlChildrenNode;
	while (cur != NULL) {
		if (!xmlStrcmp(cur->name, BAD_CAST "trk")) {
			xmlNodePtr curtrkseg;
			struct Track *trk;
			struct TrackSeg *trkseg;
			struct TrackPoint *trkpt;
			double dist = 0;
			double	last_lon, last_lat;
			bool first = TRUE;

			if(*trkset == NULL)
				*trkset = new_trackset();

			name = GetName(doc, cur);
			if(name == NULL)
				name = (xmlChar *)gmap_strdup("Anonymous Track");
			trk = new_track((char *)name, *trkset);
			xmlFree(name);

			curtrkseg = cur->xmlChildrenNode;
			while (curtrkseg != NULL) {
				xmlNodePtr curtrkpt;
				if ((xmlStrcmp(curtrkseg->name, BAD_CAST "trkseg"))){
					if(!xmlStrcmp(curtrkseg->name, BAD_CAST "number")) {
						tmp = xmlNodeListGetString(doc, curtrkseg->xmlChildrenNode, 1);
						trk->number = atoi((char *)tmp);
						xmlFree(tmp);
					}
					else if(xmlStrcmp(curtrkseg->name, BAD_CAST "name"))
						fprintf(stderr, "Ignoring trk.%s element\n", curtrkseg->name);
					curtrkseg = curtrkseg->next;
					continue;
				}
				trkseg = new_trackseg(trk);
				curtrkpt = curtrkseg->xmlChildrenNode;
				while(curtrkpt != NULL) {
					xmlNodePtr param;
					if ((xmlStrcmp(curtrkpt->name, BAD_CAST "trkpt"))){
						fprintf(stderr, "Ignoring trk.trkseg.%s element\n", curtrkpt->name);
						curtrkpt = curtrkpt->next;
						continue;
					}
					trkpt = new_trackpoint(trkseg);
					lat = xmlGetProp(curtrkpt, BAD_CAST "lat");
					lon = xmlGetProp(curtrkpt, BAD_CAST "lon");
					trkpt->point.geo_lon = atof((char *)lon);
					trkpt->point.geo_lat = atof((char *)lat);

					if(!first)
						dist += Distance(last_lat, last_lon, trkpt->point.geo_lat, trkpt->point.geo_lon);
					trkpt->distance = dist;
					first = FALSE;
					last_lat = trkpt->point.geo_lat;
					last_lon = trkpt->point.geo_lon;

					param = curtrkpt->xmlChildrenNode;
					while(param != NULL) {
						if(!xmlStrcmp(param->name, BAD_CAST "ele")) {
							tmp = xmlNodeListGetString(doc, param->xmlChildrenNode, 1);
							trkpt->point.elevation = atof((char *)tmp);
							xmlFree(tmp);
						}
						else if(!xmlStrcmp(param->name, BAD_CAST "time")) {
							tmp = xmlNodeListGetString(doc, param->xmlChildrenNode, 1);
							if(!g_time_val_from_iso8601((char *)tmp, &trkpt->time)) {
								g_message("Could not parse iso8601 time string \"%s\"", tmp);
							}
							xmlFree(tmp);
						}
						param = param->next;
					}

					curtrkpt = curtrkpt->next;
				}
				curtrkseg = curtrkseg->next;
			}
		}
		else if (!xmlStrcmp(cur->name, BAD_CAST "wpt")) {
			xmlNodePtr param;
			struct WayPoint *waypt;

			if(*waypointset == NULL)
				*waypointset = new_waypointset();

			name = GetName(doc, cur);
			waypt = new_waypoint((char *)name, *waypointset);
			xmlFree(name);

			lat = xmlGetProp(cur, BAD_CAST "lat");
			lon = xmlGetProp(cur, BAD_CAST "lon");
			waypt->point.geo_lon = atof((char *)lon);
			waypt->point.geo_lat = atof((char *)lat);

			param = cur->xmlChildrenNode;
			while(param != NULL) {
				if(!xmlStrcmp(param->name, BAD_CAST "ele")) {
					tmp = xmlNodeListGetString(doc, param->xmlChildrenNode, 1);
					waypt->point.elevation = atof((char *)tmp);
					xmlFree(tmp);
				}
				else if(!xmlStrcmp(param->name, BAD_CAST "cmt")) {
					tmp = xmlNodeListGetString(doc, param->xmlChildrenNode, 1);
					waypt->comment = gmap_strdup((char *)tmp);
					xmlFree(tmp);
				}
				else if(!xmlStrcmp(param->name, BAD_CAST "desc")) {
					tmp = xmlNodeListGetString(doc, param->xmlChildrenNode, 1);
					waypt->description = gmap_strdup((char *)tmp);
					xmlFree(tmp);
				}
				else if(!xmlStrcmp(param->name, BAD_CAST "sym")) {
					tmp = xmlNodeListGetString(doc, param->xmlChildrenNode, 1);
					waypt->symbol = gmap_strdup((char *)tmp);
					xmlFree(tmp);
				}
				param = param->next;
			}
			if(waypt->symbol == NULL)
				waypt->symbol = gmap_strdup(DEFAULT_WAYPT_SYM);
			waypt->image = get_image_for_symbol(waypt->symbol);
		}
		else if (!xmlStrcmp(cur->name, BAD_CAST "rte")) {
			xmlNodePtr currtept;
			struct Route *route;

			if(*routeset == NULL)
				*routeset = new_routeset();

			name = GetName(doc, cur);
			route = new_route((char *)name, *routeset);
			xmlFree(name);

			currtept = cur->xmlChildrenNode;
			while(currtept != NULL) {
				xmlNodePtr param;
				struct RoutePoint *routepoint;
				
				if ((xmlStrcmp(currtept->name, BAD_CAST "rtept"))){
					if(xmlStrcmp(currtept->name, BAD_CAST "name"))
						fprintf(stderr, "Ignoring rte.%s element\n", currtept->name);
					currtept = currtept->next;
					continue;
				}
				routepoint = new_routepoint(route);

				lat = xmlGetProp(currtept, BAD_CAST "lat");
				lon = xmlGetProp(currtept, BAD_CAST "lon");
				routepoint->point.geo_lon = atof((char *)lon);
				routepoint->point.geo_lat = atof((char *)lat);


				param = currtept->xmlChildrenNode;
				while(param != NULL) {
					param = param->next;
				}
				currtept = currtept->next;
			}
		}
		else {
			fprintf(stderr, "Ignoring %s element\n", cur->name);
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);
	return TRUE;
}
