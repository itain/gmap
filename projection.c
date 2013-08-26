#include "gmap.h"
#include <ogr_api.h>
#include <cpl_conv.h>
#include <gdalwarper.h>

extern GDALDatasetH GDALOpenPixbuf(GdkPixbuf *, GDALAccess);
extern GDALDatasetH GDALOpenPixbuf2(GdkPixbuf *, GDALAccess, int x, int y, int w, int h);
extern GDALDatasetH GDALOpenCairo(cairo_surface_t *, GDALAccess);
struct ProjectionData;

/* Parameter names for projections. The projection name is always seen. Other parameters are
   Seen if they match the selected projection.  */
enum param {
	param_ProjName, param_VariantName, param_CenterLat, param_CenterLong, param_Scale,
	param_CentralMeridian, param_SatelliteHeight, param_Zone, param_North, param_Azimuth,
	param_PseudoStdParallelLat, param_RectToSkew, param_StdP1, param_StdP2, param_OriginLat,
	param_CMeridian, param_LatitudeOfOrigin, param_Lat1, param_Long1, param_Lat2, param_Long2,
	param_NAD83, param_OverrideUnitName, param_OverrideUnit,
	param_FalseEasting, param_FalseNorthing,
};

/* List of projection names/descriptions. It must match the enum above. */
char *ProjParamDescr[] = {
	"Projection Name",
	"Variant Name",
	"Center Latitude",
	"Center Longitude",
	"Scale",
	"Central Meridian",
	"Satellite Height",
	"UTM Zone",
	"North or South",
	"Azimuth",
	"Pseudo Parallel Lat",
	"RectToSkew",
	"Std P1",
	"Std P2",
	"Origin Latitude",
	"CMeridian",
	"Latitude of Origin",
	"Latitude 1",
	"Longitude 1",
	"Latitude 2",
	"Longitude 2",
	"NAD83",
	"Override Unit Name",
	"Override Unit",
	"False Easting",
	"False Northing",
};

/* 
 * Known projections.
 */
enum Projection {
	LatitudeLongitude,
	AlbersConicEqualArea,
	AzimuthalEquidistant,
	Bonne,
	CylindricalEqualArea,
	CassiniSoldner,
	EquidistantConic,
	EckertIV,
	EckertVI,
	Equirectangular,
	GeostationarySatellite,
	GoodeHomolosine,
	GallStereograpic,
	Gnomonic,
	HotineObliqueMercatorAzimuthAngle,
	HotineObliqueMercatorTwoPointson,
	KrovakObliqueConicConformal,
	LambertAzimuthalEqualArea,
	LambertConformalConic,
	LambertConformalConic1SP,
	LambertConformalConicBelgium,
	MillerCylindrical,
	Mercator,
	Mollweide,
	NewZealandMapGrid,
	ObliqueStereographic,
	Orthographic,
	Polyconic,
	PolarStereographic,
	Robinson,
	Sinusoidal,
	Stereographic,
	SwissObliqueCylindrical,
	TransverseMercator,
	TransverseMercatorvariants,
	TunesiaMiningGrid,
	TransverseMercatorSouthOriented,
	// TwoPointEquidistant,
	VanDerGrinten,
	UniversalTransverseMercator,
	// USStatePlane,
};

/* Callback functions to set a projection.
 * p->osrs is already initialized with the proper datum.
 * Except for LongLat, OSRSetProjCS was called.
 */
static bool set_LongLat(struct ProjectionData *p);
static bool set_AlbersConicEqualArea(struct ProjectionData *p);
static bool set_AzimuthalEquidistant(struct ProjectionData *p);
static bool set_Bonne(struct ProjectionData *p);
static bool set_CylindricalEqualArea(struct ProjectionData *p);
static bool set_CassiniSoldner(struct ProjectionData *p);
static bool set_EquidistantConic(struct ProjectionData *p);
static bool set_EckertIV(struct ProjectionData *p);
static bool set_EckertVI(struct ProjectionData *p);
static bool set_Equirectangular(struct ProjectionData *p);
static bool set_GeostationarySatellite(struct ProjectionData *p);
static bool set_GoodeHomolosine(struct ProjectionData *p);
static bool set_GallStereograpic(struct ProjectionData *p);
static bool set_Gnomonic(struct ProjectionData *p);
static bool set_HotineObliqueMercatorAzimuthAngle(struct ProjectionData *p);
static bool set_HotineObliqueMercatorTwoPointson(struct ProjectionData *p);
static bool set_KrovakObliqueConicConformal(struct ProjectionData *p);
static bool set_LambertAzimuthalEqualArea(struct ProjectionData *p);
static bool set_LambertConformalConic(struct ProjectionData *p);
static bool set_LambertConformalConic1SP(struct ProjectionData *p);
static bool set_LambertConformalConicBelgium(struct ProjectionData *p);
static bool set_MillerCylindrical(struct ProjectionData *p);
static bool set_Mercator(struct ProjectionData *p);
static bool set_Mollweide(struct ProjectionData *p);
static bool set_NewZealandMapGrid(struct ProjectionData *p);
static bool set_ObliqueStereographic(struct ProjectionData *p);
static bool set_Orthographic(struct ProjectionData *p);
static bool set_Polyconic(struct ProjectionData *p);
static bool set_PolarStereographic(struct ProjectionData *p);
static bool set_Robinson(struct ProjectionData *p);
static bool set_Sinusoidal(struct ProjectionData *p);
static bool set_Stereographic(struct ProjectionData *p);
static bool set_SwissObliqueCylindrical(struct ProjectionData *p);
static bool set_TransverseMercator(struct ProjectionData *p);
static bool set_TransverseMercatorvariants(struct ProjectionData *p);
static bool set_TunesiaMiningGrid(struct ProjectionData *p);
static bool set_TransverseMercatorSouthOriented(struct ProjectionData *p);
//static bool set_TwoPointEquidistant(struct ProjectionData *p);
static bool set_VanDerGrinten(struct ProjectionData *p);
static bool set_UniversalTransverseMercator(struct ProjectionData *p);
//static bool set_USStatePlane(struct ProjectionData *p);

