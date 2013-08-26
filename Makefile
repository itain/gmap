#GDLDIR=./gdl-0.7.5

DEPENDS=.depends

CFLAGS   := -g -Wall -DLINUX `pkg-config --cflags gtk+-2.0 gdk-pixbuf-2.0 libxml-2.0 gdal gtkglext-1.0` -Igdal_pixbuf
LDFLAGS  := -g `pkg-config --libs gtk+-2.0 gdk-pixbuf-2.0 libxml-2.0 gdal libexif gtkglext-1.0` -lzip
CXXFLAGS := -g -DLINUX -Wall `pkg-config --cflags gtk+-2.0 gdk-pixbuf-2.0 libxml-2.0 gdal` -Igdal_pixbuf
SUBDIRS  :=  gdal_pixbuf gdal_cairo

GDAL_DRIVERS=./gdal_pixbuf/gdal_pixbuf.o ./gdal_cairo/gdal_cairo.o

GMAP_FILES=gmap_main.o mapset.o mapset_gdal.o mapfit.o gdal_utils.o \
	mapwindow.o calibrate.o affinegrid.o track.o point_gdal.o \
	waypoint.o tree.o add_action.o solid_fill.o zoom_tool.o \
	file_utils.o waypoint_symbols.o layers_box.o geo_inverse.o \
	utf8.o print.o select_region.o projection.o

#EXTRA_FILES=mapset_gui.o projection_gui.o

SRCS=$(GMAP_FILES:.o=.c)

all: subdirs gmap # xml s1

gmap: $(GMAP_FILES) $(GDAL_DRIVERS)
	$(CXX) -o gmap $(GMAP_FILES) $(LDFLAGS) $(GDAL_DRIVERS) -lm

#S1_FILES=s1.o s1_gl.o
#s1: $(S1_FILES)
#	$(CC) -o s1 $(S1_FILES) $(LDFLAGS) -lm
#
#xml: xml.o
#	$(CXX) -o xml xml.o $(LDFLAGS)

.PHONY: depend
depend $(DEPENDS): $(SRCS)
	${CC} -M $(CFLAGS) $(SRCS) > $(DEPENDS)

.PHONY: tags
tags: $(SRCS)
	ctags $(SRCS)

.PHONY: subdirs
subdirs:
	for i in $(SUBDIRS); do $(MAKE) -C $$i all; done


.PHONY: clean
clean::
	for i in $(SUBDIRS); do $(MAKE) -C $$i clean; done
	$(RM) gmap  xml *.o $(DEPENDS)

ifneq ($(wildcard $(DEPENDS)),)
#$(info Including $(DEPENDS))
include $(DEPENDS)
else
$(warning Run make depend!)
endif
