/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*-
 * $Id$
 *
 * XASTIR, Amateur Station Tracking and Information Reporting
 * Copyright (C) 2003  The Xastir Group
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Look at the README for more information on the program.
 *
 *
 */


//
// NOTE:  GDAL sample data can be found here:
//
//          ftp://ftp.remotesensing.org/gdal/data
// or here: http://gdal.maptools.org/dl/data/
//


#include "config.h"
#include "snprintf.h"

//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <sys/stat.h>
//#include <ctype.h>
//#include <sys/types.h>
//#include <pwd.h>
//#include <errno.h>

// Needed for Solaris
//#include <strings.h>

//#include <dirent.h>
//#include <netinet/in.h>
//#include <Xm/XmAll.h>

//#ifdef HAVE_X11_XPM_H
//#include <X11/xpm.h>
//#ifdef HAVE_LIBXPM // if we have both, prefer the extra library
//#undef HAVE_XM_XPMI_H
//#endif // HAVE_LIBXPM
//#endif // HAVE_X11_XPM_H

//#ifdef HAVE_XM_XPMI_H
//#include <Xm/XpmI.h>
//#endif // HAVE_XM_XPMI_H

//#include <X11/Xlib.h>

//#include <math.h>

#include "xastir.h"
#include "maps.h"
#include "alert.h"
//#include "util.h"
#include "main.h"
//#include "datum.h"
//#include "draw_symbols.h"
//#include "rotated.h"
//#include "color.h"
//#include "xa_config.h"


#define CHECKMALLOC(m)  if (!m) { fprintf(stderr, "***** Malloc Failed *****\n"); exit(0); }



#ifdef HAVE_LIBGDAL

//#warning
//#warning
//#warning GDAL/OGR library support is not fully implemented yet!
//#warning Preliminary TIGER/Line, Shapefile, mid/mif/tab (MapInfo),
//#warning and SDTS support is functional, including on-the-fly
//#warning coordinate conversion for both indexing and drawing.
//#warning
//#warning

// Getting rid of stupid compiler warnings in GDAL
#define XASTIR_PACKAGE_BUGREPORT PACKAGE_BUGREPORT
#undef PACKAGE_BUGREPORT
#define XASTIR_PACKAGE_NAME PACKAGE_NAME
#undef PACKAGE_NAME
#define XASTIR_PACKAGE_STRING PACKAGE_STRING
#undef PACKAGE_STRING
#define XASTIR_PACKAGE_TARNAME PACKAGE_TARNAME
#undef PACKAGE_TARNAME
#define XASTIR_PACKAGE_VERSION PACKAGE_VERSION
#undef PACKAGE_VERSION

#include "gdal.h"
#include "ogr_api.h"
#include "ogr_srs_api.h"
#include "cpl_string.h"

#undef PACKAGE_BUGREPORT
#define PACKAGE_BUGREPORT XASTIR_PACKAGE_BUGREPORT
#undef XASTIR_PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#define PACKAGE_NAME XASTIR_PACKAGE_NAME
#undef XASTIR_PACKAGE_NAME
#undef PACKAGE_STRING
#define PACKAGE_STRING XASTIR_PACKAGE_STRING
#undef XASTIR_PACKAGE_STRING
#undef PACKAGE_TARNAME
#define PACKAGE_TARNAME XASTIR_PACKAGE_TARNAME
#undef XASTIR_PACKAGE_TARNAME
#undef PACKAGE_VERSION
#define PACKAGE_VERSION XASTIR_PACKAGE_VERSION
#undef XASTIR_PACKAGE_VERSION
#endif  // HAVE_LIBGDAL





void map_gdal_init() {

#ifdef HAVE_LIBGDAL

    GDALDriverH  gDriver;   // GDAL driver
    OGRSFDriverH oDriver;   // OGR driver
    int ii, jj;


    // Register all known GDAL drivers
    GDALAllRegister();

    // Register all known OGR drivers
    OGRRegisterAll();

    // Print out each supported GDAL driver.
    //
    ii = GDALGetDriverCount();

    fprintf(stderr,"\nGDAL Registered Drivers: (not implemented yet)\n");
    for (jj = 0; jj < ii; jj++) {
        gDriver = GDALGetDriver(jj);
        fprintf(stderr,"%10s   %s\n",
            GDALGetDriverShortName(gDriver),
            GDALGetDriverLongName(gDriver) );
    }
    fprintf(stderr,"\n");


    // Print out each supported OGR driver
    //
    ii = OGRGetDriverCount();

    fprintf(stderr,"OGR Registered Drivers: (not implemented yet)\n");
    for  (jj = 0; jj < ii; jj++) {
        oDriver = OGRGetDriver(jj);
        fprintf(stderr,"%10s   %s\n",
            "",
            OGR_Dr_GetName(oDriver) );
    }
    fprintf(stderr,"\n");


#endif  // HAVE_LIBGDAL

}





/*  TODO: 
 *    projections
 *    multiple bands 
 *    colormaps 
 *    .geo files vs. .wld
 *    map index
 */


#ifdef HAVE_LIBGDAL

void draw_gdal_map(Widget w,
                   char *dir,
                   char *filenm,
                   alert_entry *alert,
                   u_char alert_color,
                   int destination_pixmap,
                   int draw_filled) {

    GDALDatasetH hDataset;
    char file[MAX_FILENAME];    // Complete path/name of image
    GDALDriverH   hDriver;
    double adfGeoTransform[6];
//    double map_x_mind, map_x_maxd, map_dxd; // decimal degrees
//    double map_y_mind, map_y_maxd, map_dyd; // decimal degrees
    unsigned long map_x_min, map_x_max; // xastir units
    unsigned long map_y_min, map_y_max; // xastir units
    long map_dx, map_dy; // xastir units
    double scr_m_dx, scr_m_dy; // screen pixels per map pixel
    unsigned long map_s_x_min, map_s_x_max, map_s_x; // map pixels
//    unsigned long map_s_y_min, map_s_y_max, map_s_y; // map pixels
    unsigned long scr_s_x_min, scr_s_x_max, scr_s_x; // screen pixels
//    unsigned long scr_s_y_min, scr_s_y_max, scr_s_y; // screen pixels
        
    // 
    GDALRasterBandH hBand = NULL;
    int numBand, hBandc;
    int nBlockXSize, nBlockYSize;
    //    int bGotMin, bGotMax;
    //    double adfMinMax[2];
    //
    const char *pszProjection;
    float *pafScanline;
    int nXSize;
    unsigned int width,height;
    GDALColorTableH hColorTable;
    GDALColorInterp hColorInterp;
    GDALPaletteInterp hPalInterp;
    GDALColorEntry colEntry;
    GDALDataType hType;
    double dfNoData;
    int bGotNodata;
    int i;

    xastir_snprintf(file, sizeof(file), "%s/%s", dir, filenm);

    if ( debug_level & 16 ) 
        fprintf(stderr,"Calling GDALOpen on file: %s\n", file);

    hDataset = GDALOpenShared( file, GA_ReadOnly );

    if( hDataset == NULL ) {
        fprintf(stderr,"GDAL couldn't open file: %s\n", file);
    }


    if ( debug_level & 16 ) {
        hDriver = GDALGetDatasetDriver( hDataset );
        fprintf( stderr, "Driver: %s/%s\n",
                 GDALGetDriverShortName( hDriver ),
                 GDALGetDriverLongName( hDriver ) );
        
        fprintf( stderr,"Size is %dx%dx%d\n",
                 GDALGetRasterXSize( hDataset ), 
                 GDALGetRasterYSize( hDataset ),
                 GDALGetRasterCount( hDataset ) );
    
        if( GDALGetProjectionRef( hDataset ) != NULL )
            fprintf( stderr,"Projection is `%s'\n", GDALGetProjectionRef( hDataset ) );
    
        if( GDALGetGeoTransform( hDataset, adfGeoTransform ) == CE_None ) {
                fprintf( stderr,"Origin = (%.6f,%.6f)\n",
                         adfGeoTransform[0], adfGeoTransform[3] );
            
                fprintf( stderr,"Pixel Size = (%.6f,%.6f)\n",
                         adfGeoTransform[1], adfGeoTransform[5] );
            }
    }
    pszProjection = GDALGetProjectionRef( hDataset );
    GDALGetGeoTransform( hDataset, adfGeoTransform );    
    numBand = GDALGetRasterCount( hDataset );
    width = GDALGetRasterXSize( hDataset );
    height = GDALGetRasterYSize( hDataset );
    // will have to delay these calcs until after a proj call
    map_x_min = 64800000l + (360000.0 * adfGeoTransform[0]);
    map_x_max = 64800000l + (360000.0 * (adfGeoTransform[0] + adfGeoTransform[1] * width));
    map_y_min = 32400000l + (360000.0 * (-adfGeoTransform[3] ));
    map_y_max = 32400000l + (360000.0 * (-adfGeoTransform[3] + adfGeoTransform[5] * height));
    map_dx = adfGeoTransform[1] * 360000.0;
    map_dy = adfGeoTransform[5] * 360000.0;
    scr_m_dx = scale_x / map_dx;
    scr_m_dy = scale_y / map_dy;

    /*
     * Here are the corners of our viewport, using the Xastir
     * coordinate system.  Notice that Y is upside down:
     *
     *   left edge of view = x_long_offset
     *  right edge of view = x_long_offset + (screen_width  * scale_x)
     *    top edge of view =  y_lat_offset
     * bottom edge of view =  y_lat_offset + (screen_height * scale_y)
     *
     * The corners of our map:
     *
     *   left edge of map = map_x_min   in Xastir format
     *  right edge of map = map_x_max
     *    top edge of map = map_y_min
     * bottom edge of map = map_y_max
     * 
     * Map scale in xastir units per pixel: map_dx and map_dy
     * Map scale in screen pixels per map pixel: scr_m_dx and scr_m_dy
     *
     * map pixel offsets that are on screen:
     *   left edge of map = map_s_x_min
     *  right edge of map = map_s_x_max
     *    top edge of map = map_s_y_min
     * bottom edge of map = map_s_y_max
     * 
     * screen pixel offsets for map edges:
     *   left edge of map = scr_s_x_min
     *  right edge of map = scr_s_x_max
     *    top edge of map = scr_s_y_min
     * bottom edge of map = scr_s_y_max
     * 
     */


// Setting some variables that haven't been set yet.
map_s_x_min = 0;
scr_s_x_min = 0;


    // calculate map range on screen
    scr_s_x_max = map_s_x_max = 0ul;
    for ( map_s_x = 0, scr_s_x = 0; map_s_x_min < width; map_s_x_min++, scr_s_x_min += scr_m_dx) {
        if ((x_long_offset + (scr_s_x_min * scale_x)) > (map_x_min + (map_s_x_min * map_dx))) 
            map_s_x_min = map_s_x;
        
        
    }

    for (hBandc = 0; hBandc < numBand; hBandc++) {
        hBand = GDALGetRasterBand( hDataset, hBandc + 1 );
        dfNoData = GDALGetRasterNoDataValue( hBand, &bGotNodata );
        GDALGetBlockSize( hBand, &nBlockXSize, &nBlockYSize );
        hType = GDALGetRasterDataType(hBand);
        
        if ( debug_level & 16 ) {
            fprintf(stderr, "Band %d (/%d):\n", hBandc, numBand); 
            fprintf( stderr, " Block=%dx%d Type=%s.\n",
                     nBlockXSize, nBlockYSize,
                     GDALGetDataTypeName(hType) );
            if( GDALGetDescription( hBand ) != NULL 
                && strlen(GDALGetDescription( hBand )) > 0 )
                fprintf(stderr, " Description = %s\n", GDALGetDescription(hBand) );
            if( strlen(GDALGetRasterUnitType(hBand)) > 0 )
                fprintf(stderr, " Unit Type: %s\n", GDALGetRasterUnitType(hBand) );
            if( bGotNodata )
                fprintf(stderr, " NoData Value=%g\n", dfNoData );
        }
        // handle colormap stuff

        hColorInterp = GDALGetRasterColorInterpretation ( hBand );
        if ( debug_level & 16 ) 
            fprintf ( stderr, " Band's Color is interpreted as %s.\n", 
                      GDALGetColorInterpretationName (hColorInterp));

        if( GDALGetRasterColorInterpretation(hBand) == GCI_PaletteIndex ) {

            hColorTable = GDALGetRasterColorTable( hBand );
            hPalInterp = GDALGetPaletteInterpretation ( hColorTable );

            if ( debug_level & 16 ) 
                fprintf( stderr, " Band has a color table (%s) with %d entries.\n", 
                         GDALGetPaletteInterpretationName( hPalInterp),
                         GDALGetColorEntryCount( hColorTable ) );

            for( i = 0; i < GDALGetColorEntryCount( hColorTable ); i++ ) {
                    GDALGetColorEntryAsRGB( hColorTable, i, &colEntry );
                    fprintf( stderr, "  %3d: %d,%d,%d,%d\n", 
                             i, 
                             colEntry.c1,
                             colEntry.c2,
                             colEntry.c3,
                             colEntry.c4 );
                }
        } // PaletteIndex
        

    }
    //

        if ( numBand == 1) { // single band

        }
        if ( numBand == 3) { // seperate RGB bands
        
        }




    
    nXSize = GDALGetRasterBandXSize( hBand );

    pafScanline = (float *) malloc(sizeof(float)*nXSize);
    GDALRasterIO( hBand, GF_Read, 0, 0, nXSize, 1, 
                  pafScanline, nXSize, 1, GDT_Float32, 
                  0, 0 );

    /* The raster is */






    GDALClose(hDataset);
}