#define P(i) (1<<(i))
struct {
	char *ProjName;
	unsigned long params;
	bool (*set_func)(struct ProjectionData *p);
} projections[] = {
	{ "Latitude Longitude", 0,
	  set_LongLat, },
	{ "Albers Conic Equal Area",
	  (P(param_StdP1)|P(param_StdP2)|P(param_CenterLat)|P(param_CenterLong)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_AlbersConicEqualArea, },
	{ "Azimuthal Equidistant",
	  (P(param_CenterLat)|P(param_CenterLong)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_AzimuthalEquidistant, },
	{ "Bonne",
	  (P(param_StdP1)|P(param_CentralMeridian)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_Bonne, },
	{ "Cylindrical Equal Area",
	  (P(param_StdP1)|P(param_CentralMeridian)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_CylindricalEqualArea, },
	{ "Cassini-Soldner",
	  (P(param_CenterLat)|P(param_CenterLong)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_CassiniSoldner, },
	{ "Equidistant Conic",
	  (P(param_StdP1)|P(param_StdP2)|P(param_CenterLat)|P(param_CenterLong)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_EquidistantConic, },
	{ "Eckert IV",
	  (P(param_CentralMeridian)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_EckertIV, },
	{ "Eckert VI",
	  (P(param_CentralMeridian)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_EckertVI, },
	{ "Equirectangular",
	  (P(param_CenterLat)|P(param_CenterLong)|P(param_FalseEasting)|P(param_FalseNorthing)), 
	  set_Equirectangular, },
	{ "Geostationary Satellite",
	  (P(param_CentralMeridian)|P(param_SatelliteHeight)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_GeostationarySatellite, },
	{ "Goode Homolosine",
	  (P(param_CentralMeridian)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_GoodeHomolosine, },
	{ "Gall Stereograpic",
	  (P(param_CentralMeridian)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_GallStereograpic},
	{ "Gnomonic",
	  (P(param_CenterLat)|P(param_CenterLong)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_Gnomonic, },
	{ "Hotine Oblique Mercator (using azimuth angle)",
	  (P(param_CenterLat)|P(param_CenterLong)|
	   P(param_Azimuth)|P(param_RectToSkew)|
	   P(param_Scale)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_HotineObliqueMercatorAzimuthAngle, },
	{ "Hotine Oblique Mercator (using two points on projection centerline)",
	  (P(param_CenterLat)|P(param_CenterLong)|
	   P(param_Lat1)|P(param_Long1)|P(param_Lat2)|P(param_Long2)|
	   P(param_Scale)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_HotineObliqueMercatorTwoPointson, },
	{ "Krovak Oblique Conic Conformal",
	  (P(param_CenterLat)|P(param_CenterLong)|
	   P(param_Azimuth)|P(param_PseudoStdParallelLat)|
	   P(param_Scale)|P(param_FalseEasting)|P(param_FalseNorthing)),
	   set_KrovakObliqueConicConformal, },
	{ "Lambert Azimuthal Equal-Area",
	  (P(param_CenterLat)|P(param_CenterLong)|
	   P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_LambertAzimuthalEqualArea, },
	{ "Lambert Conformal Conic",
	  (P(param_StdP1)|P(param_StdP2)|
	   P(param_CenterLat)|P(param_CenterLong)|
	   P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_LambertConformalConic},
	{ "Lambert Conformal Conic 1SP",
	  (P(param_CenterLat)|P(param_CenterLong)|
	   P(param_Scale)|
	   P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_LambertConformalConic1SP, },
	{ "Lambert Conformal Conic (Belgium)",
	  (P(param_StdP1)|P(param_StdP2)|
	   P(param_CenterLat)|P(param_CenterLong)|
	   P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_LambertConformalConicBelgium, },
	{ "Miller Cylindrical",
	  (P(param_CenterLat)|P(param_CenterLong)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_MillerCylindrical, },
	{ "Mercator",
	  (P(param_CenterLat)|P(param_CenterLong)|P(param_Scale)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_Mercator, },
	{ "Mollweide",
	  (P(param_CentralMeridian)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_Mollweide, },
	{ "New Zealand Map Grid",
	  (P(param_CenterLat)|P(param_CenterLong)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_NewZealandMapGrid, },
	{ "Oblique Stereographic",
	  (P(param_OriginLat)|P(param_CMeridian)|P(param_Scale)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_ObliqueStereographic, },
	{ "Orthographic",
	  (P(param_CenterLat)|P(param_CenterLong)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_Orthographic, },
	{ "Polyconic",
	  (P(param_CenterLat)|P(param_CenterLong)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_Polyconic, },
	{ "Polar Stereographic",
	  (P(param_CenterLat)|P(param_CenterLong)|P(param_Scale)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_PolarStereographic, },
	{ "Robinson",
	  (P(param_CenterLong)|P(param_Scale)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_Robinson, },
	{ "Sinusoidal",
	  (P(param_CenterLong)|P(param_Scale)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_Sinusoidal, },
	{ "Stereographic",
	  (P(param_CenterLat)|P(param_CenterLong)|P(param_Scale)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_Stereographic, },
	{ "Swiss Oblique Cylindrical",
	  (P(param_LatitudeOfOrigin)|P(param_CentralMeridian)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_SwissObliqueCylindrical, },
	{ "Transverse Mercator",
	  (P(param_CenterLat)|P(param_CenterLong)|P(param_Scale)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_TransverseMercator, },
	{ "Transverse Mercator variants",
	  (P(param_VariantName)|P(param_CenterLat)|P(param_CenterLong)|P(param_Scale)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_TransverseMercatorvariants, },
	{ "Tunesia Mining Grid",
	  (P(param_CenterLat)|P(param_CenterLong)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_TunesiaMiningGrid, },
	{ "Transverse Mercator (South Oriented)",
	  (P(param_CenterLat)|P(param_CenterLong)|P(param_Scale)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_TransverseMercatorSouthOriented, },
	/* { "Two Point Equidistant",
	  (P(param_Lat1)|P(param_Long1)|P(param_Lat2)|P(param_Long2)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_TwoPointEquidistant, }, */
	{ "VanDerGrinten",
	  (P(param_CenterLong)|P(param_FalseEasting)|P(param_FalseNorthing)),
	  set_VanDerGrinten, },
	{ "Universal Transverse Mercator",
	  (P(param_Zone)|P(param_North)),
	  set_UniversalTransverseMercator, },
	/* { "US State Plane",
	  (P(param_Zone)|P(param_NAD83)|P(param_OverrideUnitName)|P(param_OverrideUnit)),
	  set_USStatePlane, }, */
};

struct ProjectionData {
	char	*WKT;
	OGRSpatialReferenceH osrs;
	OGRErr	err;

	/* Projected or Geographic */
	//bool	Projected;

	/* Datum Params */
	/* ellibsoid */
	//double	MajorAxis;	/* or A */
	//double  InvF;		/* Inverse Falttening = 1/f or a/(a-b) */

	/* to WGS84 */
	//double	dx, dy, dz;
	//double	ex,ey,ez,ppm;

	/* projection parameters */
	GtkWidget *assistant;

	gint	  n_pages;
	gint      (**next_func)(struct ProjectionData *);

	GtkWidget *intro_page;
	GtkWidget *intro_page_combo;
	gint	  intro_page_no;

	GtkWidget *from_wkt_page;
	GtkWidget *from_wkt_textview;
	gint	  from_wkt_page_no;

	GtkWidget *spheroid_page;
	gint	  spheroid_page_no;
	GtkWidget *spheroid_SemiMajorAxis;
	GtkWidget *spheroid_InvFlat;

	GtkWidget *toWGS84_page;
	gint	  toWGS84_page_no;
	GtkWidget *toWGS84_param_name_widget[G_N_ELEMENTS(ProjParamDescr)];
	GtkWidget *toWGS84_param_value_widget[G_N_ELEMENTS(ProjParamDescr)];

	GtkWidget *projection_page;
	gint	  projection_page_no;
	GtkWidget *proj_param_name_widget[G_N_ELEMENTS(ProjParamDescr)];
	GtkWidget *proj_param_value_widget[G_N_ELEMENTS(ProjParamDescr)];
	GtkWidget *view_wkt_checkbox;

	GtkWidget *view_wkt_page;
	GtkWidget *view_wkt_textview;
	gint	  view_wkt_page_no;

	GtkWidget *accept_page;
	gint	  accept_page_no;

	GtkWidget *had_error;
	GtkWidget *save_by_name;
	GtkWidget *show_wkt;
	GtkWidget *summary;

	char *done_message;
	GCallback done_callback;
	gpointer  done_callback_param;
	GCallback closing_callback;
	gpointer  closing_callback_param;
};

/* pre-declare page functions */
static gint intro_page(struct ProjectionData *p);
static gint from_wkt_page(struct ProjectionData *p);
static gint accept_page(struct ProjectionData *p);
static gint spheroid_page(struct ProjectionData *p);
static gint toWGS84_page(struct ProjectionData *p);
static gint projection_page(struct ProjectionData *p);
static gint view_wkt_page(struct ProjectionData *p);

/* All Projection functions */
static bool set_LongLat(struct ProjectionData *p) {
	return p->err == OGRERR_NONE;
}

static bool set_Robinson(struct ProjectionData *p) {
	double dfCenterLong, dfFalseEasting, dfFalseNorthing;
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetRobinson(p->osrs, dfCenterLong, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_Orthographic(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetOrthographic(p->osrs, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_TransverseMercator(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfScale, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfScale = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Scale]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetTM(p->osrs, dfCenterLat, dfCenterLong, dfScale, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_TransverseMercatorSouthOriented(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfScale, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfScale = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Scale]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetTMSO(p->osrs, dfCenterLat, dfCenterLong, dfScale, dfFalseEasting, dfFalseNorthing); 
	return p->err == OGRERR_NONE;
}

static bool set_CassiniSoldner(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetCS(p->osrs, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_UniversalTransverseMercator(struct ProjectionData *p) {
	int nZone = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Zone]));
	bool bNorth = gtk_combo_box_get_active(GTK_COMBO_BOX(p->proj_param_value_widget[param_North])) == 0;
	p->err = OSRSetUTM(p->osrs, nZone, bNorth);
	return p->err == OGRERR_NONE;
}

static bool set_AlbersConicEqualArea(struct ProjectionData *p) {
	double dfStdP1, dfStdP2, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing;
	dfStdP1 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_StdP1]));
	dfStdP2 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_StdP2]));
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetACEA(p->osrs, dfStdP1, dfStdP2, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_AzimuthalEquidistant(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetAE(p->osrs, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_Bonne(struct ProjectionData *p) {
	double dfStandardParallel, dfCentralMeridian, dfFalseEasting, dfFalseNorthing;
	dfStandardParallel = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_StdP1]));
	dfCentralMeridian = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CentralMeridian]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetBonne(p->osrs, dfStandardParallel, dfCentralMeridian, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_CylindricalEqualArea(struct ProjectionData *p) {
	double dfStdP1, dfCentralMeridian, dfFalseEasting, dfFalseNorthing;
	dfStdP1 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_StdP1]));
	dfCentralMeridian = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CentralMeridian]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetCEA(p->osrs, dfStdP1, dfCentralMeridian, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_EquidistantConic(struct ProjectionData *p) {
	double dfStdP1, dfStdP2, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing;
	dfStdP1 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_StdP1]));
	dfStdP2 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_StdP2]));
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetEC(p->osrs, dfStdP1, dfStdP2, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_EckertIV(struct ProjectionData *p) {
	double dfCentralMeridian, dfFalseEasting, dfFalseNorthing;
	dfCentralMeridian = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CentralMeridian]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetEckertIV(p->osrs, dfCentralMeridian, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_EckertVI(struct ProjectionData *p) {
	double dfCentralMeridian, dfFalseEasting, dfFalseNorthing;
	dfCentralMeridian = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CentralMeridian]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetEckertVI(p->osrs, dfCentralMeridian, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_Equirectangular(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetEquirectangular(p->osrs, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_GeostationarySatellite(struct ProjectionData *p) {
	double dfCentralMeridian, dfSatelliteHeight, dfFalseEasting, dfFalseNorthing;
	dfCentralMeridian = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CentralMeridian]));
	dfSatelliteHeight = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_SatelliteHeight]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetGEOS(p->osrs, dfCentralMeridian, dfSatelliteHeight, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_GoodeHomolosine(struct ProjectionData *p) {
	double dfCentralMeridian, dfFalseEasting, dfFalseNorthing;
	dfCentralMeridian = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CentralMeridian]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetGH(p->osrs, dfCentralMeridian, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_GallStereograpic(struct ProjectionData *p) {
	double dfCentralMeridian, dfFalseEasting, dfFalseNorthing;
	dfCentralMeridian = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CentralMeridian]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetGS(p->osrs, dfCentralMeridian, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_Gnomonic(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetGnomonic(p->osrs, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_HotineObliqueMercatorAzimuthAngle(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfAzimuth, dfRectToSkew, dfScale, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfAzimuth = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Azimuth]));
	dfRectToSkew = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_RectToSkew]));
	dfScale = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Scale]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetHOM(p->osrs, dfCenterLat, dfCenterLong, dfAzimuth, dfRectToSkew, dfScale, dfFalseEasting, dfFalseNorthing );
	return p->err == OGRERR_NONE;
}

static bool set_HotineObliqueMercatorTwoPointson(struct ProjectionData *p) {
	double dfCenterLat, dfLat1, dfLong1, dfLat2, dfLong2, dfScale, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfLat1 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Lat1]));
	dfLong1 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Long1]));
	dfLat2 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Lat2]));
	dfLong2 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Long2]));
	dfScale = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Scale]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetHOM2PNO(p->osrs, dfCenterLat, dfLat1, dfLong1, dfLat2, dfLong2, dfScale, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_KrovakObliqueConicConformal(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfAzimuth, dfPseudoStdParallelLat, dfScale, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfAzimuth = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Azimuth]));
	dfPseudoStdParallelLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_PseudoStdParallelLat]));
	dfScale = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Scale]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetKrovak(p->osrs, dfCenterLat, dfCenterLong, dfAzimuth, dfPseudoStdParallelLat, dfScale, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_LambertAzimuthalEqualArea(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetLAEA(p->osrs, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_LambertConformalConic(struct ProjectionData *p) {
	double dfStdP1, dfStdP2, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing;
	dfStdP1 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_StdP1]));
	dfStdP2 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_StdP2]));
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetLCC(p->osrs, dfStdP1, dfStdP2, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_LambertConformalConic1SP(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfScale, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfScale = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Scale]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetLCC1SP(p->osrs, dfCenterLat, dfCenterLong, dfScale, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_LambertConformalConicBelgium(struct ProjectionData *p) {
	double dfStdP1, dfStdP2, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing;
	dfStdP1 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_StdP1]));
	dfStdP2 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_StdP2]));
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetLCCB(p->osrs, dfStdP1, dfStdP2, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_MillerCylindrical(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetMC(p->osrs, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_Mercator(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfScale, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfScale = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Scale]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetMercator(p->osrs, dfCenterLat, dfCenterLong, dfScale, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_Mollweide(struct ProjectionData *p) {
	double dfCentralMeridian, dfFalseEasting, dfFalseNorthing;
	dfCentralMeridian = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CentralMeridian]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetMollweide(p->osrs, dfCentralMeridian, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_NewZealandMapGrid(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetNZMG(p->osrs, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_ObliqueStereographic(struct ProjectionData *p) {
	double dfOriginLat, dfCMeridian, dfScale, dfFalseEasting, dfFalseNorthing;
	dfOriginLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_OriginLat]));
	dfCMeridian = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CMeridian]));
	dfScale = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Scale]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetOS(p->osrs, dfOriginLat, dfCMeridian, dfScale, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_Polyconic(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetPolyconic(p->osrs, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_PolarStereographic(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfScale, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfScale = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Scale]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetPS(p->osrs, dfCenterLat, dfCenterLong, dfScale, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_Sinusoidal(struct ProjectionData *p) {
	double dfCenterLong, dfFalseEasting, dfFalseNorthing;
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetSinusoidal(p->osrs, dfCenterLong, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_Stereographic(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfScale, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfScale = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Scale]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetStereographic(p->osrs, dfCenterLat, dfCenterLong, dfScale, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_SwissObliqueCylindrical(struct ProjectionData *p) {
	double dfLatitudeOfOrigin, dfCentralMeridian, dfFalseEasting, dfFalseNorthing;
	dfLatitudeOfOrigin = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_LatitudeOfOrigin]));
	dfCentralMeridian = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CentralMeridian]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetSOC(p->osrs, dfLatitudeOfOrigin, dfCentralMeridian, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_TransverseMercatorvariants(struct ProjectionData *p) {
	const char *pszVariantName;
	double dfCenterLat, dfCenterLong, dfScale, dfFalseEasting, dfFalseNorthing;
	pszVariantName = gtk_entry_get_text(GTK_ENTRY(p->proj_param_value_widget[param_VariantName]));
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfScale = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_Scale]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetTMVariant(p->osrs, pszVariantName, dfCenterLat, dfCenterLong, dfScale, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_TunesiaMiningGrid(struct ProjectionData *p) {
	double dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing;
	dfCenterLat = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLat]));
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetTMG(p->osrs, dfCenterLat, dfCenterLong, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool set_VanDerGrinten(struct ProjectionData *p) {
	double dfCenterLong, dfFalseEasting, dfFalseNorthing;
	dfCenterLong = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_CenterLong]));
	dfFalseEasting = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseEasting]));
	dfFalseNorthing = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->proj_param_value_widget[param_FalseNorthing]));
	p->err = OSRSetVDG(p->osrs, dfCenterLong, dfFalseEasting, dfFalseNorthing);
	return p->err == OGRERR_NONE;
}

static bool apply_toWGS84(struct ProjectionData *p) {
	double  d[7];
	int i;

	for(i = 0; i < 7; i++)
		d[i] = gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->toWGS84_param_value_widget[i]));

	p-> err = OSRSetTOWGS84(p->osrs, d[0], d[1], d[2], d[3], d[4], d[5], d[6]);
	return p->err == OGRERR_NONE;
}

