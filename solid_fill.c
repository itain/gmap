#include "gmap.h"
// #include <assert.h>

struct SolidFillData {
	double r, g, b, a;
};

static struct SolidFillData *
solid_fill_create_data(double r, double g, double b, double a) {
	struct SolidFillData *ret;

	ret = (struct SolidFillData *)gmap_malloc(sizeof(struct SolidFillData));

	ret->r = r;
	ret->g = g;
	ret->b = b;
	ret->a = a;

	return ret;
}

static void
solid_fill_render_layer(const struct Layer *layer, const struct RenderContext *context) {
	struct SolidFillData *data = (struct SolidFillData *)layer->data;

	cairo_set_source_rgb(context->ct, data->r, data->g, data->b);
	if(layer->type == LAYER_SOLID_ALPHA) {
		cairo_paint_with_alpha(context->ct, data->a);
	}
	else {
		cairo_paint(context->ct);
	}
}

static void
solid_fill_calc_target_data(struct Layer *layer, const struct RenderTarget *target) {
	/* Nothing to do here */
}

static const struct LayerOps solid_fill_layer_ops = {
	solid_fill_render_layer,
	solid_fill_calc_target_data,
	NULL,
};

void
solid_fill_init_layer(struct Layer *layer, enum LayerType type, double a, double r, double g, double b) {
	layer->type = type;
	layer->ops = &solid_fill_layer_ops;
	layer->flags = LAYER_IS_VISIBLE;
	layer->data = solid_fill_create_data(r, g, b, a);
	layer->priv = NULL;
}