/////////////////////////////////////////////////////////////////////
// Above this point is GDAL code, below is OGR code.
/////////////////////////////////////////////////////////////////////



// Draw_OGR_Points().
//
// A function which can be recursively called.  Tracks the recursion
// depth so that we can recover if we exceed the maximum.  If we
// keep finding geometries below us, keep calling the same function.
// Simple and efficient.
//
// Really the important thing here is not to draw a tiny dot on the
// screen, but to draw a symbol and/or a label at that point on the
// screen.  We'll need more code to handle that stuff though, to
// determine which field in the file to use as a label and the
// font/color/placement/size/etc.
// 
void Draw_OGR_Points(OGRGeometryH geometryH,
        int level,
        OGRCoordinateTransformationH transformH) {
 
    int kk;
    int object_num = 0;


    //fprintf(stderr, "Draw_OGR_Points\n");

    if (geometryH == NULL)
        return; // Exit early

    // Check for more objects below this one, recursing into any
    // objects found.  "level" keeps us from recursing too far (we
    // don't want infinite recursion here).
    // 
    object_num = OGR_G_GetGeometryCount(geometryH);
    // Iterate through the objects found.  If another geometry is
    // detected, call this function again recursively.  That will
    // cause all of the lower objects to get drawn.
    //
    if (object_num) {

        //fprintf(stderr, "DrawPoints: Found %d geometries\n", object_num);
 
        for ( kk = 0; kk < object_num; kk++ ) {
            OGRGeometryH child_geometryH;
            int sub_object_num;


            // See if we have geometries below this one.
            child_geometryH = OGR_G_GetGeometryRef(geometryH, kk);

            sub_object_num = OGR_G_GetGeometryCount(child_geometryH);

            if (sub_object_num) {
                // We found geometries below this.  Recurse.
                if (level < 5) {
                    //fprintf(stderr, "DrawPoints: Recursing level %d\n", level);
                    Draw_OGR_Points(child_geometryH,
                        level+1,
                        transformH);
                }
            }
        }
    }
    else {  // Draw
        int num_points;
        int ii;


        // Get number of elements (points)
        num_points = OGR_G_GetPointCount(geometryH);
        //fprintf(stderr,"  Number of elements: %d\n",num_points);

        // Draw the points
        for ( ii = 0; ii < num_points; ii++ ) {
            double X1, Y1, Z1;


            // Get the point!
            OGR_G_GetPoint(geometryH,
                ii,
                &X1,
                &Y1,
                &Z1);

            if (transformH) {
                // Convert to WGS84 coordinates.
                if (!OCTTransform(transformH, 1, &X1, &Y1, &Z1)) {
                    fprintf(stderr,
                        "Couldn't convert point to WGS84\n");
                    // Draw it anyway.  It _might_ be in WGS84 or
                    // NAD83!
                }
            }

            // Skip the map_visible_lat_lon() check:
            // draw_point_ll() does the check for us.
            //
            draw_point_ll(da,
                (float)Y1,
                (float)X1,
                gc,
                pixmap);
        }
    }
}

 