static void
set_next_func(struct ProjectionData *p, gint n, gint(*func)(struct ProjectionData *)) {
	if(p->n_pages < n+1) {
		p->n_pages = n+1;
		p->next_func = (gint (**)(struct ProjectionData *))gmap_realloc(p->next_func, p->n_pages * sizeof(gint (*)(struct ProjectionData *)));
	}
	p->next_func[n] = func;
}

static void
projection_clean_all(struct ProjectionData *p) {
	if(p->WKT != NULL)
		gmap_free(p->WKT);
	p->WKT = NULL;

	if(p->osrs != NULL)
		OSRDestroySpatialReference(p->osrs);
	p->osrs = NULL;
	p->err = OGRERR_NONE;
}

static bool
projection_done(struct ProjectionData *p) {
	char *wkt;
	if(p->err != OGRERR_NONE)
		return FALSE;
	if(p->osrs == NULL)
		return FALSE;
	p->err = OSRValidate(p->osrs);
	if(p->err)
		return FALSE;
	p->err = OSRExportToWkt(p->osrs, &wkt);
	if(p->err != OGRERR_NONE || wkt == NULL)
		return FALSE;
	p->WKT = gmap_strdup(wkt);
	CPLFree(wkt);
	return TRUE;
}

/* Intro page */
static gint
intro_page_next(struct ProjectionData *p) {
	gint value;
	g_message("intro_page_next");
	value = gtk_combo_box_get_active(GTK_COMBO_BOX(p->intro_page_combo));
	if(value == 0)
		return from_wkt_page(p);
	else
		return accept_page(p);
}

