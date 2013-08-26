#include "gdal_cairo.h"

#include <iostream>


gdal_cairo_dataset::gdal_cairo_dataset() {}
gdal_cairo_dataset::~gdal_cairo_dataset() {}

const char *gdal_cairo_dataset::GetProjectionRef()
{
	if( pszProjection == NULL )
		return "";
	else
		return pszProjection;
}

CPLErr gdal_cairo_dataset::SetProjection( const char *pszProjectionIn )
{
	CPLFree( pszProjection );
	pszProjection = CPLStrdup( pszProjectionIn );

	return CE_None;
}

CPLErr gdal_cairo_dataset::GetGeoTransform( double *padfGeoTransform )
{
	memcpy( padfGeoTransform, adfGeoTransform, sizeof(double) * 6 );
	if( bGeoTransformSet )
		return CE_None;
	else
		return CE_Failure;
}

CPLErr gdal_cairo_dataset::SetGeoTransform( double *padfGeoTransform )
{
	memcpy( adfGeoTransform, padfGeoTransform, sizeof(double) * 6 );
	bGeoTransformSet = TRUE;

	return CE_None;
}

GDALDataset *gdal_cairo_dataset::Open(GDALOpenInfo *poOpenInfo)
{
	char    **papszOptions;

/* -------------------------------------------------------------------- */
/*      Do we have the special filename signature for gdk_pixbuf format */
/*      description strings?                                            */
/* -------------------------------------------------------------------- */
	if( !EQUALN(poOpenInfo->pszFilename,"GDK:::",6) || poOpenInfo->fp != NULL )
		return NULL;

	papszOptions = CSLTokenizeStringComplex(poOpenInfo->pszFilename+6, ",", TRUE, FALSE );

/* -------------------------------------------------------------------- */
/*      Verify we have all required fields                              */
/* -------------------------------------------------------------------- */
	const char *pszDataPointer;
	if((pszDataPointer = CSLFetchNameValue(papszOptions, "PTR")) == NULL)
	{
		CPLError(CE_Failure, CPLE_AppDefined,
			"Missing required fieldd (shoulld be either PTR or FILENAME or both)\n"
			"Unable to access cairo surface structure." );

		CSLDestroy( papszOptions );
		return NULL;
	}

	// We don't need them anymore
	CSLDestroy( papszOptions );

// Get pointer to The user's cairo_surface_t to open
	cairo_surface_t **cairo_ptr = (cairo_surface_t **)CPLScanPointer(pszDataPointer, (int)strlen(pszDataPointer));

	if(cairo_ptr == NULL || *cairo_ptr == NULL)
	{
		CPLError( CE_Failure, CPLE_AppDefined,
			"PTR is NULL\n"
			"Unable to access cairo surface structure." );

		return NULL;
	}

	cairo_surface_t *cairo = *cairo_ptr;

	return OpenCairo(cairo);
}

GDALDataset *gdal_cairo_dataset::OpenCairo2(cairo_surface_t *cairo, GDALAccess eAccess, int x, int y, int w, int h) {
	// Check that the cairo_surface_t  struct is valid.
	unsigned char *data = cairo_image_surface_get_data (cairo);
	if(data == NULL) {
		CPLError( CE_Failure, CPLE_AppDefined,
			"Cairo surface type  not supported." );
		return NULL;
	}

	cairo_format_t format = cairo_image_surface_get_format(cairo);

	if(format != CAIRO_FORMAT_ARGB32 && format != CAIRO_FORMAT_RGB24) {
		CPLError( CE_Failure, CPLE_AppDefined,
			"Cairo surface format not supported." );
		return NULL;
	}

	if(x < 0 || x+w > cairo_image_surface_get_width(cairo) ||
	   y < 0 || y+h > cairo_image_surface_get_height(cairo)) {
		CPLError( CE_Failure, CPLE_AppDefined,
			"Cairo requested size out of range" );
		return NULL;
	}

/* -------------------------------------------------------------------- */
/*      Create the new gdal_pixbuf_dataset object.                             */
/* -------------------------------------------------------------------- */
	gdal_cairo_dataset *poDS;

	poDS = new gdal_cairo_dataset();

	poDS->cairo = cairo;
	poDS->nRasterXSize = w;
	poDS->nRasterYSize = h;
	poDS->stride = cairo_image_surface_get_stride(cairo);
	poDS->data = data + y * poDS->stride + x * 4;
	poDS->eAccess = eAccess;
	poDS->pszProjection = NULL;
	poDS->bGeoTransformSet = FALSE;

	poDS->SetBand(1, new gdal_cairo_rasterband(poDS, 1, poDS->data));

	return poDS;
}