// Draw_OGR_Lines().
//
// A function which can be recursively called.  Tracks the recursion
// depth so that we can recover if we exceed the maximum.  If we
// keep finding geometries below us, keep calling the same function.
// Simple and efficient.
//
// In the case of !fast_extents, it might be faster to convert the
// entire object to Xastir coordinates, then check whether it is
// visible.  That would elimate two conversions in the case that the
// object gets drawn.  For the fast_extents case, we're just
// snagging the extents instead of the entire object, so we might
// just leave it as-is.
//
void Draw_OGR_Lines(OGRGeometryH geometryH,
        int level,
        OGRCoordinateTransformationH transformH,
        int fast_extents) {
 
    int kk;
    int object_num = 0;


    //fprintf(stderr, "Draw_OGR_Lines\n");

    if (geometryH == NULL)
        return; // Exit early

    // Check for more objects below this one, recursing into any
    // objects found.  "level" keeps us from recursing too far (we
    // don't want infinite recursion here).
    // 
    object_num = OGR_G_GetGeometryCount(geometryH);

    // Iterate through the objects found.  If another geometry is
    // detected, call this function again recursively.  That will
    // cause all of the lower objects to get drawn.
    //
    if (object_num) {

        //fprintf(stderr, "DrawLines: Found %d geometries\n", object_num);
 
        for ( kk = 0; kk < object_num; kk++ ) {
            OGRGeometryH child_geometryH;
            int sub_object_num;


            // Check for more geometries below this one.
            child_geometryH = OGR_G_GetGeometryRef(geometryH, kk);

            sub_object_num = OGR_G_GetGeometryCount(child_geometryH);

            if (sub_object_num) {
                // We found geometries below this.  Recurse.
                if (level < 5) {
                    //fprintf(stderr, "DrawLines: Recursing level %d\n", level);
                    Draw_OGR_Lines(child_geometryH,
                        level+1,
                        transformH,
                        fast_extents);
                }
            }
        }
    }
    else {  // Just draw it.  Standard linestring files return 0 for
            // object_num.  Multilinestring must return more, so we
            // recurse?  Yet to be tested!
        int num_points;
        OGREnvelope envelopeH;


        // If we have fast_extents available, take advantage of it
        // to escape this routine as quickly as possible, should the
        // object not be within our view.
        //
        if (fast_extents) {

            // Get the extents for this Polyline
            OGR_G_GetEnvelope(geometryH, &envelopeH);

            // Convert them to WGS84/Geographic coordinate system
            if (transformH) {
                // Convert to WGS84 coordinates.
                if (!OCTTransform(transformH, 2, &envelopeH.MinX, &envelopeH.MinY, NULL)) {
                    fprintf(stderr,
                        "Couldn't convert point to WGS84\n");
                }
            }

/*
fprintf(stderr,"MinY:%f, MaxY:%f, MinX:%f, MaxX:%f\n",
    envelopeH.MinY,
    envelopeH.MaxY,
    envelopeH.MinX,
    envelopeH.MaxX);
*/

            if (map_visible_lat_lon( envelopeH.MinY,    // bottom
                    envelopeH.MaxY, // top
                    envelopeH.MinX, // left
                    envelopeH.MaxX, // right
                    NULL)) {
                //fprintf(stderr, "Fast Extents: Line is visible\n");
            }
            else {
                //fprintf(stderr, "Fast Extents: Line is NOT visible\n");
                return; // Exit early
            }

            // If we made it this far with an object with
            // fast_extents , the feature is within our view.
        }
        else {  // Fast extents are not available.  Compute the
                // extents ourselves once we have all the points,
                // then check whether this object is within our
                // view.
        }

        num_points = OGR_G_GetPointCount(geometryH);
        //fprintf(stderr,"  Number of elements: %d\n",num_points);

        // Draw one polyline
        if (num_points > 0) {
            int ii;
            double *vectorX;
            double *vectorY;
            double *vectorZ;
            unsigned long *XL = NULL;
            unsigned long *YL = NULL;
            long *XI = NULL;
            long *YI = NULL;
            XPoint *xpoints = NULL;


            // Get some memory to hold the vectors
            vectorX = (double *)malloc(sizeof(double) * num_points);
            vectorY = (double *)malloc(sizeof(double) * num_points);
            vectorZ = (double *)malloc(sizeof(double) * num_points);

            // Get the points, fill in the vectors
            for ( ii = 0; ii < num_points; ii++ ) {

                OGR_G_GetPoint(geometryH,
                    ii,
                    &vectorX[ii],
                    &vectorY[ii],
                    &vectorZ[ii]);
            }

            if (transformH) {
                // Convert all vectors to WGS84 coordinates.
                if (!OCTTransform(transformH, num_points, vectorX, vectorY, vectorZ)) {
                    fprintf(stderr,
                        "Couldn't convert vectors to WGS84\n");
                }
            }

            // For the non-fast_extents case, we need to compute the
            // extents and check whether this object is within our
            // view.
            //
            if (!fast_extents) {
                double MinX, MaxX, MinY, MaxY;

                MinX = vectorX[0];
                MaxX = vectorX[0];
                MinY = vectorY[0];
                MaxY = vectorY[0];

                for ( ii = 1; ii < num_points; ii++ ) {
                    if (vectorX[ii] < MinX) MinX = vectorX[ii];
                    if (vectorX[ii] > MaxX) MaxX = vectorX[ii];
                    if (vectorY[ii] < MinY) MinY = vectorY[ii];
                    if (vectorY[ii] > MaxY) MaxY = vectorY[ii];
                }

                // We have the extents now in geographic/WGS84
                // datum.  Check for visibility.
                //
                if (map_visible_lat_lon( MinY,    // bottom
                        MaxY, // top
                        MinX, // left
                        MaxX, // right
                        NULL)) {
                    //fprintf(stderr, "Line is visible\n");
                }
                else {

                    //fprintf(stderr, "Line is NOT visible\n");

                    // Free the allocated vector memory
                    if (vectorX)
                        free(vectorX);
                    if (vectorY)
                        free(vectorY);
                    if (vectorZ)
                        free(vectorZ);

                    return; // Exit early
                }
            }

            // If we've gotten to this point, part or all of the
            // object is within our view so at least some drawing
            // should occur.

            XL = (unsigned long *)malloc(sizeof(unsigned long) * num_points);
            YL = (unsigned long *)malloc(sizeof(unsigned long) * num_points);

            // Convert arrays to the Xastir coordinate system
            for (ii = 0; ii < num_points; ii++) {
                convert_to_xastir_coordinates(&XL[ii],
                    &YL[ii],
                    vectorX[ii],
                    vectorY[ii]);
            }

            // Free the allocated vector memory
            if (vectorX)
                free(vectorX);
            if (vectorY)
                free(vectorY);
            if (vectorZ)
                free(vectorZ);

            XI = (long *)malloc(sizeof(long) * num_points);
            YI = (long *)malloc(sizeof(long) * num_points);

            // Convert arrays to screen coordinates.  Careful here!
            // The format conversions you'll need if you try to
            // compress this into two lines will get you into
            // trouble.
            //
            // We also clip to screen size and compute min/max
            // values here.
            for (ii = 0; ii < num_points; ii++) {
                XI[ii] = XL[ii] - x_long_offset;
                XI[ii] = XI[ii] / scale_x;

                YI[ii] = YL[ii] - y_lat_offset;
                YI[ii] = YI[ii] / scale_y;

// Here we truncate:  We should polygon clip instead, so that the
// slopes of the line segments don't change.  Points beyond +/-
// 16000 can cause problems in X11 when we draw.
                if      (XI[ii] > 1700l) XI[ii] = 1700l;
                else if (XI[ii] <    0l) XI[ii] =    0l;
                if      (YI[ii] > 1700l) YI[ii] = 1700l;
                else if (YI[ii] <    0l) YI[ii] =    0l;
            }

            // We don't need the Xastir coordinate system arrays
            // anymore.  We've already converted to screen
            // coordinates.
            if (XL)
                free(XL);
            if (YL)
                free(YL);

            // Set up the XPoint array.
            xpoints = (XPoint *)malloc(sizeof(XPoint) * num_points);

            // Load up our xpoints array
            for (ii = 0; ii < num_points; ii++) {
                xpoints[ii].x = (short)XI[ii];
                xpoints[ii].y = (short)YI[ii];
            }

            // Free the screen coordinate arrays.
            if (XI)
                free(XI);
            if (YI)
                free(YI);

            // Actually draw the lines
            (void)XDrawLines(XtDisplay(da),
                pixmap,
                gc,
                xpoints,
                num_points,
                CoordModeOrigin);

            if (xpoints)
                free(xpoints);
        }
    }
}





// create_clip_mask()
//
// Create a rectangular X11 Region.  It should be the size of the
// extents for the outer polygon.
//
Region create_clip_mask(int num, int minX, int minY, int maxX, int maxY) {
    XRectangle rectangle;
    Region region;


//    fprintf(stderr,"Create mask:");

    rectangle.x      = (short) minX;
    rectangle.y      = (short) minY;
    rectangle.width  = (unsigned short)(maxX - minX + 1);
    rectangle.height = (unsigned short)(maxY - minY + 1);

    // Create an empty region
    region = XCreateRegion();

/*
    fprintf(stderr,"Create: x:%d y:%d x:%d y:%d\n",
        minX,
        minY,
        maxX,
        maxY);

    fprintf(stderr,"Create: x:%d y:%d w:%d h:%d\n",
        rectangle.x,
        rectangle.y,
        rectangle.width,
        rectangle.height);
*/

    // Create a region containing a filled rectangle
    XUnionRectWithRegion(&rectangle,
        region,
        region);

    return(region);
}


 


// create_hole_in_mask()
//
// Create a hole in an X11 Region, using a polygon as input.  X11
// Region must have been created with "create_clip_mask()" before
// this function is called.
//
Region create_hole_in_mask(Region mask,
        int num,
        long *X,
        long *Y) {

    Region region2 = NULL;
    Region region3 = NULL;
    XPoint *xpoints = NULL;
    int ii;


//    fprintf(stderr,"Hole:");

    if (num < 3) {  // Not enough for a polygon
        fprintf(stderr,
            "create_hole_in_mask:XPolygonRegion w/too few vertices: %d\n",
            num);
        return(mask);    // Net result = no change to Region
    }

    // Get memory to hold the points
    xpoints = (XPoint *)malloc(sizeof(XPoint) * num);

    // Load up our xpoints array
    for (ii = 0; ii < num; ii++) {
        xpoints[ii].x = (short)X[ii];
        xpoints[ii].y = (short)Y[ii];
    }

    // Create empty regions
    region2 = XCreateRegion();
    region3 = XCreateRegion();

    // Draw the "hole" polygon
    region2 = XPolygonRegion(xpoints,
        num,
        WindingRule);

    // Free the allocated memory
    if (xpoints)
        free(xpoints);

    // Subtract region2 from mask and put the result into region3.
    XSubtractRegion(mask, region2, region3);

    // Get rid of the two regions we no longer need.
    XDestroyRegion(mask);
    XDestroyRegion(region2);

    // Return our new region that has another hole in it.
    return(region3);
}





