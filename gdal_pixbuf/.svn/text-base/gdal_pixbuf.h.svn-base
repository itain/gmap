#include <gdal_pam.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

CPL_C_START
void GDALRegister_gdal_pixbuf();
GDALDataset *GDALOpenPixbuf(GdkPixbuf *pixbuf, GDALAccess eAccess);
GDALDataset *GDALOpenPixbuf2(GdkPixbuf *pixbuf, GDALAccess eAccess, int x, int y, int w, int h);
CPL_C_END

class gdal_pixbuf_dataset : public GDALDataset
{
	friend class	gdal_pixbuf_rasterband;

	GdkPixbuf	*pixbuf;
	guchar 		*data;
	int		stride;
	int		byteskip;

	int		bGeoTransformSet;
	double		adfGeoTransform[6];
	char		*pszProjection;

	FILE		*fp;
	GByte		abyHeader[1012];

public:
	gdal_pixbuf_dataset();
	~gdal_pixbuf_dataset();

	virtual const char *GetProjectionRef(void);
	virtual CPLErr SetProjection( const char * );

	virtual CPLErr GetGeoTransform( double * );
	virtual CPLErr SetGeoTransform( double * );

	static GDALDataset *Open(GDALOpenInfo *);
	static GDALDataset *Create( const char * pszFilename,
					int nXSize, int nYSize, int nBands,
					GDALDataType eType, char ** papszParmList );

	// Driver specific methods
	static GDALDataset *OpenPixbuf(GdkPixbuf *, GDALAccess eAccess = GA_ReadOnly);
	static GDALDataset *OpenPixbuf2(GdkPixbuf *, GDALAccess eAccess, int x, int y, int w, int h);
	GdkPixbuf *GetPixbuf();
};

class gdal_pixbuf_rasterband : public GDALRasterBand
{
	guchar		*data;
	int		width;
	int		stride;
	int		byteskip;

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
	gdal_pixbuf_rasterband(gdal_pixbuf_dataset *, int,  guchar *);
	~gdal_pixbuf_rasterband();

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