static gint
intro_page(struct ProjectionData *p) {
	if(p->intro_page == NULL) {
		GtkWidget *widget;
		p->intro_page = gtk_vbox_new(FALSE, 1);
		widget = gtk_label_new(
			"Intruduction to Geographic Projections\n"
			"You can pick one of the named projections from the list below, or proceed to create a new one.");
		gtk_label_set_line_wrap(GTK_LABEL(widget), TRUE);
		gtk_label_set_line_wrap_mode(GTK_LABEL(widget), PANGO_WRAP_WORD);
		gtk_widget_show(widget);
		gtk_box_pack_start(GTK_BOX(p->intro_page), widget, TRUE, FALSE, 0);
		widget = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(widget), "Create new projection...");
		//gtk_combo_box_append_text(GTK_COMBO_BOX(widget), "Israel new");
		gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
		gtk_widget_show(widget);
		p->intro_page_combo = widget;
		gtk_box_pack_start(GTK_BOX(p->intro_page), widget, FALSE, FALSE, 0);
		gtk_widget_show(p->intro_page);
		p->intro_page_no = gtk_assistant_append_page(GTK_ASSISTANT(p->assistant), p->intro_page);
		g_message("p->intro_page_no = %d", p->intro_page_no);
		gtk_assistant_set_page_type(GTK_ASSISTANT(p->assistant), p->intro_page, GTK_ASSISTANT_PAGE_INTRO);
		gtk_assistant_set_page_title(GTK_ASSISTANT(p->assistant), p->intro_page, "Select saved projection");
		gtk_assistant_set_page_complete(GTK_ASSISTANT(p->assistant), p->intro_page, FALSE);
		set_next_func(p, p->intro_page_no, intro_page_next);
	}
	return p->intro_page_no;
}