// draw_polygon_with_mask()
//
// Draws a polygon onto a pixmap using an X11 Region to mask areas
// that shouldn't be drawn over (holes).  X11 Region is created with
// the "create_clip_mask()" function, holes in it are created with
// the "create_hole_in_mask()" function.  The polygon used to create
// the initial X11 Region was saved away during the creation.  Now
// we just draw it to the pixmap using the X11 Region as a mask.
//
void draw_polygon_with_mask(Region mask,
        XPoint *xpoints,
        int num_points) {

    GC gc_temp = NULL;
    XGCValues gc_temp_values;


//    fprintf(stderr,"Draw w/mask:");

    // There were "hole" polygons, so by now we've created a "holey"
    // region.  Draw a filled polygon with gc_temp here and then get
    // rid of gc_temp and the regions.

    gc_temp = XCreateGC(XtDisplay(da),
        XtWindow(da),
        0,
        &gc_temp_values);

//WE7U
// Hard-coded drawing attributes
(void)XSetLineAttributes (XtDisplay(da), gc_temp, 0, LineSolid, CapButt,JoinMiter);
//(void)XSetForeground(XtDisplay(da), gc_temp, colors[(int)0x08]);  // black
//(void)XSetForeground(XtDisplay(da), gc_temp, colors[(int)0x1a]);  // Steel Blue
(void)XSetForeground(XtDisplay(da), gc_temp, colors[(int)0x0e]);  // yellow

    // Set the clip-mask into the GC.  This GC is now ruined for
    // other purposes, so destroy it when we're done drawing this
    // one shape.
    if (mask != NULL)
        XSetRegion(XtDisplay(da), gc_temp, mask);

    // Actually draw the filled polygon
    (void)XFillPolygon(XtDisplay(da),
        pixmap,
        gc_temp,
        xpoints,
        num_points,
        Nonconvex,
        CoordModeOrigin);

    if (mask != NULL)
        XDestroyRegion(mask);

    if (gc_temp != NULL)
        XFreeGC(XtDisplay(da), gc_temp);

//    fprintf(stderr,"Done!\n");
}





// Draw_OGR_Polygons().
//
// A function which can be recursively called.  Tracks the recursion
// depth so that we can recover if we exceed the maximum.  If we
// keep finding geometries below us, keep calling the same function.
// Simple and efficient.
// 
// This can get complicated for Shapefiles:  Polygons are composed
// of multiple rings.  If a ring goes in one direction, it's a fill
// polygon, if the other direction, it's a hole polygon.
//
// Can test the operation using Shapefiles for Island County, WA,
// Camano Island, where there's an Island in Puget Sound that has a
// lake with an island in it (48.15569N/122.48953W)!
//
// At 48.43867N/122.61687W there's another lake with an island in
// it, but this lake isn't on an island like the first example.
//
// A note from Frank Warmerdam (GDAL guy):
//
// "An OGRPolygon objects consists of one outer ring and zero or
// more inner rings.  The inner rings are holes.  The C API kind of
// hides this fact, but you should be able to assume that the first
// ring is an outer ring, and any other rings are inner rings
// (holes).  Note that in the simple features geometry model you
// can't make any assumptions about the winding direction of holes
// or outer rings.
//
// Some polygons when read from Shapefiles will be returned as
// OGRMultiPolygon.  These are basically areas that have multiple
// outer rings.  For instance, if you had one feature that was a
// chain of islands.  In the past (and likely even now with some
// format drivers) these were just returned as polygons with the
// outer and inner rings all mixed up.  But the development code
// includes fixes (for shapefiles at least) to fix this up and
// convert into a multi polygon so you can properly establish what
// are outer rings, and what are inner rings (holes)."
//
// In the case of !fast_extents, it might be faster to convert the
// entire object to Xastir coordinates, then check whether it is
// visible.  That would elimate two conversions in the case that the
// object gets drawn.  For the fast_extents case, we're just
// snagging the extents instead of the entire object, so we might
// just leave it as-is.
//
void Draw_OGR_Polygons(OGRGeometryH geometryH,
        int level,
        OGRCoordinateTransformationH transformH,
        int draw_filled,
        int fast_extents) {
 
    int kk;
    int object_num = 0;
    Region mask = NULL;
    XPoint *xpoints = NULL;
    int num_outer_points = 0;


    if (geometryH == NULL)
        return; // Exit early

    //fprintf(stderr,"Draw_OGR_Polygons\n");

    // Check for more objects below this one, recursing into any
    // objects found.  "level" keeps us from recursing too far (we
    // don't want infinite recursion here).  These objects may be
    // rings or they may be other polygons in a collection.
    // 
    object_num = OGR_G_GetGeometryCount(geometryH);

    // Iterate through the objects found.  If another geometry is
    // detected, call this function again recursively.  That will
    // cause all of the lower objects to get drawn.
    //
    for ( kk = 0; kk < object_num; kk++ ) {
        OGRGeometryH child_geometryH;
        int sub_object_num;


        // This may be a ring, or another object with rings.
        child_geometryH = OGR_G_GetGeometryRef(geometryH, kk);

        sub_object_num = OGR_G_GetGeometryCount(child_geometryH);

        if (sub_object_num) {
            // We found geometries below this.  Recurse.
            if (level < 5) {

                // If we got here, we're dealing with a multipolygon
                // file.  There are multiple sets of polygon
                // objects, each may have inner (hole) and outer
                // (fill) rings.  If (level > 0), it's a
                // multipolygon layer.

                //fprintf(stderr, "DrawPolygons: Recursing level %d\n", level);

                Draw_OGR_Polygons(child_geometryH,
                    level+1,
                    transformH,
                    draw_filled,
                    fast_extents);
            }
        }

        else {  // Draw
            int polygon_points;
            OGREnvelope envelopeH;
            int polygon_hole = 0;
            int single_polygon = 0;


            // if (kk==0) we're dealing with an outer (fill) ring.
            // If (kk>0) we're dealing with an inner (hole) ring.
            if (kk == 0 || object_num == 1) {
                if (object_num == 1) {
                    //fprintf(stderr,"Polygon->Fill\n");
                    single_polygon++;
                }
                else {
                    //fprintf(stderr,"Polygon->Fill w/holes\n");
                }
                
            }
            else if (object_num > 1) {
                //fprintf(stderr,"Polygon->Hole\n");
                polygon_hole++;
            }

            if (fast_extents) {

                // Get the extents for this Polygon
                OGR_G_GetEnvelope(geometryH, &envelopeH);

                // Convert them to WGS84/Geographic coordinate system
                if (transformH) {
                    // Convert to WGS84 coordinates.
                    if (!OCTTransform(transformH, 2, &envelopeH.MinX, &envelopeH.MinY, NULL)) {
                        fprintf(stderr,
                            "Couldn't convert to WGS84\n");
                    }
                }

                if (map_visible_lat_lon( envelopeH.MinY,    // bottom
                        envelopeH.MaxY, // top
                        envelopeH.MinX, // left
                        envelopeH.MaxX, // right
                        NULL)) {
                    //fprintf(stderr, "Fast Extents: Polygon is visible\n");
                }
                else {
                    //fprintf(stderr, "Fast Extents: Polygon is NOT visible\n");
                    return; // Exit early
                }

                // If we made it this far, the feature is within our
                // view (if it has fast_extents).
            }
            else {
                // Fast extents not available.  We'll need to
                // compute our own extents below.
            }

            polygon_points = OGR_G_GetPointCount(child_geometryH);
            //fprintf(stderr, "Vertices: %d\n", polygon_points);

            if (polygon_points > 3) { // Not a polygon if < 3 points
                int mm;
                double *vectorX;
                double *vectorY;
                double *vectorZ;


                // Get some memory to hold the polygon vectors
                vectorX = (double *)malloc(sizeof(double) * polygon_points);
                vectorY = (double *)malloc(sizeof(double) * polygon_points);
                vectorZ = (double *)malloc(sizeof(double) * polygon_points);

                // Get the points, fill in the vectors
                for ( mm = 0; mm < polygon_points; mm++ ) {

                    OGR_G_GetPoint(child_geometryH,
                        mm,
                        &vectorX[mm],
                        &vectorY[mm],
                        &vectorZ[mm]);
                }

                if (transformH) {
                    // Convert entire polygon to WGS84 coordinates.
                    if (!OCTTransform(transformH, polygon_points, vectorX, vectorY, vectorZ)) {
//                        fprintf(stderr,
//                            "Couldn't convert polygon to WGS84\n");
//return;
                    }
                }

                // For the non-fast_extents case, we need to compute
                // the extents and check whether this object is
                // within our view.
                //
                if (!fast_extents) {
                    double MinX, MaxX, MinY, MaxY;

                    MinX = vectorX[0];
                    MaxX = vectorX[0];
                    MinY = vectorY[0];
                    MaxY = vectorY[0];

                    for ( mm = 1; mm < polygon_points; mm++ ) {
                        if (vectorX[mm] < MinX) MinX = vectorX[mm];
                        if (vectorX[mm] > MaxX) MaxX = vectorX[mm];
                        if (vectorY[mm] < MinY) MinY = vectorY[mm];
                        if (vectorY[mm] > MaxY) MaxY = vectorY[mm];
                    }

                    // We have the extents now in geographic/WGS84
                    // datum.  Check for visibility.
                    //
                    if (map_visible_lat_lon( MinY,    // bottom
                            MaxY, // top
                            MinX, // left
                            MaxX, // right
                            NULL)) {
                        //fprintf(stderr, "Polygon is visible\n");
                    }
                    else {

                        //fprintf(stderr, "Polygon is NOT visible\n");

                        // Free the allocated vector memory
                        if (vectorX)
                            free(vectorX);
                        if (vectorY)
                            free(vectorY);
                        if (vectorZ)
                            free(vectorZ);

                        //return; // Exit early
                        continue;   // Next ieration of the loop
                    }
                }

                // If draw_filled != 0, draw the polygon using X11
                // polygon calls instead of just drawing the border.
                //
                if (draw_filled) { // Draw a filled polygon
                    unsigned long *XL = NULL;
                    unsigned long *YL = NULL;
                    long *XI = NULL;
                    long *YI = NULL;
                    int nn;
                    int minX, maxX, minY, maxY;


                    XL = (unsigned long *)malloc(sizeof(unsigned long) * polygon_points);
                    YL = (unsigned long *)malloc(sizeof(unsigned long) * polygon_points);
 
                    // Convert arrays to the Xastir coordinate
                    // system
                    for (nn = 0; nn < polygon_points; nn++) {
                        convert_to_xastir_coordinates(&XL[nn],
                            &YL[nn],
                            vectorX[nn],
                            vectorY[nn]);
                    }

                    XI = (long *)malloc(sizeof(long) * polygon_points);
                    YI = (long *)malloc(sizeof(long) * polygon_points);
 
// Note:  We're limiting screen size to 1700 in this routine.
                    minX = 1700;
                    maxX = 0;
                    minY = 1700;
                    maxY = 0;
 
                    // Convert arrays to screen coordinates.
                    // Careful here!  The format conversions you'll
                    // need if you try to compress this into two
                    // lines will get you into trouble.
                    //
                    // We also clip to screen size and compute
                    // min/max values here.
                    for (nn = 0; nn < polygon_points; nn++) {
                        XI[nn] = XL[nn] - x_long_offset;
                        XI[nn] = XI[nn] / scale_x;

                        YI[nn] = YL[nn] - y_lat_offset;
                        YI[nn] = YI[nn] / scale_y;
 
// Here we truncate:  We should polygon clip instead, so that the
// slopes of the line segments don't change.  Points beyond +/-
// 16000 can cause problems in X11 when we draw.  Here we are more
// interested in keeping the rectangles small and fast.  Screen-size
// or smaller basically.
                        if      (XI[nn] > 1700l) XI[nn] = 1700l;
                        else if (XI[nn] <    0l) XI[nn] =    0l;
                        if      (YI[nn] > 1700l) YI[nn] = 1700l;
                        else if (YI[nn] <    0l) YI[nn] =    0l;

                        if (!polygon_hole) {

                            // Find the min/max extents for the
                            // arrays.  We use that to set the size
                            // of our mask region.
                            if (XI[nn] < minX) minX = XI[nn];
                            if (XI[nn] > maxX) maxX = XI[nn];
                            if (YI[nn] < minY) minY = YI[nn];
                            if (YI[nn] > maxY) maxY = YI[nn];
                        }
                    }

                    // We don't need the Xastir coordinate system
                    // arrays anymore.  We've already converted to
                    // screen coordinates.
                    if (XL)
                        free(XL);
                    if (YL)
                        free(YL);

                    if (!polygon_hole) {    // Outer ring (fill)
                        int pp;

                        // Set up the XPoint array that we'll need
                        // for our final draw (after the "holey"
                        // region is set up if we have multiple
                        // rings).
                        xpoints = (XPoint *)malloc(sizeof(XPoint) * polygon_points);

                        // Load up our xpoints array
                        for (pp = 0; pp < polygon_points; pp++) {
                            xpoints[pp].x = (short)XI[pp];
                            xpoints[pp].y = (short)YI[pp];
                        }
                        num_outer_points = polygon_points;
                    }

                    // If we have no inner polygons, skip the whole
                    // X11 Region thing and just draw a filled
                    // polygon to the pixmap for speed.  We do that
                    // here via the "single_polygon" variable.
                    //
                    if (!polygon_hole) {   // Outer ring (fill)

                        if (single_polygon) {
                            mask = NULL;
                        }
                        else {
                            // Pass the extents of the polygon to
                            // create a mask rectangle out of them.
                            mask = create_clip_mask(polygon_points,
                                minX,
                                minY,
                                maxX,
                                maxY);
                        }
                    }
                    else {  // Inner ring (hole)

                        // Pass the entire "hole" polygon set of
                        // vertices into the hole region creation
                        // function.  This knocks a hole in our mask
                        // so that underlying map layers can show
                        // through.

                        mask = create_hole_in_mask(mask,
                            polygon_points,
                            XI,
                            YI);
                    }

                    // Free the screen coordinate arrays.
                    if (XI)
                        free(XI);
                    if (YI)
                        free(YI);

                    // Draw the original polygon to the pixmap
                    // using the X11 Region as a mask.  Mask may be
                    // null if we're doing a single outer polygon
                    // with no "hole" polygons.
                    if (kk == (object_num - 1)) {
                        draw_polygon_with_mask(mask,
                            xpoints,
                            num_outer_points);

                        free(xpoints);
                    }
                }   // end of draw_filled
                else {  // We're drawing non-filled polygons.
                        // Draw just the border.

                    if (polygon_hole) {
                        // Inner ring, draw a dashed line
                        (void)XSetLineAttributes (XtDisplay(da), gc, 0, LineOnOffDash, CapButt,JoinMiter);
                    }
                    else {
                        // Outer ring, draw a solid line
                        (void)XSetLineAttributes (XtDisplay(da), gc, 0, LineSolid, CapButt,JoinMiter);
                    }

                    for ( mm = 1; mm < polygon_points; mm++ ) {

                        draw_vector_ll(da,
                            (float)vectorY[mm-1],
                            (float)vectorX[mm-1],
                            (float)vectorY[mm],
                            (float)vectorX[mm],
                            gc,
                            pixmap);
                    }
                }

// For weather polygons, we might want to draw the border color in a
// different color so that we can see these borders easily, or skip
// drawing the border itself for a few pixels, like UI-View does.

                // Free the allocated vector memory
                if (vectorX)
                    free(vectorX);
                if (vectorY)
                    free(vectorY);
                if (vectorZ)
                    free(vectorZ);
 
            }
            else {
                // Not enough points to draw a polygon
            }
        }
    }
}

 



