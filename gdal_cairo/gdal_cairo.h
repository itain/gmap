#include <gdal_pam.h>
#include <cairo/cairo.h>

CPL_C_START
void GDALRegister_gdal_cairo();
GDALDataset *GDALOpenCairo(cairo_surface_t *cairo, GDALAccess eAccess);
GDALDataset *GDALOpenCairo2(cairo_surface_t *cairo, GDALAccess eAccess, int x, int y, int w, int h);
CPL_C_END

class gdal_cairo_dataset : public GDALDataset
{
	friend class	gdal_cairo_rasterband;

	cairo_surface_t *cairo;
	unsigned char 	*data;
	int		stride;

	int		bGeoTransformSet;
	double		adfGeoTransform[6];
	char		*pszProjection;

public:
	gdal_cairo_dataset();
	~gdal_cairo_dataset();

	virtual const char *GetProjectionRef(void);
	virtual CPLErr SetProjection( const char * );

	virtual CPLErr GetGeoTransform( double * );
	virtual CPLErr SetGeoTransform( double * );

	static GDALDataset *Open(GDALOpenInfo *);
	static GDALDataset *Create( const char * pszFilename,
					int nXSize, int nYSize, int nBands,
					GDALDataType eType, char ** papszParmList );

	// Driver specific methods
	static GDALDataset *OpenCairo(cairo_surface_t *, GDALAccess eAccess = GA_ReadOnly);
	static GDALDataset *OpenCairo2(cairo_surface_t *, GDALAccess eAccess, int x, int y, int w, int h);
	cairo_surface_t *GetCairo();
};

class gdal_cairo_rasterband : public GDALRasterBand
{
	unsigned char	*data;
	int		width;
	int		stride;

	int		bNoDataSet;
	double		dfNoData;

	GDALColorTable	*poColorTable;
	GDALColorInterp	eColorInterp;

	char		*pszUnitType;
	char		**papszCategoryNames;

	double		dfOffset;
	double		dfScale;

protected:
	virtual CPLErr IReadBlock( int, int, void * );
	virtual CPLErr IWriteBlock( int, int, void * );

public:
	gdal_cairo_rasterband(gdal_cairo_dataset *, int,  unsigned char *);
	~gdal_cairo_rasterband();

	virtual double GetNoDataValue( int *pbSuccess = NULL );
	virtual CPLErr SetNoDataValue( double );

	virtual GDALColorInterp GetColorInterpretation();
	virtual CPLErr SetColorInterpretation( GDALColorInterp );

	virtual GDALColorTable *GetColorTable();
	virtual CPLErr SetColorTable( GDALColorTable * );

	virtual const char *GetUnitType();
	CPLErr SetUnitType( const char * );

	virtual char **GetCategoryNames();
	virtual CPLErr SetCategoryNames( char ** );

	virtual double GetOffset( int *pbSuccess = NULL );
	CPLErr SetOffset( double );

	virtual double GetScale( int *pbSuccess = NULL );
	CPLErr SetScale( double );
};
