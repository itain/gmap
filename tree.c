/*
 * tree.c
 * Copyright (C) 2007 Itai Nahshon
 *
 */
#include "gmap.h"

#define MIN_SIZE 10

typedef enum {
	PARTITION_NONE,
	PARTITION_RL,
	PARTITION_TB,
} TreeNodePartition;

typedef enum {
	SEGMENT,
	WAYPOINT,
} TreeObjectType;

struct TreeObject {
	TreeObjectType type;
	union {
		struct {
			struct Track *trk;
			int x0, y0, x1, y1;
			struct TrackPoint *p1, *p2;
		} seg;
		struct {
			struct WayPoint *wpt;
			int x, y, w, h;
		} wpt;
	} u;
};

struct  TreeSearchResult {
	struct TreeObject *obj;
	double dist;
	double u;
	struct TrackPoint *p1, *p2;
};

struct TreeNode {
	int top, bottom, left, right;
	TreeNodePartition partition;
	int center;
	int count;
	struct TreeObject *objects;
	struct TreeNode *p1, *p2;
};

void
free_tree(struct TreeNode *t) {
	if(t == NULL)
		return;

	free_tree(t->p1);
	free_tree(t->p2);

	gmap_free(t->objects);
	gmap_free(t);
}

struct TreeNode *
new_branch(int top, int bottom, int left, int right) {
	struct TreeNode *t;

	t = gmap_malloc(sizeof(struct TreeNode));
	t->left = left;
	t->right = right;
	t->top = top;
	t->bottom = bottom;
	if((bottom - top) < MIN_SIZE && (right - left) < MIN_SIZE) {
		t->partition = PARTITION_NONE;
	}
	else if((bottom - top) > (right - left)) {
		t->partition = PARTITION_TB;
		t->center = (t->top + t->bottom) / 2;
	}
	else {
		t->partition = PARTITION_RL;
		t->center = (t->left + t->right) / 2;
	}
	t->count = 0;
	t->objects = NULL;
	t->p1 = NULL;
	t->p2 = NULL;

	return t;
}

static void
tree_node_add_object(struct TreeNode *t, struct TreeObject *o) {
	// g_message("tree_node_add_segment: %d %d %d %d", x0, y0, x1, y1);
	t->objects = (struct TreeObject *)gmap_realloc(t->objects, (1 + t->count) * sizeof(struct TreeObject));
	t->objects[t->count] = *o;

	t->count++;
}

static void
tree_add_object(struct TreeNode *t, struct TreeObject *o, int left, int right, int top, int bottom) {
	for(;;) {
		switch(t->partition) {
		case PARTITION_RL:
			if(right < t->center) {
				if(t->p1 == NULL)
					t->p1 = new_branch(t->top, t->bottom, t->left, t->center);
				t = t->p1;
				continue;
			}
			if(left > t->center) {
				if(t->p2 == NULL)
					t->p2 = new_branch(t->top, t->bottom, t->center, t->right);
				t = t->p2;
				continue;
			}
			tree_node_add_object(t, o);
			return;
		case PARTITION_TB:
			if(bottom < t->center) {
				if(t->p1 == NULL)
					t->p1 = new_branch(t->top, t->center, t->left, t->right);
				t = t->p1;
				continue;
			}
			if(top > t->center) {
				if(t->p2 == NULL)
					t->p2 = new_branch(t->center, t->bottom, t->left, t->right);
				t = t->p2;
				continue;
			}
			tree_node_add_object(t, o);
			return;
		case PARTITION_NONE:
			tree_node_add_object(t, o);
			return;
		}
	}
}

void
tree_add_segment(struct TreeNode *t, struct Track *trk, int x0, int y0, int x1, int y1, struct TrackPoint *p1, struct TrackPoint *p2) {
	struct TreeObject obj;
	obj.type = SEGMENT;
	obj.u.seg.trk = trk;
	obj.u.seg.x0 = x0;
	obj.u.seg.y0 = y0;
	obj.u.seg.x1 = x1;
	obj.u.seg.y1 = y1;
	obj.u.seg.p1 = p1;
	obj.u.seg.p2 = p2;
	tree_add_object(t, &obj, MIN(x0, x1), MAX(x0, x1), MIN(y0, y1), MAX(y0, y1));
}