/* from_wkt_page */
static gint
from_wkt_page_next(struct ProjectionData *p) {
	char *str;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	g_message("from_wkt_page_next");
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(p->from_wkt_textview));
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	str = gtk_text_buffer_get_slice(buffer, &start, &end, TRUE);
	g_message("WKT='%s'", str);
	if(*str) {
		projection_clean_all(p);
		p->osrs = OSRNewSpatialReference(NULL);
		p->err = OSRSetFromUserInput(p->osrs, str);
		if(p->err == OGRERR_NONE) {
			g_free(str);
			return accept_page(p);
		}
		g_message("Do not understand the '%s'", str);
		g_free(str);
		return from_wkt_page(p);
	}
	g_free(str);
	return spheroid_page(p);
}

static void
from_wkt_text_buffer_changed_handler(GtkTextBuffer *textbuffer, struct ProjectionData *p) {
	char *str;
	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(textbuffer, &start, &end);
	str = gtk_text_buffer_get_slice(textbuffer, &start, &end, TRUE);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(p->assistant), p->from_wkt_page, FALSE);
	if(str && *str) {
		projection_clean_all(p);
		p->osrs = OSRNewSpatialReference(NULL);
		p->err = OSRSetFromUserInput(p->osrs, str);
		if(p->err == OGRERR_NONE) {
			gtk_assistant_set_page_complete(GTK_ASSISTANT(p->assistant), p->from_wkt_page, TRUE);
		}
	}
	else {
		gtk_assistant_set_page_complete(GTK_ASSISTANT(p->assistant), p->from_wkt_page, TRUE);
	}
	g_free(str);
}

static gint
from_wkt_page(struct ProjectionData *p) {
	if(p->from_wkt_page == NULL) {
		GtkWidget *widget;
		GtkTextBuffer *textbuffer;
		p->from_wkt_page = gtk_vbox_new(FALSE, 1);
		widget = gtk_label_new(
			"If you already have a WKT or PROJ4 string you can paste it here, "
			"Otherwise just click the forward button.");
		gtk_label_set_line_wrap(GTK_LABEL(widget), TRUE);
		gtk_label_set_line_wrap_mode(GTK_LABEL(widget), PANGO_WRAP_WORD);
		gtk_widget_show(widget);
		gtk_box_pack_start(GTK_BOX(p->from_wkt_page), widget, FALSE, FALSE, 0);
		p->from_wkt_textview = gtk_text_view_new();
		textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(p->from_wkt_textview));
		g_signal_connect(G_OBJECT(textbuffer), "changed", G_CALLBACK(from_wkt_text_buffer_changed_handler), p);
		gtk_widget_show(p->from_wkt_textview);
		widget = gtk_scrolled_window_new(NULL, NULL);
		gtk_container_add(GTK_CONTAINER(widget), p->from_wkt_textview);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_widget_show(widget);
		gtk_box_pack_start(GTK_BOX(p->from_wkt_page), widget, TRUE, TRUE, 0);
		gtk_widget_show(p->from_wkt_page);
		p->from_wkt_page_no = gtk_assistant_append_page(GTK_ASSISTANT(p->assistant), p->from_wkt_page);
		g_message("p->from_wkt_page_no = %d", p->from_wkt_page_no);
		g_object_set_data(G_OBJECT(p->from_wkt_page), "page-no", GINT_TO_POINTER(1+p->from_wkt_page_no));
		gtk_assistant_set_page_type(GTK_ASSISTANT(p->assistant), p->from_wkt_page, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_title(GTK_ASSISTANT(p->assistant), p->from_wkt_page, "Set Projection from WKT");
		gtk_assistant_set_page_complete(GTK_ASSISTANT(p->assistant), p->from_wkt_page, FALSE);
		set_next_func(p, p->from_wkt_page_no, from_wkt_page_next);
	}
	return p->from_wkt_page_no;
}

/* spheroid_page */
static gint
spheroid_page_next(struct ProjectionData *p) {
	return toWGS84_page(p);
}

static gint
spheroid_page(struct ProjectionData *p) {
	if(p->spheroid_page == NULL) {
		GtkWidget *table, *widget;
		p->spheroid_page = gtk_vbox_new(FALSE, 1);
		widget = gtk_label_new("Spheroid page");
		gtk_widget_show(widget);
		gtk_box_pack_start(GTK_BOX(p->spheroid_page), widget, FALSE, FALSE, 0);
		table = gtk_table_new(2, 2, FALSE);
		widget = gtk_label_new("Semi Major axis");
		gtk_widget_show(widget);
		gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 0, 1);
		widget = gtk_spin_button_new_with_range(0, 100000000, 0.1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), SRS_WGS84_SEMIMAJOR);
		p->spheroid_SemiMajorAxis = widget;
		gtk_widget_show(widget);
		gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 0, 1);

		widget = gtk_label_new("Inverse Flattening (1/F)");
		gtk_widget_show(widget);
		gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 1, 2);
		widget = gtk_spin_button_new_with_range(0, 1000, 0.0001);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(widget), 15);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), SRS_WGS84_INVFLATTENING);
		gtk_widget_show(widget);
		p->spheroid_InvFlat = widget;
		gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 1, 2);
		gtk_widget_show(table);
		gtk_box_pack_start(GTK_BOX(p->spheroid_page), table, FALSE, FALSE, 0);
		gtk_widget_show(p->spheroid_page);
		p->spheroid_page_no = gtk_assistant_append_page(GTK_ASSISTANT(p->assistant), p->spheroid_page);
		g_message("p->spheroid_page_no = %d", p->spheroid_page_no);
		g_object_set_data(G_OBJECT(p->spheroid_page), "page-no", GINT_TO_POINTER(1+p->spheroid_page_no));
		gtk_assistant_set_page_type(GTK_ASSISTANT(p->assistant), p->spheroid_page, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_title(GTK_ASSISTANT(p->assistant), p->spheroid_page, "Set Ellipsoid Parameters");
		gtk_assistant_set_page_complete(GTK_ASSISTANT(p->assistant), p->spheroid_page, FALSE);
		set_next_func(p, p->spheroid_page_no, spheroid_page_next);
	}
	return p->spheroid_page_no;
}

/* toWGS84_page */
static gint
toWGS84_page_next(struct ProjectionData *p) {
	return projection_page(p);
}

