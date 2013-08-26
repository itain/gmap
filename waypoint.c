#include "gmap.h"

struct WayPointSet *
new_waypointset() {
	struct WayPointSet *ret;

	ret = (struct WayPointSet *)gmap_malloc(sizeof(struct WayPointSet));
	ret->count = 0;
	ret->waypoints = NULL;
	return ret;
}

struct WayPoint *
new_waypoint(char *name, struct WayPointSet *waypointset) {
	struct WayPoint *ret;

	waypointset->waypoints = (struct WayPoint *)gmap_realloc(waypointset->waypoints, (1 + waypointset->count) * sizeof(struct WayPoint));
	ret = &waypointset->waypoints[waypointset->count];
	waypointset->count++;

	ret->name = gmap_strdup(name);;
	ret->comment = NULL;
	ret->description = NULL;
	ret->symbol = NULL;
	ret->image = NULL;

	return ret;
}

struct RouteSet *
new_routeset() {
	struct RouteSet *ret;

	ret = (struct RouteSet *)gmap_malloc(sizeof(struct RouteSet));
	ret->count = 0;
	ret->routes = NULL;

	return ret;
}

struct Route *
new_route(char *name, struct RouteSet *routeset) {
	struct Route *ret;

	routeset->routes = (struct Route *)gmap_realloc(routeset->routes, (1 + routeset->count) * sizeof(struct Route));
	ret = &routeset->routes[routeset->count];
	routeset->count++;

	ret->name = gmap_strdup(name);;
	ret->count = 0;
	ret->routepoints = NULL;
	ret->color = 0x00000000;
	ret->width = 2;
	ret->dashes = NULL;
	ret->color = g_random_int() & 0x00FFFFFF;

	return ret;
}

struct RoutePoint *
new_routepoint(struct Route *route) {
	struct RoutePoint *ret;
	route->routepoints = (struct RoutePoint *)gmap_realloc(route->routepoints, (1 + route->count) * sizeof(struct RoutePoint));
	ret = &route->routepoints[route->count];
	route->count++;

	return ret;
}
