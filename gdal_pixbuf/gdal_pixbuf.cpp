#include "gdal_pixbuf.h"

#include <iostream>


gdal_pixbuf_dataset::gdal_pixbuf_dataset() {}
gdal_pixbuf_dataset::~gdal_pixbuf_dataset() {}

const char *gdal_pixbuf_dataset::GetProjectionRef()
{
	if( pszProjection == NULL )
		return "";
	else
		return pszProjection;
}

CPLErr gdal_pixbuf_dataset::SetProjection( const char *pszProjectionIn )
{
	CPLFree( pszProjection );
	pszProjection = CPLStrdup( pszProjectionIn );

	return CE_None;
}

CPLErr gdal_pixbuf_dataset::GetGeoTransform( double *padfGeoTransform )
{
	memcpy( padfGeoTransform, adfGeoTransform, sizeof(double) * 6 );
	if( bGeoTransformSet )
		return CE_None;
	else
		return CE_Failure;
}

CPLErr gdal_pixbuf_dataset::SetGeoTransform( double *padfGeoTransform )
{
	memcpy( adfGeoTransform, padfGeoTransform, sizeof(double) * 6 );
	bGeoTransformSet = TRUE;

	return CE_None;
}

GDALDataset *gdal_pixbuf_dataset::Open(GDALOpenInfo *poOpenInfo)
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
			"Missing required field (should be either PTR or FILENAME or both)\n"
			"Unable to access Gdk Pixbuf structure." );

		CSLDestroy( papszOptions );
		return NULL;
	}

	// We don't need them anymore
	CSLDestroy( papszOptions );

// Get pointer to The user's GdkPixbuf to open
	GdkPixbuf **pixbuf_ptr = (GdkPixbuf **)CPLScanPointer(pszDataPointer, (int)strlen(pszDataPointer));

	if(pixbuf_ptr == NULL || *pixbuf_ptr == NULL)
	{
		CPLError( CE_Failure, CPLE_AppDefined,
			"PTR is NULL\n"
			"Unable to access Gdk Pixbuf structure." );

		return NULL;
	}

	GdkPixbuf       *pixbuf = *pixbuf_ptr;

	return OpenPixbuf(pixbuf);
}

GDALDataset *gdal_pixbuf_dataset::OpenPixbuf2(GdkPixbuf *pixbuf, GDALAccess eAccess, int x, int y, int w, int h) {
	// Check that the puxbuf struct is valid.
	int bits_per_sample = gdk_pixbuf_get_bits_per_sample (pixbuf);
	int n_channels = gdk_pixbuf_get_n_channels (pixbuf);

	if(bits_per_sample != 8 || n_channels != 3) {
		CPLError( CE_Failure, CPLE_AppDefined,
			"Pixbuf format not supported." );
		return NULL;
	}
	if(x < 0 || x+w > gdk_pixbuf_get_width(pixbuf) ||
	   y < 0 || y+h > gdk_pixbuf_get_height(pixbuf)) {
		CPLError( CE_Failure, CPLE_AppDefined,
			"Pixbuf requested size out of range" );
		return NULL;
	}

/* -------------------------------------------------------------------- */
/*      Create the new gdal_pixbuf_dataset object.                             */
/* -------------------------------------------------------------------- */
	gdal_pixbuf_dataset *poDS;

	poDS = new gdal_pixbuf_dataset();

	poDS->pixbuf = pixbuf;
	poDS->nRasterXSize = w;
	poDS->nRasterYSize = h;
	poDS->data = gdk_pixbuf_get_pixels (pixbuf) + y * gdk_pixbuf_get_rowstride(pixbuf) + x * n_channels;
	poDS->stride = gdk_pixbuf_get_rowstride (pixbuf);
	poDS->byteskip = 3;
	poDS->eAccess = eAccess;
	poDS->pszProjection = NULL;

	for(int iBand = 0; iBand < 1; iBand++) {
		poDS->SetBand(iBand+1, 
                       new gdal_pixbuf_rasterband(poDS, iBand+1, poDS->data+iBand));
	}

	return poDS;
}

GDALDataset *GDALOpenPixbuf2(GdkPixbuf *pixbuf, GDALAccess eAccess, int x, int y, int w, int h) {
	return gdal_pixbuf_dataset::OpenPixbuf2(pixbuf, eAccess, x, y, w, h);
}

GDALDataset *gdal_pixbuf_dataset::OpenPixbuf(GdkPixbuf *pixbuf, GDALAccess eAccess) {

	return OpenPixbuf2(pixbuf, eAccess, 0, 0, gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));
}

GDALDataset *GDALOpenPixbuf(GdkPixbuf *pixbuf, GDALAccess eAccess) {
	return gdal_pixbuf_dataset::OpenPixbuf(pixbuf, eAccess);
}

GDALDataset *gdal_pixbuf_dataset::Create(const char * pszFilename,
					int nXSize, int nYSize, int nBands,
					GDALDataType eType, char ** papszParmList ) {
	
	char    **papszOptions;

/* -------------------------------------------------------------------- */
/*      Do we have the special filename signature for gdk_pixbuf format */
/*      description strings?                                            */
/* -------------------------------------------------------------------- */
	if(!EQUALN(pszFilename,"GDK:::",6))
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

// Get pointer to The user's GdkPixbuf to open

	GdkPixbuf **pixbuf_ptr = (GdkPixbuf **)CPLScanPointer(pszDataPointer, (int)strlen(pszDataPointer));
	
	*pixbuf_ptr = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, nXSize, nYSize);

	return OpenPixbuf(*pixbuf_ptr);
}

GdkPixbuf *gdal_pixbuf_dataset::GetPixbuf() {
	return pixbuf;
}