static const char *toWGS84_label_strings[] = { "DX", "DY", "DZ", "EX", "EY", "EZ", "PPM" };

static gint
toWGS84_page(struct ProjectionData *p) {
	if(p->toWGS84_page == NULL) {
		GtkWidget *table, *widget;
		int i;
		p->toWGS84_page = gtk_vbox_new(FALSE, 1);
		//widget = gtk_label_new("ToWGS84 page");
		//gtk_widget_show(widget);
		//gtk_box_pack_start(GTK_BOX(p->toWGS84_page), widget, FALSE, FALSE, 0);
		table = gtk_table_new(7, 2, FALSE);
		for(i = 0; i < 7; i++) {
			widget = gtk_label_new(toWGS84_label_strings[i]);
			p->toWGS84_param_name_widget[i] = widget;
			gtk_widget_show(widget);
			gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, i, 1+i);
			widget = gtk_spin_button_new_with_range(-1000, 1000, 0.1);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), 0);
			p->toWGS84_param_value_widget[i] = widget;
			gtk_widget_show(widget);
			gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, i, 1+i);
		}
		gtk_widget_show(table);
		gtk_box_pack_start(GTK_BOX(p->toWGS84_page), table, FALSE, FALSE, 0);
		gtk_widget_show(p->toWGS84_page);
		p->toWGS84_page_no = gtk_assistant_append_page(GTK_ASSISTANT(p->assistant), p->toWGS84_page);
		g_message("p->toWGS84_page_no = %d", p->toWGS84_page_no);
		g_object_set_data(G_OBJECT(p->toWGS84_page), "page-no", GINT_TO_POINTER(1+p->toWGS84_page_no));
		gtk_assistant_set_page_type(GTK_ASSISTANT(p->assistant), p->toWGS84_page, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_title(GTK_ASSISTANT(p->assistant), p->toWGS84_page, "Set Datum to WGS84 Parameters");
		gtk_assistant_set_page_complete(GTK_ASSISTANT(p->assistant), p->toWGS84_page, FALSE);
		set_next_func(p, p->toWGS84_page_no, toWGS84_page_next);
	}
	return p->toWGS84_page_no;
}

/* projection_page */
static gint
projection_page_next(struct ProjectionData *p) {
	gint value = gtk_combo_box_get_active(GTK_COMBO_BOX(p->proj_param_value_widget[param_ProjName]));
	g_message("Projection is: %s", projections[value].ProjName);
	if(value != LatitudeLongitude && projections[value].set_func == NULL) {
		g_message("NULL set_func");
		return projection_page(p);
	}
	projection_clean_all(p);
	p->osrs = OSRNewSpatialReference(NULL);
	if(p->osrs == NULL)
		goto had_error;
	if(value != LatitudeLongitude) {
		p->err = OSRSetProjCS(p->osrs, projections[value].ProjName);
		if(p->err != OGRERR_NONE)
			goto had_error;
	}
	p->err = OSRSetGeogCS(p->osrs, "unknown", "unknown", "unknown",
				gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->spheroid_SemiMajorAxis)),
				gtk_spin_button_get_value(GTK_SPIN_BUTTON(p->spheroid_InvFlat)),
				"Greenwich", 0, "degree", 0.0174532925199433);
	if(p->err != OGRERR_NONE)
		goto had_error;

	if(!apply_toWGS84(p))
		goto had_error;

	if(!projections[value].set_func(p))
		goto had_error;

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->view_wkt_checkbox)))
		return view_wkt_page(p);

	/* Finally !! */
	return accept_page(p);

had_error:
	return projection_page(p);
}

static void
projection_name_combo_changed(GtkComboBox *widget, struct ProjectionData *p) {
	gint value = gtk_combo_box_get_active(widget);
	int i;
	for(i = 1; i < G_N_ELEMENTS(ProjParamDescr); i++) {
		if(projections[value].params & P(i)) {
			gtk_widget_show(p->proj_param_name_widget[i]);
			gtk_widget_show(p->proj_param_value_widget[i]);
		}
		else {
			gtk_widget_hide(p->proj_param_name_widget[i]);
			gtk_widget_hide(p->proj_param_value_widget[i]);
		}
	}
	gtk_assistant_set_page_complete(GTK_ASSISTANT(p->assistant), p->projection_page,
		(value == LatitudeLongitude) || (projections[value].set_func != NULL));
}

#if DEBUG
static void
projection_param_changed(GtkWidget *widget, struct ProjectionData *p) {
	g_message("projection_param_changed: %d %s",
		GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "proj-param-num"))-1,
		(char *)g_object_get_data(G_OBJECT(widget), "proj-param-name"));
}
#endif

static gint
projection_page(struct ProjectionData *p) {
	if(p->projection_page == NULL) {
		GtkWidget *widget;
		GtkWidget *table;
		int i, j;
		p->projection_page = gtk_vbox_new(FALSE, 1);
		//widget = gtk_label_new("Projection page");
		//gtk_widget_show(widget);
		//gtk_box_pack_start(GTK_BOX(p->projection_page), widget, FALSE, FALSE, 0);
		table = gtk_table_new(G_N_ELEMENTS(ProjParamDescr), 2, FALSE);
		for(i = 0; i < G_N_ELEMENTS(ProjParamDescr); i++) {
			widget = gtk_label_new(ProjParamDescr[i]);
			gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, i, i+1);
			if(i == 0)
				gtk_widget_show(widget);
			p->proj_param_name_widget[i] = widget;
			switch(i) {
			case param_ProjName:
				widget = gtk_combo_box_new_text();
				for(j = 0; j < G_N_ELEMENTS(projections); j++)
					gtk_combo_box_append_text(GTK_COMBO_BOX(widget), projections[j].ProjName);
				g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(projection_name_combo_changed), p);
				gtk_widget_show(widget);
				break;
			case param_VariantName:
				widget = gtk_entry_new();
				break;
			case param_Zone:
				widget = gtk_spin_button_new_with_range(1, 60, 1);
				break;
			case param_NAD83:
				widget = gtk_toggle_button_new();
				break;
			case param_North:
				widget = gtk_combo_box_new_text();
				gtk_combo_box_append_text(GTK_COMBO_BOX(widget), "North");
				gtk_combo_box_append_text(GTK_COMBO_BOX(widget), "South");
				gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
				break;
			case param_Scale:
				widget = gtk_spin_button_new_with_range(-0, 2, 0.01);
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), 1);
				break;
			default:
				widget = gtk_spin_button_new_with_range(-100000000, 100000000, 0.1);
				gtk_spin_button_set_digits(GTK_SPIN_BUTTON(widget), 15);
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), 0);
				break;
			}
			// g_object_set_data(G_OBJECT(widget), "proj-param-num", GINT_TO_POINTER(i+1));
			// g_object_set_data(G_OBJECT(widget), "proj-param-name", ProjParamDescr[i]);
			// g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(projection_param_changed), p);
			p->proj_param_value_widget[i] = widget;
			gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, i, i+1);
		}
		gtk_widget_show(table);
		gtk_box_pack_start(GTK_BOX(p->projection_page), table, FALSE, FALSE, 0);
		widget = gtk_check_button_new_with_label("View/modify WKT");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
		p->view_wkt_checkbox = widget;
		gtk_widget_show(widget);
		gtk_box_pack_start(GTK_BOX(p->projection_page), widget, FALSE, FALSE, 0);
		gtk_widget_show(p->projection_page);
		p->projection_page_no = gtk_assistant_append_page(GTK_ASSISTANT(p->assistant), p->projection_page);
		g_message("p->projection_page_no = %d", p->projection_page_no);
		gtk_assistant_set_page_type(GTK_ASSISTANT(p->assistant), p->projection_page, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_title(GTK_ASSISTANT(p->assistant), p->projection_page, "Set Projection and Parameters");
		set_next_func(p, p->projection_page_no, projection_page_next);
		gtk_combo_box_set_active(GTK_COMBO_BOX(p->proj_param_value_widget[param_ProjName]), TransverseMercator);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(p->assistant), p->projection_page, FALSE);
	}
	return p->projection_page_no;
}