GDALDataset *GDALOpenCairo2(cairo_surface_t *cairo, GDALAccess eAccess, int x, int y, int w, int h) {
	return gdal_cairo_dataset::OpenCairo2(cairo, eAccess, x, y, w, h);
}

GDALDataset *gdal_cairo_dataset::OpenCairo(cairo_surface_t *cairo, GDALAccess eAccess) {

	return OpenCairo2(cairo, eAccess,
				0,
				0,
				cairo_image_surface_get_width(cairo),
				cairo_image_surface_get_height(cairo));
}

GDALDataset *GDALOpenCairo(cairo_surface_t *cairo, GDALAccess eAccess) {
	return gdal_cairo_dataset::OpenCairo2(cairo, eAccess,
				0,
				0,
				cairo_image_surface_get_width(cairo),
				cairo_image_surface_get_height(cairo));
}

GDALDataset *gdal_cairo_dataset::Create(const char * pszFilename,
					int nXSize, int nYSize, int nBands,
					GDALDataType eType, char ** papszParmList ) {
	
	char    **papszOptions;

/* -------------------------------------------------------------------- */
/*      Do we have the special filename signature for cairo_surface_t format */
/*      description strings?                                            */
/* -------------------------------------------------------------------- */
	if(!EQUALN(pszFilename,"CAI:::",6))
		return NULL;

	papszOptions = CSLTokenizeStringComplex(pszFilename+6, ",", TRUE, FALSE );

/* -------------------------------------------------------------------- */
/*      Verify we have all required fields                              */
/* -------------------------------------------------------------------- */
	const char *pszDataPointer;
	if((pszDataPointer = CSLFetchNameValue( papszOptions, "PTR")) == NULL)
	{
		CPLError( CE_Failure, CPLE_AppDefined,
			"Missing required fieldd (shoulld be either PTR or FILENAME or both)\n"
			"Unable to access Gdk Pixbuf structure." );

		CSLDestroy( papszOptions );
		return NULL;
	}

	// Get pointer to The user's cairo_surface_t to open
	cairo_surface_t **cairo_ptr = (cairo_surface_t **)CPLScanPointer(pszDataPointer, (int)strlen(pszDataPointer));
	
	*cairo_ptr = cairo_image_surface_create(CAIRO_FORMAT_RGB24, nXSize, nYSize);

	return OpenCairo(*cairo_ptr);
}

cairo_surface_t *gdal_cairo_dataset::GetCairo() {
	return cairo;
}

gdal_cairo_rasterband::gdal_cairo_rasterband(gdal_cairo_dataset *poDS, int iBand, unsigned char *data)
{
	this->poDS = poDS;
	this->nBand = iBand;

	this->eAccess = poDS->GetAccess();

	eDataType = GDT_UInt32;

	nBlockXSize = poDS->GetRasterXSize();
	nBlockYSize = 1;

	this->data = data;
	this->stride = poDS->stride;
}

gdal_cairo_rasterband::~gdal_cairo_rasterband() {
	// std::cout << "~gdal_pixbuf_rasterband" << std::endl;
	FlushCache();
	// CPLFree(pszProjection);
}

CPLErr gdal_cairo_rasterband::IReadBlock(int nBlockXOff, int nBlockYOff, void * pImage)
{
	// std::cout << "IReadBlock: " << poDS << "(" << nBand << ") x: " << nBlockXOff << " y: " << nBlockYOff << std::endl;
	for( int iPixel = 0; iPixel < nBlockXSize; iPixel++ )
	{
		((GByte *) pImage)[iPixel * 4 +2] = data[nBlockYOff * stride + iPixel * 4 + 2];
		((GByte *) pImage)[iPixel * 4 +1] = data[nBlockYOff * stride + iPixel * 4 + 1];
		((GByte *) pImage)[iPixel * 4 +0] = data[nBlockYOff * stride + iPixel * 4 + 0];
		((GByte *) pImage)[iPixel * 4 +3] = data[nBlockYOff * stride + iPixel * 4 + 3];
	}

	return CE_None;
}