gdal_pixbuf_rasterband::gdal_pixbuf_rasterband(gdal_pixbuf_dataset *poDS, int iBand, guchar *data)
{
	this->poDS = poDS;
	this->nBand = iBand;

	this->eAccess = poDS->GetAccess();

	eDataType = GDT_UInt32;

	nBlockXSize = poDS->GetRasterXSize();
	nBlockYSize = 1;

	this->data = data;
	this->stride = poDS->stride;
	this->byteskip = poDS->byteskip;
}

gdal_pixbuf_rasterband::~gdal_pixbuf_rasterband() {
	// std::cout << "~gdal_pixbuf_rasterband" << std::endl;
	FlushCache();
	// CPLFree(pszProjection);
}

CPLErr gdal_pixbuf_rasterband::IReadBlock(int nBlockXOff, int nBlockYOff, void * pImage)
{
	// std::cout << "IReadBlock: " << poDS << "(" << nBand << ") x: " << nBlockXOff << " y: " << nBlockYOff << std::endl;
	for( int iPixel = 0; iPixel < nBlockXSize; iPixel++ )
	{
		((GByte *) pImage)[iPixel * 4 +2] = data[nBlockYOff * stride + iPixel * byteskip + 0];
		((GByte *) pImage)[iPixel * 4 +1] = data[nBlockYOff * stride + iPixel * byteskip + 1];
		((GByte *) pImage)[iPixel * 4 +0] = data[nBlockYOff * stride + iPixel * byteskip + 2];
		((GByte *) pImage)[iPixel * 4 +3] = 0x00;
	}

	return CE_None;
}

CPLErr gdal_pixbuf_rasterband::IWriteBlock(int nBlockXOff, int nBlockYOff, void * pImage)
{
	// std::cout << "IWriteBlock: " << poDS << "(" << nBand << ") x: " << nBlockXOff << " y: " << nBlockYOff << std::endl;
	for( int iPixel = 0; iPixel < nBlockXSize; iPixel++ )
	{
		data[nBlockYOff*stride + iPixel * byteskip + 0] = ((GByte *) pImage)[iPixel * 4 + 2];
		data[nBlockYOff*stride + iPixel * byteskip + 1] = ((GByte *) pImage)[iPixel * 4 + 1];
		data[nBlockYOff*stride + iPixel * byteskip + 2] = ((GByte *) pImage)[iPixel * 4 + 0];
	}
	return CE_None;
}

double gdal_pixbuf_rasterband::GetNoDataValue( int *pbSuccess )
{
	if(pbSuccess)
		*pbSuccess = bNoDataSet;

	if( bNoDataSet )
		return dfNoData;
	else
		return 0.0;
}

CPLErr gdal_pixbuf_rasterband::SetNoDataValue(double dfNewValue)
{
	dfNoData = dfNewValue;
	bNoDataSet = TRUE;

	return CE_None;
}

GDALColorInterp gdal_pixbuf_rasterband::GetColorInterpretation()
{
	if( poColorTable != NULL )
		return GCI_PaletteIndex;
	else
		return eColorInterp;
}

CPLErr gdal_pixbuf_rasterband::SetColorInterpretation( GDALColorInterp eGCI )
{
	eColorInterp = eGCI;

	return CE_None;
}

GDALColorTable *gdal_pixbuf_rasterband::GetColorTable()

{
	return poColorTable;
}

CPLErr gdal_pixbuf_rasterband::SetColorTable( GDALColorTable *poCT )
{
	if( poColorTable != NULL )
		delete poColorTable;

	if( poCT == NULL )
		poColorTable = NULL;
	else
		poColorTable = poCT->Clone();

	return CE_None;
}

const char *gdal_pixbuf_rasterband::GetUnitType()
{
	if( pszUnitType == NULL )
		return "";
	else
		return pszUnitType;
}

CPLErr gdal_pixbuf_rasterband::SetUnitType( const char *pszNewValue )
{
	CPLFree( pszUnitType );

	if( pszNewValue == NULL )
		pszUnitType = NULL;
	else
		pszUnitType = CPLStrdup(pszNewValue);

	return CE_None;
}

char **gdal_pixbuf_rasterband::GetCategoryNames()
{
	return papszCategoryNames;
}

CPLErr gdal_pixbuf_rasterband::SetCategoryNames( char ** papszNewNames )
{
	CSLDestroy( papszCategoryNames );
	papszCategoryNames = CSLDuplicate( papszNewNames );

	return CE_None;
}

double gdal_pixbuf_rasterband::GetOffset( int *pbSuccess )
{
	if( pbSuccess != NULL )
		*pbSuccess = TRUE;

	return dfOffset;
}

CPLErr gdal_pixbuf_rasterband::SetOffset( double dfNewOffset )
{
	dfOffset = dfNewOffset;
	return CE_None;
}

double gdal_pixbuf_rasterband::GetScale( int *pbSuccess )
{
	if( pbSuccess != NULL )
		*pbSuccess = TRUE;

	return dfScale;
}

CPLErr gdal_pixbuf_rasterband::SetScale( double dfNewScale )
{
	dfScale = dfNewScale;
	return CE_None;
}

// Register te driver
void GDALRegister_gdal_pixbuf()
{
	GDALDriver  *poDriver;

	if( GDALGetDriverByName( "gdal_pixbuf" ) == NULL )
	{
		poDriver = new GDALDriver();

		poDriver->SetDescription( "gdal_pixbuf" );
		poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
			"Driver for in-core gdk-pixbuf" );
		poDriver->SetMetadataItem( GDAL_DMD_CREATIONDATATYPES,
			"Byte" );
		// poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
		//	"frmt_various.html#JDEM" );

		poDriver->pfnOpen = gdal_pixbuf_dataset::Open;

		GetGDALDriverManager()->RegisterDriver( poDriver );
	}
}