// The GDAL docs say to use these flags to compile:
// `gdal-config --libs` `gdal-config * --cflags`
// but so far they return: "-L/usr/local/lib -lgdal" and
// "-I/usr/local/include", which aren't much.  Leaving it hard-coded
// in configure.ac for now instead of using "gdal-config".
//
// GRASS module that does OGR (for reference):  v.in.ogr
//
// This function started out as very nearly verbatim the example
// C-API code off the OGR web pages.
//
// If destination_pixmap equals INDEX_CHECK_TIMESTAMPS or
// INDEX_NO_TIMESTAMPS, then we are indexing the file (finding the
// extents) instead of drawing it.
//
// Indexing/drawing works properly for either geographic or
// projected coordinates, and does conversions to geographic/WGS84
// datum before storing the extents in the map index.  It does not
// work for local coordinate systems.
//
//
// TODO:
// *) map_visible_ll() needs to be re-checked.  Lines are not
//    showing up sometimes when the end points are way outside the
//    view.  Perhaps need line_visible() and line_visible_ll()
//    functions?  If I don't want to compute the slope of a line and
//    such, might just check for line ends being within boundaries
//    or left/above, and within boundaries or right/below, to catch
//    those lines that slant across the view.
// *) Dbfawk support or similar?  Map preferences:  Colors, line
//    widths, layering, filled, choose label field, label
//    fonts/placement/color.
// *) Allow user to select layers to draw/ignore.  Very important
//    for those sorts of files that provide all layers (i.e. Tiger &
//    SDTS).
// *) Weather alert tinted polygons, draw to the correct pixmap.
// *) Fast Extents:  We now pass a variable to the draw functions
//    that tells whether we can do fast extents, but we still need
//    to compute our own extents after we have the points for a
//    shape in an array.  We could either check extents or just call
//    the draw functions, which skip drawing if outside our view
//    anyway (we currently do the latter).
// *) Figure out why SDTS hypsography (contour lines) on top of
//    terraserver gives strange results when zooming/panning
//    sometimes.  Restarting Xastir cleans up the image.  Perhaps
//    colors or GC's are getting messed up?  Perhaps lines segments
//    are too long when drawing?
// *) Some sort of warning to the operator if large regions are
//    being filled, and there's a base raster map?  Dbfawk likes to
//    fill counties in with purple, wiping out rasters underneath.
// *) Draw portions of a line/polygon correctly, instead of just
//    dropping or concatenating lines.  This can cause missing line
//    segments that cross the edge of our view, or incorrect slopes
//    for lines that cross the edge.
// *) Speed things up in any way possible.
//
// 
// Regarding coordinate systems, a note from Frank Warmerdam:  "In
// theory different layers in a datasource can have different
// coordinate systems, though this is uncommon.  It does happen
// though."
//
// Question regarding how to specify drawing preferences, answered
// by Frank:  "There are many standards.  Within the OGR API the
// preferred approach to this is the OGR Feature Style
// Specification:
//
// http://gdal.velocet.ca/~warmerda/projects/opengis/ogr_feature_style_008.html
//
// Basically this is intended to provide a mechanism for different
// underlying formats to indicate their rendering information.  It
// is at least partially implemented for the DGN and Mapinfo
// drivers.  The only applications that I am aware of taking
// advantage of it are OpenEV and MapServer."

