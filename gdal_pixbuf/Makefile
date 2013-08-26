CXXFLAGS= -g `pkg-config --cflags gtk+-2.0 gdk-pixbuf-2.0 gdal`
LDFLAGS= -g `pkg-config --libs gtk+-2.0 gdk-pixbuf-2.0 gdal`

all: gdal_pixbuf.o

gdal_pixbuf.o: gdal_pixbuf.cpp gdal_pixbuf.h
	$(CXX) -c gdal_pixbuf.cpp $(CXXFLAGS)

clean::
	$(RM) *.o