/* view_wkt_page */
static gint
view_wkt_page_next(struct ProjectionData *p) {
	// char *str;
	// GtkTextBuffer *buffer;
	// GtkTextIter start, end;
	// g_message("view_wkt_page_next");

	if(p->osrs)
		return accept_page(p);
	else
		return view_wkt_page(p);
#if 0
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(p->view_wkt_textview));
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	str = gtk_text_buffer_get_slice(buffer, &start, &end, TRUE);
	g_message("WKT='%.*s...'", 5, str);
	if(*str) {
		projection_clean_all(p);
		p->osrs = OSRNewSpatialReference(str);
		if(p->osrs != NULL) {
			g_free(str);
			return accept_page(p);
		}
		g_message("wkt is invalid");
		g_free(str);
		return view_wkt_page(p);
	}
	g_free(str);
	return view_wkt_page(p);
#endif
}

static void
view_wkt_text_buffer_changed_handler(GtkTextBuffer *textbuffer, struct ProjectionData *p) {
	char *str;
	GtkTextIter start, end;
	bool page_complete;
	g_message("view_wkt_text_buffer_changed_handler");
	gtk_text_buffer_get_bounds(textbuffer, &start, &end);
	str = gtk_text_buffer_get_slice(textbuffer, &start, &end, TRUE);
	projection_clean_all(p);
	if(str && *str)
		p->osrs = OSRNewSpatialReference(str);
	g_free(str);
	page_complete = (p->osrs != NULL);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(p->assistant), p->view_wkt_page, page_complete);
}

static gint
view_wkt_page(struct ProjectionData *p) {
	if(p->view_wkt_page == NULL) {
		GtkWidget *widget;
		GtkTextBuffer *textbuffer;
		p->view_wkt_page = gtk_vbox_new(FALSE, 1);
		widget = gtk_label_new("You can review and change this WKT.");
		gtk_label_set_line_wrap(GTK_LABEL(widget), TRUE);
		gtk_label_set_line_wrap_mode(GTK_LABEL(widget), PANGO_WRAP_WORD);
		gtk_widget_show(widget);
		gtk_box_pack_start(GTK_BOX(p->view_wkt_page), widget, FALSE, FALSE, 0);
		p->view_wkt_textview = gtk_text_view_new();
		textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(p->view_wkt_textview));
		g_signal_connect(G_OBJECT(textbuffer), "changed", G_CALLBACK(view_wkt_text_buffer_changed_handler), p);
		gtk_widget_show(p->view_wkt_textview);
		widget = gtk_scrolled_window_new(NULL, NULL);
		gtk_container_add(GTK_CONTAINER(widget), p->view_wkt_textview);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_widget_show(widget);
		gtk_box_pack_start(GTK_BOX(p->view_wkt_page), widget, TRUE, TRUE, 0);
		gtk_widget_show(p->view_wkt_page);
		p->view_wkt_page_no = gtk_assistant_append_page(GTK_ASSISTANT(p->assistant), p->view_wkt_page);
		g_message("p->view_wkt_page_no = %d", p->view_wkt_page_no);
		gtk_assistant_set_page_type(GTK_ASSISTANT(p->assistant), p->view_wkt_page, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_title(GTK_ASSISTANT(p->assistant), p->view_wkt_page, "Review projection's Well Known Text");
		gtk_assistant_set_page_complete(GTK_ASSISTANT(p->assistant), p->view_wkt_page, TRUE);
		set_next_func(p, p->view_wkt_page_no, view_wkt_page_next);
	}
	return p->view_wkt_page_no;
}

/* Accept page */
static gint
accept_page_next(struct ProjectionData *p) {
	g_message("accept_page_next");
	return 0;
}

static gint
accept_page(struct ProjectionData *p) {
	if(p->accept_page == NULL) {
		p->accept_page = gtk_label_new("Are you sure?");
		gtk_widget_show(p->accept_page);
		p->accept_page_no = gtk_assistant_append_page(GTK_ASSISTANT(p->assistant), p->accept_page);
		g_message("p->accept_page_no = %d", p->accept_page_no);
		gtk_assistant_set_page_type(GTK_ASSISTANT(p->assistant), p->accept_page, GTK_ASSISTANT_PAGE_CONFIRM);
		gtk_assistant_set_page_title(GTK_ASSISTANT(p->assistant), p->accept_page, "Accept new projection");
		gtk_assistant_set_page_complete(GTK_ASSISTANT(p->assistant), p->accept_page, TRUE);
		set_next_func(p, p->accept_page_no, accept_page_next);
	}
	return p->accept_page_no;
}

static bool
assistant_closed(GtkWidget *widget, struct ProjectionData *p) {
	g_message("assistant_closed");
	gtk_widget_destroy(p->assistant);
	return FALSE;
}

static void
assistant_canceled(GtkWidget *widget, struct ProjectionData *p) {
	g_message("assistant_canceled");
	gtk_widget_destroy(p->assistant);
}

static bool
assistant_destroyed(GtkWidget *widget, struct ProjectionData *p) {
	g_message("assistant_destroyed");
	if(p->closing_callback) {
		(*(void(*)(gpointer))p->closing_callback)(p->closing_callback_param);
	}
	projection_clean_all(p);
	gmap_free(p->next_func);
	gmap_free(p);
	return FALSE;
}

static void
assistant_apply(GtkWidget *widget, struct ProjectionData *p) {
	g_message("assistant_apply");
	projection_done(p);
	((void(*)(struct ProjectionData *, void *))p->done_callback)(p, p->done_callback_param);
	//gtk_widget_destroy(p->assistant);
}