void draw_ogr_map(Widget w,
                   char *dir,
                   char *filenm,
                   alert_entry *alert,
                   u_char alert_color,
                   int destination_pixmap,
                   int draw_filled) {

    OGRDataSourceH datasourceH = NULL;
    OGRSFDriverH driver = NULL;
    OGRSpatialReferenceH map_spatialH = NULL;
    OGRSpatialReferenceH wgs84_spatialH = NULL;
    OGRCoordinateTransformationH transformH = NULL;
    int i, numLayers;
    char full_filename[300];
    const char *ptr;
    int no_spatial = 1;
    int geographic = 0;
    int projected = 0;
    int local = 0;
    const char *datum = NULL;
    const char *geogcs = NULL;


    if (debug_level & 16)
        fprintf(stderr,"Entered draw_ogr_map function\n");

    xastir_snprintf(full_filename,
        sizeof(full_filename),
        "%s/%s",
        dir,
        filenm);

    if (debug_level & 16)
        fprintf(stderr,"Opening datasource %s\n", full_filename);

    //
    // One computer segfaulted at OGROpen() if a .prj file was
    // present with a shapefile.  Another system with newer Linux/
    // libtiff/ libgeotiff works fine.  Getting rid of older header
    // files/ libraries, then recompiling/installing libproj/
    // libgeotiff/ libshape/ libgdal/ Xastir seemed to fix it.
    //
    // Open data source
    datasourceH = OGROpen(full_filename,
        0 /* bUpdate */,
        &driver);

    if (datasourceH == NULL) {
/*
        fprintf(stderr,
            "Unable to open %s\n",
            full_filename);
*/
        return;
    }

    if (debug_level & 16)
        fprintf(stderr,"Opened datasource\n");

    ptr = OGR_Dr_GetName(driver);
    fprintf(stderr,"%s: ", ptr);

    // Get name/path.  Less than useful since we should already know
    // this.
    ptr = OGR_DS_GetName(datasourceH);
    fprintf(stderr,"%s\n", ptr);

    // Set up coordinate translation:  We need it for indexing and
    // drawing so we do it first and keep a pointer to our
    // transform.
    //
    if (OGR_DS_GetLayerCount(datasourceH) > 0) {
        // We have at least one layer.
        OGRLayerH layer;


        // Snag a pointer to the first layer so that we can get the
        // spatial info from it, if it exists.
        //
        layer = OGR_DS_GetLayer( datasourceH, 0 );

        // Query the coordinate system for the dataset.
        map_spatialH = OGR_L_GetSpatialRef(layer);

        if (map_spatialH) {
            const char *temp;


            // We found spatial data
            no_spatial = 0;

            if (OSRIsGeographic(map_spatialH)) {
                geographic++;
            }
            else if (OSRIsProjected(map_spatialH)) {
                projected++;
            }
            else {
                local++;
            }

            // PROJCS, GEOGCS, DATUM, SPHEROID, PROJECTION
            //
            if (projected) {
                temp = OSRGetAttrValue(map_spatialH, "PROJCS", 0);
                temp = OSRGetAttrValue(map_spatialH, "PROJECTION", 0);
            }
            temp = OSRGetAttrValue(map_spatialH, "SPHEROID", 0);
            datum = OSRGetAttrValue(map_spatialH, "DATUM", 0);
            geogcs = OSRGetAttrValue(map_spatialH, "GEOGCS", 0);
        }
        else {

            fprintf(stderr,"  Couldn't get spatial reference\n");

            // For this case, assume that it is WGS84/geographic,
            // and attempt to index as-is.  If the numbers don't
            // make sense, we can skip indexing this dataset.

            // Perhaps some layers may have a spatial reference, and
            // others might not?  Solved this problem by defined
            // "no_spatial", which will be '1' if no spatial data
            // was found in any of the layers.  In that case we just
            // store the extents we find.
        }

        if ( no_spatial         // No spatial info found, or
                || (geographic  // Geographic and correct datum
                  && ( strcasecmp(geogcs,"WGS84") == 0
                    || strcasecmp(geogcs,"NAD83") == 0
                    || strcasecmp(datum,"North_American_Datum_1983") == 0
                    || strcasecmp(datum,"World_Geodetic_System_1984") == 0
                    || strcasecmp(datum,"D_North_American_1983") ) ) ) {

// We also need to check "DATUM", as some datasets have nothing in
// the "GEOGCS" variable.  Check for "North_American_Datum_1983" or
// "???".

            transformH = NULL;  // No transform needed
            fprintf(stderr,
                "  Geographic coordinate system, NO CONVERSION NEEDED!, %s\n",
                geogcs);
        }
        else {  // We have coordinates but they're in the wrong
                // datum or in a projected/local coordinate system.
                // Set up a transform to WGS84.

            if (geographic) {
                fprintf(stderr,
                    "  Found geographic/wrong datum: %s.  Converting to wgs84 datum\n",
                    geogcs);
fprintf(stderr, "  DATUM: %s\n", datum);
            }
            else if (projected) {
                fprintf(stderr,
                    "  Found projected coordinates: %s.  Converting to geographic/wgs84 datum\n",
                    geogcs);
            }
            else if (local) {
                // Convert to geographic/WGS84?  How?

                fprintf(stderr,
                    "  Found local coordinate system.  Returning\n");

                // Close data source
                if (datasourceH != NULL) {
                    OGR_DS_Destroy( datasourceH );
                }

                return; // Exit early
            }
            else {
                // Abandon all hope, ye who enter here!  We don't
                // have a geographic, projected, or local coordinate
                // system.

                // Close data source
                if (datasourceH != NULL) {
                    OGR_DS_Destroy( datasourceH );
                }

                return; // Exit early
            }
 
            wgs84_spatialH = OSRNewSpatialReference(NULL);

            if (wgs84_spatialH == NULL) {
                fprintf(stderr,
                    "Couldn't create empty wgs84_spatialH object\n");

                // Close data source
                if (datasourceH != NULL) {
                    OGR_DS_Destroy( datasourceH );
                }

                return; // Exit early
            }

            if (OSRSetWellKnownGeogCS(wgs84_spatialH,"WGS84") == OGRERR_FAILURE) {
 
                // Couldn't fill in WGS84 parameters.
                fprintf(stderr,
                    "Couldn't fill in wgs84 spatial reference parameters\n");

                // NOTE: DO NOT destroy map_spatialH!  It is part of
                // the datasource.  You'll get a segfault if you
                // try, at the point where you destroy the
                // datasource.

                if (wgs84_spatialH != NULL) {
                    OSRDestroySpatialReference(wgs84_spatialH);
                }

                // Close data source
                if (datasourceH != NULL) {
                    OGR_DS_Destroy( datasourceH );
                }

                return; // Exit early
            }

            if (map_spatialH == NULL || wgs84_spatialH == NULL) {
                fprintf(stderr,
                    "Couldn't transform because map_spatialH or wgs84_spatialH are NULL\n");

                if (wgs84_spatialH != NULL) {
                    OSRDestroySpatialReference(wgs84_spatialH);
                }

                // Close data source
                if (datasourceH != NULL) {
                    OGR_DS_Destroy( datasourceH );
                }

                return; // Exit early
            }
            else {
                // Set up transformation from original datum to
                // wgs84 datum.
                transformH = OCTNewCoordinateTransformation(
                    map_spatialH, wgs84_spatialH);

                if (transformH == NULL) {
                    // Couldn't create transformation object
                    fprintf(stderr,
                        "Couldn't create transformation object\n");

                    if (wgs84_spatialH != NULL) {
                        OSRDestroySpatialReference(wgs84_spatialH);
                    }

                    // Close data source
                    if (datasourceH != NULL) {
                        OGR_DS_Destroy( datasourceH );
                    }

                    return; // Exit early
                }
            }
        }
    }


    // Implement the indexing functions, so that we can use these
    // map formats from within Xastir.  Without an index, it'll
    // never appear in the map chooser.  Use the OGR "envelope"
    // functions to get the extents for the each layer in turn.
    // We'll find the min/max of all and use that for the extents
    // for the entire dataset.
    //
    // Check whether we're indexing or drawing the map
    if ( (destination_pixmap == INDEX_CHECK_TIMESTAMPS)
            || (destination_pixmap == INDEX_NO_TIMESTAMPS) ) {

/////////////////////////////////////////////////////////////////////
// We're indexing only.  Save the extents in the index.
/////////////////////////////////////////////////////////////////////

        char status_text[MAX_FILENAME];
        double file_MinX = 0;
        double file_MaxX = 0;
        double file_MinY = 0;
        double file_MaxY = 0;
        int first_extents = 1;


        xastir_snprintf(status_text,
            sizeof(status_text),
            langcode ("BBARSTA039"),
            filenm);
        statusline(status_text,0);       // Indexing ...

        fprintf(stderr,"Indexing %s\n", filenm);

        // Use the OGR "envelope" function to get the extents for
        // the entire file or dataset.  Remember that it could be in
        // state-plane coordinate system or something else equally
        // strange.  Convert it to WGS84 or NAD83 geographic
        // coordinates before saving the index.

        // Loop through all layers in the data source.  We can't get
        // the extents for the entire data source without looping
        // through the layers.
        //
        numLayers = OGR_DS_GetLayerCount(datasourceH);
        for ( i=0; i<numLayers; i++ ) {
            OGRLayerH layer;
            OGREnvelope psExtent;  


            layer = OGR_DS_GetLayer( datasourceH, i );
            if (layer == NULL) {
                fprintf(stderr,
                    "Unable to open layer %d of %s\n",
                    i,
                    full_filename);

                if (wgs84_spatialH != NULL) {
                    OSRDestroySpatialReference(wgs84_spatialH);
                }

                if (transformH != NULL) {
                    OCTDestroyCoordinateTransformation(transformH);
                }

                // Close data source
                if (datasourceH != NULL) {
                    OGR_DS_Destroy( datasourceH );
                }

                return; // Exit early
            }

            // Get the extents for this layer.  OGRERR_FAILURE means
            // that the layer has no spatial info or that it would be
            // an expensive operation to establish the extent.
            // Here we set the force option to TRUE in order to read
            // all of the extents even for files where that would be
            // an expensive operation.  We're trying to index the
            // file after all!
            if (OGR_L_GetExtent(layer, &psExtent, TRUE) != OGRERR_FAILURE) {

//                fprintf(stderr,
//                    "  MinX: %f, MaxX: %f, MinY: %f, MaxY: %f\n",
//                    psExtent.MinX,
//                    psExtent.MaxX,
//                    psExtent.MinY,
//                    psExtent.MaxY);

                // If first value, store it.
                if (first_extents) {
                    file_MinX = psExtent.MinX;
                    file_MaxX = psExtent.MaxX;
                    file_MinY = psExtent.MinY;
                    file_MaxY = psExtent.MaxY;

                    first_extents = 0;
                }
                else {
                    // Store the min/max values
                    if (psExtent.MinX < file_MinX)
                        file_MinX = psExtent.MinX;
                    if (psExtent.MaxX > file_MaxX)
                        file_MaxX = psExtent.MaxX;
                    if (psExtent.MinY < file_MinY)
                        file_MinY = psExtent.MinY;
                    if (psExtent.MaxY > file_MaxY)
                        file_MaxY = psExtent.MaxY;
                }
            }
            // No need to free layer handle, it belongs to the
            // datasource.
        }
        // All done looping through the layers.


        // We only know how to handle geographic or projected
        // coordinate systems.  Test for these.
        if ( !first_extents && (geographic || projected || no_spatial) ) {
            // Need to also check datum!  Must be NAD83 or WGS84 and
            // geographic for our purposes.
            if ( no_spatial
                || (geographic
                    && ( strcasecmp(geogcs,"WGS84") == 0
                        || strcasecmp(geogcs,"NAD83") == 0) ) ) {
// Also check "datum" here

                fprintf(stderr,
                    "Geographic coordinate system, %s, adding to index\n",
                    geogcs);

                if (   file_MinY >=  -90.0 && file_MinY <  90.0
                    && file_MaxY >=  -90.0 && file_MaxY <  90.0
                    && file_MinX >= -180.0 && file_MinX < 180.0
                    && file_MaxX >= -180.0 && file_MaxX < 180.0) {

// Debug:  Don't add them to the index so that we can experiment
// with datum translation and such.
//#define WE7U
#ifndef WE7U
                    index_update_ll(filenm,    // Filename only
                        file_MinY,  // Bottom
                        file_MaxY,  // Top
                        file_MinX,  // Left
                        file_MaxX); // Right
#endif  // WE7U
                }
                else {
                    fprintf(stderr,
                        "Geographic coordinates out of bounds, skipping indexing\n");
                }
            }
            else {  // We have coordinates but they're in the wrong
                    // datum or in a projected coordinate system.
                    // Convert to WGS84 coordinates.
                double x[2];
                double y[2];


                x[0] = file_MinX;
                x[1] = file_MaxX;
                y[0] = file_MinY;
                y[1] = file_MaxY;

                fprintf(stderr,"Before: %f,%f\t%f,%f\n",
                    x[0],y[0],
                    x[1],y[1]);

                if (OCTTransform(transformH, 2, x, y, NULL)) {
                            
                    fprintf(stderr," After: %f,%f\t%f,%f\n",
                        x[0],y[0],
                        x[1],y[1]);
 
// Debug:  Don't add them to the index so that we can experiment
// with datum translation and such.
#ifndef WE7U
                    index_update_ll(filenm, // Filename only
                        y[0],  // Bottom
                        y[1],  // Top
                        x[0],  // Left
                        x[1]); // Right
#endif  // WE7U
                }
            }
        }

        // Debug code:
        // For now, set the index to be the entire world to get
        // things up and running.
//        index_update_ll(filenm,    // Filename only
//             -90.0,  // Bottom
//              90.0,  // Top
//            -180.0,  // Left
//             180.0); // Right

        if (wgs84_spatialH != NULL) {
            OSRDestroySpatialReference(wgs84_spatialH);
        }

        if (transformH != NULL) {
            OCTDestroyCoordinateTransformation(transformH);
        }

        // Close data source
        if (datasourceH != NULL) {
            OGR_DS_Destroy( datasourceH );
        }

        return; // Done indexing the file
    }

/////////////////////////////////////////////////////////////////////
// The code below this point is for drawing, not indexing.
/////////////////////////////////////////////////////////////////////
 
    // Find out what type of file we're dealing with.  This reports
    // one of:
    //
    // "AVCbin"
    // "DGN"
    // "FMEObjects Gateway"
    // "GML"
    // "Memory"
    // "MapInfo File"
    // "UK .NTF"
    // "OCI"
    // "ODBC"
    // "OGDI"
    // "PostgreSQL"
    // "REC"
    // "S57"
    // "SDTS"
    // "ESRI Shapefile"
    // "TIGER"
    // "VRT"
    //


    // If we're going to write, we need to test the capability using
    // these functions:
    // OGR_Dr_TestCapability(); // Does Driver have write capability?
    // OGR_DS_TestCapability(); // Can we create new layers?


// Optimization:  Get the envelope for each layer if it's not an
// expensive operation.  Skip the layer if it's completely outside
// our viewport.
 


//(void)XSetFunction();
(void)XSetFillStyle(XtDisplay(w), gc, FillSolid);



    // Loop through all layers in the data source.
    //
    numLayers = OGR_DS_GetLayerCount(datasourceH);
    for ( i=0; i<numLayers; i++ ) {
        OGRLayerH layer;
//        int jj;
//        int numFields;
        OGRFeatureH featureH;
//        OGRFeatureDefnH layerDefn;
        OGREnvelope psExtent;  
        int extents_found = 0;
        char geometry_type_name[50] = "";
        int geometry_type = -1;
        int fast_extents = 0;
 

        HandlePendingEvents(app_context);
        if (interrupt_drawing_now) {

            if (wgs84_spatialH != NULL) {
                OSRDestroySpatialReference(wgs84_spatialH);
            }

            if (transformH != NULL) {
                OCTDestroyCoordinateTransformation(transformH);
            }

            // Close data source
            if (datasourceH != NULL) {
                OGR_DS_Destroy( datasourceH );
            }

            return; // Exit early
        }

        layer = OGR_DS_GetLayer( datasourceH, i );
        if (layer == NULL) {
            fprintf(stderr,
                "Unable to open layer %d of %s\n",
                i,
                full_filename);

            if (wgs84_spatialH != NULL) {
                OSRDestroySpatialReference(wgs84_spatialH);
            }

            if (transformH != NULL) {
                OCTDestroyCoordinateTransformation(transformH);
            }

            // Close data source
            if (datasourceH != NULL) {
                OGR_DS_Destroy( datasourceH );
            }

            return; // Exit early
        }


        // Test the capabilities of the layer to know the best way
        // to access it:
        //
        //   OLCRandomRead: TRUE if the OGR_L_GetFeature() function works
        //   for this layer.
        //   NOTE: Tiger and Shapefile report this as TRUE.
        //
        //   OLCFastSpatialFilter: TRUE if this layer implements spatial
        //   filtering efficiently.
        //
        //   OLCFastFeatureCount: TRUE if this layer can return a feature
        //   count (via OGR_L_GetFeatureCount()) efficiently.  In some cases
        //   this will return TRUE until a spatial filter is installed after
        //   which it will return FALSE.
        //   NOTE: Tiger and Shapefile report this as TRUE.
        //
        //   OLCFastGetExtent: TRUE if this layer can return its data extent
        //   (via OGR_L_GetExtent()) efficiently ... ie. without scanning
        //   all the features. In some cases this will return TRUE until a
        //   spatial filter is installed after which it will return FALSE.
        //   NOTE: Shapefile reports this as TRUE.
        //
        if (i == 0) {   // First layer
            fprintf(stderr, "  ");
            if (OGR_L_TestCapability(layer, OLCRandomRead)) {
                fprintf(stderr, "Random Read, ");
            }
            if (OGR_L_TestCapability(layer, OLCFastSpatialFilter)) {
                fprintf(stderr,
                    "Fast Spatial Filter, ");
            }
            if (OGR_L_TestCapability(layer, OLCFastFeatureCount)) {
                fprintf(stderr,
                    "Fast Feature Count, ");
            }
            if (OGR_L_TestCapability(layer, OLCFastGetExtent)) {
                fprintf(stderr,
                    "Fast Get Extent, ");
                // Save this away and decide whether to
                // request/compute extents based on this.
                fast_extents = 1;
            }
        }


        if (map_spatialH) {
            const char *temp;
            int geographic = 0;
            int projected = 0;


            if (OSRIsGeographic(map_spatialH)) {
                if (i == 0)
                    fprintf(stderr,"  Geographic Coord, ");
                geographic++;
            }
            else if (OSRIsProjected(map_spatialH)) {
                if (i == 0)
                    fprintf(stderr,"  Projected Coord, ");
                projected++;
            }
            else {
                if (i == 0)
                    fprintf(stderr,"  Local Coord, ");
            }

            // PROJCS, GEOGCS, DATUM, SPHEROID, PROJECTION
            //
            temp = OSRGetAttrValue(map_spatialH, "DATUM", 0);
            if (i == 0)
                    fprintf(stderr,"DATUM: %s, ", temp);

            if (projected) {
                temp = OSRGetAttrValue(map_spatialH, "PROJCS", 0);
                if (i == 0)
                    fprintf(stderr,"PROJCS: %s, ", temp);
 
                temp = OSRGetAttrValue(map_spatialH, "PROJECTION", 0);
                if (i == 0)
                    fprintf(stderr,"PROJECTION: %s, ", temp);
            }

            temp = OSRGetAttrValue(map_spatialH, "GEOGCS", 0);
            if (i == 0)
                    fprintf(stderr,"GEOGCS: %s, ", temp);

            temp = OSRGetAttrValue(map_spatialH, "SPHEROID", 0);
            if (i == 0)
                    fprintf(stderr,"SPHEROID: %s, ", temp);

        }
        else {
            if (i == 0)
                    fprintf(stderr,"  No Spatial Info, ");
            // Assume geographic/WGS84 unless the coordinates go
            // outside the range of lat/long, in which case, exit.
        }


        // Get the extents for this layer.  OGRERR_FAILURE means
        // that the layer has no spatial info or that it would be
        // an expensive operation to establish the extent.
        //OGRErr OGR_L_GetExtent(OGRLayerH hLayer, OGREnvelope *psExtent, int bForce);
        if (OGR_L_GetExtent(layer, &psExtent, FALSE) != OGRERR_FAILURE) {
            // We have extents.  Check whether any part of the layer
            // is within our viewport.
            if (i == 0)
                    fprintf(stderr, "Extents obtained.");
            extents_found++;
        }
        if (i == 0)
            fprintf(stderr, "\n");

/*
        if (extents_found) {
            fprintf(stderr,
                "  MinX: %f, MaxX: %f, MinY: %f, MaxY: %f\n",
                psExtent.MinX,
                psExtent.MaxX,
                psExtent.MinY,
                psExtent.MaxY);
        }
*/


// Dump info about this layer
/*
        layerDefn = OGR_L_GetLayerDefn( layer );
        if (layerDefn != NULL) {
            numFields = OGR_FD_GetFieldCount( layerDefn );

            fprintf(stderr,"  Layer %d: '%s'\n\n", i, OGR_FD_GetName(layerDefn));

            for ( jj=0; jj<numFields; jj++ ) {
                OGRFieldDefnH fieldDefn;

                fieldDefn = OGR_FD_GetFieldDefn( layerDefn, jj );
                fprintf(stderr,"  Field %d: %s (%s)\n", 
                       jj, OGR_Fld_GetNameRef(fieldDefn), 
                       OGR_GetFieldTypeName(OGR_Fld_GetType(fieldDefn)));
            }
            fprintf(stderr,"\n");
        }
*/


// Optimization:  Get the envelope for each feature, skip the
// feature if it's completely outside our viewport.

        // Loop through all of the features in the layer.
        //

//        if ( (featureH = OGR_L_GetNextFeature( layer ) ) != NULL ) {
//if (0) {
        while ( (featureH = OGR_L_GetNextFeature( layer )) != NULL ) {
            OGRGeometryH geometryH;
            int num = 0;
//            char *buffer;

 
            HandlePendingEvents(app_context);
            if (interrupt_drawing_now) {
                if (featureH != NULL)
                    OGR_F_Destroy( featureH );

                if (wgs84_spatialH != NULL) {
                    OSRDestroySpatialReference(wgs84_spatialH);
                }

                if (transformH != NULL) {
                    OCTDestroyCoordinateTransformation(transformH);
                }

                // Close data source
                if (datasourceH != NULL) {
                    OGR_DS_Destroy( datasourceH );
                }

                return; // Exit early
            }

            if (featureH == NULL) {
                continue;
            }


            // Debug code
//OGR_F_DumpReadable( featureH, stderr );


            // Get a handle to the geometry itself
            geometryH = OGR_F_GetGeometryRef(featureH);
            if (geometryH == NULL) {
                OGR_F_Destroy( featureH );
//                if (strlen(geometry_type_name) == 0) {
                    fprintf(stderr,"  Layer %02d:   - No geometry info -\n", i);
//                    geometry_type_name[0] = ' ';
//                    geometry_type_name[1] = '\0';
//                }
// Break out of this loop.  We don't know how to draw anything but
// geometry features yet.  Change this when we start drawing labels.
                break;
//                continue;
            }


            // More debug code.  Print the Well Known Text
            // representation of the geometry.
//            if (OGR_G_ExportToWkt(geometryH, &buffer) == 0) {
//                fprintf(stderr, "%s\n", buffer);
//            }


            // These are from the OGRwkbGeometryType enumerated set
            // in ogr_core.h:
            //
            //          0 "Unknown"
            //          1 "POINT"              (ogrpoint.cpp)
            //          2 "LINESTRING"         (ogrlinestring.cpp)
            //          3 "POLYGON"            (ogrpolygon.cpp)
            //          4 "MULTIPOINT"         (ogrmultipoint.cpp)
            //          5 "MULTILINESTRING"    (ogrmultilinestring.cpp)
            //          6 "MULTIPOLYGON"       (ogrmultipolygon.cpp)
            //          7 "GEOMETRYCOLLECTION" (ogrgeometrycollection.cpp)
            //        100 "None"
            //        101 "LINEARRING"         (ogrlinearring.cpp)
            // 0x80000001 "Point25D"
            // 0x80000002 "LineString25D"
            // 0x80000003 "Polygon25D"
            // 0x80000004 "MultiPoint25D"
            // 0x80000005 "MultiLineString25D"
            // 0x80000006 "MultiPolygon25D"
            // 0x80000007 "GeometryCollection25D"
            //
            // The geometry type will be the same for any particular
            // layer.  We take advantage of that here by querying
            // once per layer and saving the results in variables.
            //
            if (strlen(geometry_type_name) == 0) {
                xastir_snprintf(geometry_type_name,
                    sizeof(geometry_type_name),
                    "%s",
                    OGR_G_GetGeometryName(geometryH));
                geometry_type = OGR_G_GetGeometryType(geometryH);
                fprintf(stderr,"  Layer %02d: ", i); 
                fprintf(stderr,"  Type: %d, %s\n",  
                    geometry_type,
                    geometry_type_name);
            }

            // Debug code 
            //OGR_G_DumpReadable(geometryH, stderr, "Shape: ");


// We could call OGR_G_GetEnvelope() here and calculate for
// ourselves it if is in our viewport, or we could set a filter and
// let the library pass us only those that fit.
//
// If point or line feature, draw in normal manner.  If polygon
// feature, do the "rotation one way = fill, rotation the other way
// = hole" thing?
//
// We need all of the coordinates in WGS84 lat/long.  Use the
// conversion code that's in the indexing portion of this routine to
// accomplish this.


            switch (geometry_type) {

                case 1:             // Point
                case 4:             // MultiPoint
                case 0x80000001:    // Point25D
                case 0x80000004:    // MultiPoint25D

//WE7U
// Hard-coded drawing attributes
(void)XSetLineAttributes (XtDisplay (w), gc, 0, LineSolid, CapButt,JoinMiter);
//(void)XSetForeground(XtDisplay(w), gc, colors[(int)0x08]);  // black
(void)XSetForeground(XtDisplay(w), gc, colors[(int)0x0f]);  // white

                    Draw_OGR_Points(geometryH,
                        1,
                        transformH);
                    break;

                case 2:             // LineString (polyline)
                case 5:             // MultiLineString
                case 0x80000002:    // LineString25D
                case 0x80000005:    // MultiLineString25D

//WE7U
// Hard-coded drawing attributes
(void)XSetLineAttributes (XtDisplay (w), gc, 0, LineSolid, CapButt,JoinMiter);
//(void)XSetForeground(XtDisplay(w), gc, colors[(int)0x08]);  // black
(void)XSetForeground(XtDisplay(w), gc, colors[(int)0x0e]);  // yellow

                    Draw_OGR_Lines(geometryH,
                        1,
                        transformH,
                        fast_extents);
                    break;

                case 3:             // Polygon
                case 6:             // MultiPolygon
                case 0x80000003:    // Polygon25D
                case 0x80000006:    // MultiPolygon25D

//WE7U
// Hard-coded drawing attributes
(void)XSetLineAttributes (XtDisplay (w), gc, 0, LineSolid, CapButt,JoinMiter);
//(void)XSetForeground(XtDisplay(w), gc, colors[(int)0x08]);  // black
//(void)XSetForeground(XtDisplay(w), gc, colors[(int)0x1a]);  // Steel Blue
(void)XSetForeground(XtDisplay(w), gc, colors[(int)0x0e]);  // yellow

                    Draw_OGR_Polygons(geometryH,
                        1,
                        transformH,
                        draw_filled,
                        fast_extents);
                    break;

                case 7:             // GeometryCollection
                case 0x80000007:    // GeometryCollection25D
                default:            // Unknown/Unimplemented

                    num = 0;
                    fprintf(stderr,"  Unknown or unimplemented geometry\n");
                    break;
            }
            if (featureH)
                OGR_F_Destroy( featureH );
        }
        // No need to free layer handle, it belongs to the datasource
    }

    if (transformH != NULL) {
        OCTDestroyCoordinateTransformation(transformH);
    }

    if (wgs84_spatialH != NULL) {
        OSRDestroySpatialReference(wgs84_spatialH);
    }

    // Close data source
    if (datasourceH != NULL) {
        OGR_DS_Destroy( datasourceH );
    }
}



#endif // HAVE_LIBGDAL