CPLErr gdal_cairo_rasterband::IWriteBlock(int nBlockXOff, int nBlockYOff, void * pImage)
{
	// std::cout << "IWriteBlock: " << poDS << "(" << nBand << ") x: " << nBlockXOff << " y: " << nBlockYOff << std::endl;
	for( int iPixel = 0; iPixel < nBlockXSize; iPixel++ )
	{
		data[nBlockYOff*stride + iPixel * 4 + 2] = ((GByte *) pImage)[iPixel * 4 + 2];
		data[nBlockYOff*stride + iPixel * 4 + 1] = ((GByte *) pImage)[iPixel * 4 + 1];
		data[nBlockYOff*stride + iPixel * 4 + 0] = ((GByte *) pImage)[iPixel * 4 + 0];
		data[nBlockYOff*stride + iPixel * 4 + 3] = ((GByte *) pImage)[iPixel * 4 + 3];
	}
	return CE_None;
}

double gdal_cairo_rasterband::GetNoDataValue( int *pbSuccess )
{
	if(pbSuccess)
		*pbSuccess = bNoDataSet;

	if( bNoDataSet )
		return dfNoData;
	else
		return 0.0;
}

CPLErr gdal_cairo_rasterband::SetNoDataValue(double dfNewValue)
{
	dfNoData = dfNewValue;
	bNoDataSet = TRUE;

	return CE_None;
}

GDALColorInterp gdal_cairo_rasterband::GetColorInterpretation()
{
	if( poColorTable != NULL )
		return GCI_PaletteIndex;
	else
		return eColorInterp;
}

CPLErr gdal_cairo_rasterband::SetColorInterpretation( GDALColorInterp eGCI )
{
	eColorInterp = eGCI;

	return CE_None;
}

GDALColorTable *gdal_cairo_rasterband::GetColorTable()

{
	return poColorTable;
}

CPLErr gdal_cairo_rasterband::SetColorTable( GDALColorTable *poCT )
{
	if( poColorTable != NULL )
		delete poColorTable;

	if( poCT == NULL )
		poColorTable = NULL;
	else
		poColorTable = poCT->Clone();

	return CE_None;
}

const char *gdal_cairo_rasterband::GetUnitType()
{
	if( pszUnitType == NULL )
		return "";
	else
		return pszUnitType;
}

CPLErr gdal_cairo_rasterband::SetUnitType( const char *pszNewValue )
{
	CPLFree( pszUnitType );

	if( pszNewValue == NULL )
		pszUnitType = NULL;
	else
		pszUnitType = CPLStrdup(pszNewValue);

	return CE_None;
}

char **gdal_cairo_rasterband::GetCategoryNames()
{
	return papszCategoryNames;
}

CPLErr gdal_cairo_rasterband::SetCategoryNames( char ** papszNewNames )
{
	CSLDestroy( papszCategoryNames );
	papszCategoryNames = CSLDuplicate( papszNewNames );

	return CE_None;
}

double gdal_cairo_rasterband::GetOffset( int *pbSuccess )
{
	if( pbSuccess != NULL )
		*pbSuccess = TRUE;

	return dfOffset;
}

CPLErr gdal_cairo_rasterband::SetOffset( double dfNewOffset )
{
	dfOffset = dfNewOffset;
	return CE_None;
}

double gdal_cairo_rasterband::GetScale( int *pbSuccess )
{
	if( pbSuccess != NULL )
		*pbSuccess = TRUE;

	return dfScale;
}

CPLErr gdal_cairo_rasterband::SetScale( double dfNewScale )
{
	dfScale = dfNewScale;
	return CE_None;
}

// Register te driver
void GDALRegister_gdal_cairo()
{
	GDALDriver  *poDriver;

	if( GDALGetDriverByName( "gdal_cairo" ) == NULL )
	{
		poDriver = new GDALDriver();

		poDriver->SetDescription( "gdal_cairo" );
		poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
			"Driver for in-core cairo memory surface" );
		poDriver->SetMetadataItem( GDAL_DMD_CREATIONDATATYPES,
			"Byte" );
		// poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
		//	"frmt_various.html#JDEM" );

		poDriver->pfnOpen = gdal_cairo_dataset::Open;

		GetGDALDriverManager()->RegisterDriver( poDriver );
	}
}