static gint
page_forward_func(gint current_page, gpointer data) {
	struct ProjectionData *p = (struct ProjectionData *)data;
	gint val;
	g_message("page_forward_func from %d", current_page);

	val = (*p->next_func[current_page])(p);
	g_message("page_forward_func: proceed from %d to %d", current_page, val);
	return val;
}

static void
projection_assistant_prepare_page(GtkAssistant *assistant, GtkWidget *page, struct ProjectionData *p) {
	const char *s = gtk_assistant_get_page_title(assistant, page);
	g_message("prepare page='%s'", s);
	if(page == p->view_wkt_page) {
		GtkTextBuffer *buffer;
		char *wkt;
		if(p->osrs != NULL)
			OSRExportToPrettyWkt(p->osrs, &wkt, 3);
		else
			wkt = NULL;
		g_message("WKT=%.*s...", 5, wkt);
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(p->view_wkt_textview));
		g_message("Before set_text");
		gtk_text_buffer_set_text(buffer, wkt, -1);
		g_message("After set_text");
		CPLFree(wkt);
	}
	gtk_assistant_set_page_complete(GTK_ASSISTANT(p->assistant), page, TRUE);
}

static struct ProjectionData *
projection_assistant_new() {
	GtkWidget *widget;

	struct ProjectionData *p;

	p = (struct ProjectionData *)gmap_malloc(sizeof(struct ProjectionData));
	memset(p, 0, sizeof(struct ProjectionData));

	p->assistant = gtk_assistant_new();
	g_signal_connect(G_OBJECT(p->assistant), "destroy", G_CALLBACK(assistant_destroyed), p);
	g_signal_connect(G_OBJECT(p->assistant), "close", G_CALLBACK(assistant_closed), p);
	g_signal_connect(G_OBJECT(p->assistant), "cancel", G_CALLBACK(assistant_canceled), p);
	g_signal_connect(G_OBJECT(p->assistant), "apply", G_CALLBACK(assistant_apply), p);
	g_signal_connect(G_OBJECT(p->assistant), "prepare", G_CALLBACK(projection_assistant_prepare_page), p);
	gtk_assistant_set_forward_page_func(GTK_ASSISTANT(p->assistant), page_forward_func, p, NULL);

	intro_page(p);
	from_wkt_page(p);
	accept_page(p);
	spheroid_page(p);
	toWGS84_page(p);
	projection_page(p);
	view_wkt_page(p);

	/* Add a cancel button */
	widget = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_assistant_add_action_widget(GTK_ASSISTANT(p->assistant), widget);

	/* Can we remove the "last" widget ? */
	// gtk_assistant_remove_action_widget(GTK_ASSISTANT(p->assistant), GTK_ASSISTANT(p->assistant)->last);

	return p;
}

static void
projection_assistant_set_transient_parent(struct ProjectionData *p, GtkWindow *window) {
	gtk_window_set_transient_for(GTK_WINDOW(p->assistant), window);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(p->assistant), TRUE);
}

static void
projection_assistant_set_done_func(struct ProjectionData *p, char *message, GCallback func, gpointer param) {
	p->done_message = message;
	p->done_callback = func;
	p->done_callback_param = param;
}

static void
projection_assistant_set_closing_func(struct ProjectionData *p, GCallback func, gpointer param) {
	p->closing_callback = func;
	p->closing_callback_param = param;
}

static void
projection_assistant_run(struct ProjectionData *p) {
	gtk_widget_show(p->assistant);
}

static void
mapview_assistant_closing(struct MapView *mapview) {
	mapview->set_projection = NULL;
	g_message("assistant ref removed from mapview\n");
}

static void
set_mapview_projection(struct ProjectionData *p, struct MapView *mapview) {
	/* let gdal suggest... */
	GDALDatasetH fakeds;
	GdkPixbuf *fakepixbuf;
	double factor;
	int width, height;
	double extents[4];
	double GeoTransform[6];
	void *hTransformArg;
	OGRErr err;

	/*
	 * create a fake dataset which covers the current view. Use a 1X1 pixbuf. Determine the
         * correct GeoTransform. Remember division factor to use later when setting scale.
         */
	copy_geotransform(mapview->rt.GeoTransform, GeoTransform);
	GeoTransform[1] *= mapview->rt.width;
	GeoTransform[4] *= mapview->rt.width;
	GeoTransform[2] *= mapview->rt.height;
	GeoTransform[5] *= mapview->rt.height;
	factor = mapview->rt.width;

	fakepixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 1, 1);
	if(fakepixbuf == NULL) {
		g_error("failed to create fake pixbuf size %dx%d", mapview->rt.width, mapview->rt.height);
		return;
	}
	fakeds = GDALOpenPixbuf(fakepixbuf, GA_ReadOnly);

	GDALSetProjection(fakeds, mapview->rt.WKT);
	GDALSetGeoTransform(fakeds, GeoTransform);
	hTransformArg = GDALCreateGenImgProjTransformer(fakeds, mapview->rt.WKT, NULL, p->WKT, FALSE, 0, 1);
	if(hTransformArg == NULL) {
		g_error("hTransformArg == NULL");
	}
	err = GDALSuggestedWarpOutput2(fakeds, GDALGenImgProjTransform, hTransformArg, GeoTransform, &width, &height, extents, 0);
	if(err != OGRERR_NONE) {
		g_error("GDALSuggestedWarpOutput2 failed");
	}

	g_message("GDAL suggested: w=%d h=%d ext=%f %f %f %f)", width, height, extents[0], extents[1], extents[2], extents[3]);

	if(mapview->rt.WKT)
		gmap_free(mapview->rt.WKT);

	mapview->rt.WKT = gmap_strdup(p->WKT);

	mapview->rt.left = extents[0];
	mapview->rt.bottom = extents[1];
	mapview->rt.right = extents[2];
	mapview->rt.top = extents[3];

	mapview_set_scale(mapview, GeoTransform[1] / factor);
	mapview_changed_projection(mapview);
	gtk_widget_queue_draw(mapview->layout);

	GDALDestroyGenImgProjTransformer(hTransformArg);
	GDALClose(fakeds);
	g_object_unref(fakepixbuf);
}

void
do_set_projection(struct MapView *mapview) {
	struct ProjectionData *p;

	if(mapview->set_projection) {
		gtk_window_present(GTK_WINDOW(mapview->set_projection));
		return;
	}

	p = projection_assistant_new();
	projection_assistant_set_transient_parent(p, GTK_WINDOW(mapview->window));
	projection_assistant_set_done_func(p, "Apply new projection to map view", G_CALLBACK(set_mapview_projection), mapview);
	projection_assistant_set_closing_func(p, G_CALLBACK(mapview_assistant_closing), mapview);
	projection_assistant_run(p);

	mapview->set_projection = p->assistant;
}