void
tree_add_waypoint(struct TreeNode *t, struct WayPoint *wpt, int x, int y) {
	struct TreeObject obj;

	obj.type = WAYPOINT;
	if(wpt->image) {
		obj.u.wpt.w = cairo_image_surface_get_width(wpt->image);
		obj.u.wpt.h = cairo_image_surface_get_height(wpt->image);
	}
	else {
		obj.u.wpt.w = 10;
		obj.u.wpt.h = 10;
	}
	obj.u.wpt.x = x-obj.u.wpt.w/2;
	obj.u.wpt.y = y-obj.u.wpt.h/2;
	obj.u.wpt.wpt = wpt;
	//g_message("tree_add_waypoint: %d, %d, %d, %d", obj.u.wpt.x, obj.u.wpt.x+obj.u.wpt.w, obj.u.wpt.y, obj.u.wpt.y+obj.u.wpt.h);
	tree_add_object(t, &obj, obj.u.wpt.x, obj.u.wpt.x+obj.u.wpt.w, obj.u.wpt.y, obj.u.wpt.y+obj.u.wpt.h);
}

struct tree_to_pixmap_s {
	int count;
	double length;
	int rects;
};

static void
tree_to_pixmap_r(struct TreeNode *t, cairo_t *ct, int x, int y, int w, int h, struct tree_to_pixmap_s *s) {
	int i;
	COLOR_T color = -1;

	if(t == NULL)
		return;

	if(t->right < x)
		return;
	if(t->left > x+w)
		return;
	if(t->bottom < y)
		return;
	if(t->top > y+h)
		return;

	//s->rects++;

	for(i = 0; i < t->count; i++) {
		struct TreeObject *obj = &t->objects[i];

		switch(obj->type) {
		case SEGMENT:
			/* Trivial elimination */
			if(obj->u.seg.x0 < x && obj->u.seg.x1 < x) continue;
			if(obj->u.seg.y0 < y && obj->u.seg.y1 < y) continue;
			if(obj->u.seg.x0 >= x+w && obj->u.seg.x1 >= x+w) continue;
			if(obj->u.seg.y0 >= y+h && obj->u.seg.y1 >= y+h) continue;

			if(color != obj->u.seg.trk->color) {
				double r, g, b;
				color = obj->u.seg.trk->color;
				r = ((color >> 16) & 0xff) / 255.;
				g = ((color >> 8) & 0xff) / 255.;
				b = ((color >> 0) & 0xff) / 255.;
				cairo_set_source_rgb(ct, r, g, b);
			}
			cairo_move_to(ct, (double)(obj->u.seg.x0-x), (double)(obj->u.seg.y0-y));
			cairo_line_to(ct, (double)(obj->u.seg.x1-x), (double)(obj->u.seg.y1-y));
			cairo_stroke(ct);
			//g_message("tree_to_pixmap: %d %d %d %d", t->segments[i].x0, t->segments[i].y0, t->segments[i].x1, t->segments[i].y1);
			//s->count++;
			//s->length += linelength(t->segments[i].x1-t->segments[i].x0, t->segments[i].y1-t->segments[i].y0);
			break;
		case WAYPOINT:
			cairo_set_source_surface(ct, (cairo_surface_t *)obj->u.wpt.wpt->image, obj->u.wpt.x-x, obj->u.wpt.y-y);
			cairo_paint(ct);
			if(obj->u.wpt.wpt->name != NULL) {
				//g_message("text: %s", obj->u.wpt.name);
				cairo_set_source_rgb(ct, 0, 0, 0);
				cairo_select_font_face(ct, "arial", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
				cairo_set_font_size(ct, 14);
				cairo_move_to(ct, obj->u.wpt.x-x, obj->u.wpt.y-y);
				cairo_show_text(ct, obj->u.wpt.wpt->name);
			}
			break;
		default:
			g_message("tree_to_pixmap_r: cannot draw an object of type %d", obj->type);
			break;
		}
	}

	tree_to_pixmap_r(t->p1, ct, x, y, w, h, s);
	tree_to_pixmap_r(t->p2, ct, x, y, w, h, s);
}

void
tree_to_pixmap(struct TreeNode *t, struct RenderTarget *target, cairo_t *ct, int x, int y, int w, int h) {
	struct tree_to_pixmap_s s;
	s.count = 0;
	s.rects = 0;
	s.length = 0;
	cairo_set_line_width(ct, 7.0);
	cairo_set_antialias(ct, CAIRO_ANTIALIAS_NONE);
	tree_to_pixmap_r(t, ct, x, y, w, h, &s);
	//g_message("Called to render %d segments in %d rects %g length\n", s.count, s.rects, s.length);
}

static double
point0_to_segment_dist(double x0, double y0, double x1, double y1, double *u) {
	double dx = x1-x0;
	double dy = y1-y0;
	double yy0 = x0*dx + y0*dy;
	double yy1 = x1*dx + y1*dy;

	if((yy0 >= 0) !=  (yy1 >= 0)) {
		*u = yy0 / (yy0 - yy1);
		return fabs(x0*y1-x1*y0)/hypot(dx, dy);
	}
	else if(fabs(yy0) < fabs(yy1)) {
		*u = 0;
		return hypot(x0, y0);
	}
	else {
		*u = 1;
		return hypot(x1, y1);
	}
}

void
find_objects_xy(struct TreeNode *t, int x, int y, int n, int maxdist, struct TreeSearchResult *res) {
	int i, j, k;
	double dist, u, dx, dy;

	if(t == NULL || t->left > x+maxdist || t->right < x-maxdist || t->top > y+maxdist || t->bottom < y-maxdist)
		return;

	for(i = 0; i < t->count; i++) {
		struct TreeObject *obj = &t->objects[i];

		/* for each relevant object calculate it's distance  from point */
		switch(obj->type) {
		case WAYPOINT:
			dx = x - obj->u.wpt.x;
			if(dx > 0) {
				if(dx <= obj->u.wpt.w) dx = 0;
				else dx -= obj->u.wpt.w;
			}
			dy = y - obj->u.wpt.y;
			if(dy > 0) {
				if(dy <= obj->u.wpt.h) dy = 0;
				else dy -= obj->u.wpt.h;
			}
			dist = hypot(dx, dy);
			u = -1;
			break;
		case SEGMENT:
			dist = point0_to_segment_dist(obj->u.seg.x0-x, obj->u.seg.y0-y, obj->u.seg.x1-x, obj->u.seg.y1-y, &u);
			break;
		default:
			continue;
		}

		/* ignore nodes that are too far from the point */
		if(dist > maxdist)
			continue;

		/* Find the right place to insert this object */
		for(j = 0; j < n && res[j].obj != NULL; j++) {

			/* If it's a segment of the same track, unite it with near-segments */
			if(obj->type == SEGMENT && res[j].obj->type == SEGMENT &&
			  res[j].obj->u.seg.trk == obj->u.seg.trk) {
				if(res[j].p2 == obj->u.seg.p1) {
					// g_message("share on p %d. new %d..%d", res[j].p2->serial, res[j].p1->serial, obj->u.seg.p2->serial);
					res[j].p2 = obj->u.seg.p2;

					goto update_existing;
				}
				else if(res[j].p1 == obj->u.seg.p2) {
					// g_message("share on p %d. new %d..%d", res[j].p1->serial, obj->u.seg.p1->serial, res[j].p2->serial);
					res[j].p1 = obj->u.seg.p1;
					goto update_existing;
				}
				else
					goto insert_new;
			}

		insert_new:
			if(dist < res[j].dist) {
				// g_message("insert_new");
				for(k = n-1; k > j; k--) {
					res[k] = res[k-1];
				}
				break;
			}
			else
				continue;

		update_existing:
			// g_message("update_existing");
			if(res[j].dist > dist) {
				res[j].obj = obj;
				res[j].dist = dist;
				res[j].u = u;
			}
			obj = NULL;
			break;
		}
		if(obj && j < n) {
			res[j].dist = dist;
			res[j].obj = obj;
			res[j].u = u;
			if(obj->type == SEGMENT) {
				res[j].p1 = obj->u.seg.p1;
				res[j].p2 = obj->u.seg.p2;
				// g_message("inserted at %d %d..%d", j, res[j].p1->serial, res[j].p2->serial);
			}
		}
		if(j < n && res[j].obj->type == SEGMENT) {
			int j1;
			for(j1 = j+1; j1 < n && res[j1].obj != NULL; j1++) {
				while(res[j1].obj != NULL) {
					if(res[j].obj->type != SEGMENT ||
					   res[j1].obj->u.seg.trk != res[j].obj->u.seg.trk)
						break;
					if(res[j].p2 == res[j1].p1)
						res[j].p2 = res[j1].p2;
					else if(res[j].p1 == res[j1].p2)
						res[j].p1 = res[j1].p1;
					else
						break;
					// g_message("delete point %d: %d..%d", j1, res[j1].p1->serial, res[j1].p2->serial);
					for(k = j1; k < n-1; k++) {
						res[k] = res[k+1];
						if(res[k].obj == NULL)
							break;
					}
				}
			}
		}
	}
	find_objects_xy(t->p1, x, y, n, maxdist, res);
	find_objects_xy(t->p2, x, y, n, maxdist, res);
}

int
get_time(char *ptr, int left, struct TreeObject *t, double u) {
	GTimeVal t1, t2;
	int ret;
	double D;
	gchar *ts;

	switch(t->type) {
	case SEGMENT:
		t1.tv_sec = t->u.seg.p1->time.tv_sec;
		t1.tv_usec = t->u.seg.p1->time.tv_usec;
		t2.tv_sec = t->u.seg.p2->time.tv_sec;
		t2.tv_usec = t->u.seg.p2->time.tv_usec;
		t2.tv_sec -= t1.tv_sec;
		t2.tv_usec -= t1.tv_usec;
		if(t2.tv_usec < 0) {
			t2.tv_usec += 1000000;
			t2.tv_sec--;
		}
		D = t2.tv_sec * 1000000 + t2.tv_usec;
		D *= u;
		t2.tv_sec = (int)(D/1000000);
		t2.tv_usec = (int)fmod(D, 1000000);;
		t1.tv_sec += t2.tv_sec;
		t1.tv_usec += t2.tv_usec;

		ts = g_time_val_to_iso8601(&t1);
		if(ts == NULL) {
			ret = snprintf(ptr, left, "\t%ld %ld", t1.tv_sec, t1.tv_usec);
		}
		else
			ret = snprintf(ptr, left, "\t%s", ts);
		g_free(ts);
		return ret;
	default:
		break;
	}

	return 0;
}

static char *
Heading(struct Point *p1, struct Point *p2) {
	int i;
	static char *dirs[8] = { "E", "NE", "N", "NW", "W", "SW", "S", "SE" };
	double dx = p2->geo_lon - p1->geo_lon;
	double dy = p2->geo_lat - p1->geo_lat;
	double a = atan2(dy, dx) + (M_PI/8);
	if(a < 0) a += 2*M_PI;
	i = (int)(a/(M_PI/4));
	return dirs[i];
}

#define NP 100
char *
get_near_object(struct MapView *mapview, int x, int y) {
	struct TreeSearchResult res[NP];
	int i;
	static char ret[1000];
	int filled;

	memset(res, 0, sizeof(res));

	for(i = 0; i < mapview->rt.n_layers; i++) {
		if(mapview->rt.layers[i].flags & LAYER_IS_TREE_SEARCHABLE) {
			// g_message("--------------");
			find_objects_xy(mapview->rt.layers[i].priv, x, y, NP, 5, res);
		}
	}

	if(res[0].obj == NULL)
		return NULL;

	filled = 0;
	filled += snprintf(ret+filled, sizeof(ret)-filled, "%s", UTF8[UTF8_LTRMARK]);
	for(i = 0; i < NP && res[i].obj; i++) {
		if(filled >= sizeof(ret)-20)
			break;
		if(i > 0)
			filled += snprintf(ret+filled, sizeof(ret)-filled, "\n%s", UTF8[UTF8_LTRMARK]);
		switch(res[i].obj->type) {
		case SEGMENT:
			if(filled >= sizeof(ret))
				break;
			filled += snprintf(ret+filled, sizeof(ret)-filled, "%s: %.3f Km %s %.1fm",
				res[i].obj->u.seg.trk->name,
				((1-res[i].u)*res[i].obj->u.seg.p1->distance + res[i].u*res[i].obj->u.seg.p2->distance)/1000,
				Heading(&res[i].obj->u.seg.p1->point, &res[i].obj->u.seg.p2->point),
				(1-res[i].u)*res[i].obj->u.seg.p1->point.elevation + res[i].u*res[i].obj->u.seg.p2->point.elevation);
			if(filled >= sizeof(ret))
				break;
			filled += get_time(ret+filled, sizeof(ret)-filled, res[i].obj, res[i].u);
			break;
		case WAYPOINT:
			if(filled >= sizeof(ret))
				break;
			if(res[i].obj->u.wpt.wpt->name) {
				filled += snprintf(ret+filled, sizeof(ret)-filled, "%s",
					res[i].obj->u.wpt.wpt->name);
				if(res[i].obj->u.wpt.wpt->description)
					filled += snprintf(ret+filled, sizeof(ret)-filled, ": ");
			}
			if(res[i].obj->u.wpt.wpt->description) {
				filled += snprintf(ret+filled, sizeof(ret)-filled, "%s",
					res[i].obj->u.wpt.wpt->description);
			}
			//filled += snprintf(ret+filled, sizeof(ret)-filled, "%s: %s",
			//	res[i].obj->u.wpt.wpt->name, res[i].obj->u.wpt.wpt->description);
			break;
		}
	}
	return ret;
}
