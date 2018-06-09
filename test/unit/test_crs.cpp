/******************************************************************************
 *
 * Project:  PROJ
 * Purpose:  Test ISO19111:2018 implementation
 * Author:   Even Rouault <even dot rouault at spatialys dot com>
 *
 ******************************************************************************
 * Copyright (c) 2018, Even Rouault <even dot rouault at spatialys dot com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "gtest_include.h"

#include "proj/common.hpp"
#include "proj/coordinateoperation.hpp"
#include "proj/coordinatesystem.hpp"
#include "proj/crs.hpp"
#include "proj/datum.hpp"
#include "proj/io.hpp"
#include "proj/metadata.hpp"
#include "proj/util.hpp"

using namespace osgeo::proj::common;
using namespace osgeo::proj::crs;
using namespace osgeo::proj::cs;
using namespace osgeo::proj::datum;
using namespace osgeo::proj::io;
using namespace osgeo::proj::metadata;
using namespace osgeo::proj::operation;
using namespace osgeo::proj::util;

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4326_get_components) {
    auto crs = GeographicCRS::EPSG_4326;
    EXPECT_EQ(crs->name()->code(), "4326");
    EXPECT_EQ(*(crs->name()->authority()->title()), "EPSG");
    EXPECT_EQ(*(crs->name()->description()), "WGS 84");

    auto datum = crs->datum();
    EXPECT_EQ(datum->name()->code(), "6326");
    EXPECT_EQ(*(datum->name()->authority()->title()), "EPSG");
    EXPECT_EQ(*(datum->name()->description()), "WGS_1984");

    auto ellipsoid = datum->ellipsoid();
    EXPECT_EQ(ellipsoid->semiMajorAxis().value(), 6378137.0);
    EXPECT_EQ(ellipsoid->semiMajorAxis().unit(), UnitOfMeasure::METRE);
    EXPECT_EQ(ellipsoid->inverseFlattening()->value(), 298.257223563);
    EXPECT_EQ(ellipsoid->name()->code(), "7030");
    EXPECT_EQ(*(ellipsoid->name()->authority()->title()), "EPSG");
    EXPECT_EQ(*(ellipsoid->name()->description()), "WGS 84");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4326_as_WKT2) {
    auto crs = GeographicCRS::EPSG_4326;
    WKTFormatterNNPtr f(WKTFormatter::create());
    crs->exportToWKT(f);
    EXPECT_EQ(f->toString(),
              "GEODCRS[\"WGS 84\",\n"
              "    DATUM[\"WGS_1984\",\n"
              "        ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
              "            LENGTHUNIT[\"metre\",1,\n"
              "                ID[\"EPSG\",9001]],\n"
              "            ID[\"EPSG\",7030]],\n"
              "        ID[\"EPSG\",6326]],\n"
              "    PRIMEM[\"Greenwich\",0,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433,\n"
              "            ID[\"EPSG\",9122]],\n"
              "        ID[\"EPSG\",8901]],\n"
              "    CS[ellipsoidal,2],\n"
              "        AXIS[\"latitude\",north,\n"
              "            ORDER[1],\n"
              "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
              "                ID[\"EPSG\",9122]]],\n"
              "        AXIS[\"longitude\",east,\n"
              "            ORDER[2],\n"
              "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
              "                ID[\"EPSG\",9122]]],\n"
              "    ID[\"EPSG\",4326]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4326_as_WKT2_2018) {
    auto crs = GeographicCRS::EPSG_4326;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_2018));
    crs->exportToWKT(f);
    EXPECT_EQ(f->toString(),
              "GEOGCRS[\"WGS 84\",\n"
              "    DATUM[\"WGS_1984\",\n"
              "        ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
              "            LENGTHUNIT[\"metre\",1,\n"
              "                ID[\"EPSG\",9001]],\n"
              "            ID[\"EPSG\",7030]],\n"
              "        ID[\"EPSG\",6326]],\n"
              "    PRIMEM[\"Greenwich\",0,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433,\n"
              "            ID[\"EPSG\",9122]],\n"
              "        ID[\"EPSG\",8901]],\n"
              "    CS[ellipsoidal,2],\n"
              "        AXIS[\"latitude\",north,\n"
              "            ORDER[1],\n"
              "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
              "                ID[\"EPSG\",9122]]],\n"
              "        AXIS[\"longitude\",east,\n"
              "            ORDER[2],\n"
              "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
              "                ID[\"EPSG\",9122]]],\n"
              "    ID[\"EPSG\",4326]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4326_as_WKT2_SIMPLIFIED) {
    auto crs = GeographicCRS::EPSG_4326;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_SIMPLIFIED));
    crs->exportToWKT(f);
    EXPECT_EQ(f->toString(),
              "GEODCRS[\"WGS 84\",\n"
              "    DATUM[\"WGS_1984\",\n"
              "        ELLIPSOID[\"WGS 84\",6378137,298.257223563]],\n"
              "    CS[ellipsoidal,2],\n"
              "        AXIS[\"latitude\",north],\n"
              "        AXIS[\"longitude\",east],\n"
              "        UNIT[\"degree\",0.0174532925199433],\n"
              "    ID[\"EPSG\",4326]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4326_as_WKT2_SIMPLIFIED_single_line) {
    auto crs = GeographicCRS::EPSG_4326;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_SIMPLIFIED));
    f->setMultiLine(false);
    crs->exportToWKT(f);
    EXPECT_EQ(
        f->toString(),
        "GEODCRS[\"WGS 84\",DATUM[\"WGS_1984\",ELLIPSOID[\"WGS "
        "84\",6378137,298.257223563]],"
        "CS[ellipsoidal,2],AXIS[\"latitude\",north],AXIS[\"longitude\",east],"
        "UNIT[\"degree\",0.0174532925199433],ID[\"EPSG\",4326]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4326_as_WKT2_2018_SIMPLIFIED) {
    auto crs = GeographicCRS::EPSG_4326;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_2018_SIMPLIFIED));
    crs->exportToWKT(f);
    EXPECT_EQ(f->toString(),
              "GEOGCRS[\"WGS 84\",\n"
              "    DATUM[\"WGS_1984\",\n"
              "        ELLIPSOID[\"WGS 84\",6378137,298.257223563]],\n"
              "    CS[ellipsoidal,2],\n"
              "        AXIS[\"latitude\",north],\n"
              "        AXIS[\"longitude\",east],\n"
              "        UNIT[\"degree\",0.0174532925199433],\n"
              "    ID[\"EPSG\",4326]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4326_as_WKT1_GDAL) {
    auto crs = GeographicCRS::EPSG_4326;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL));
    crs->exportToWKT(f);
    EXPECT_EQ(f->toString(),
              "GEOGCS[\"WGS 84\",\n"
              "    DATUM[\"WGS_1984\",\n"
              "        SPHEROID[\"WGS 84\",6378137,298.257223563,\n"
              "            AUTHORITY[\"EPSG\",\"7030\"]],\n"
              "        AUTHORITY[\"EPSG\",\"6326\"]],\n"
              "    PRIMEM[\"Greenwich\",0,\n"
              "        AUTHORITY[\"EPSG\",\"8901\"]],\n"
              "    UNIT[\"degree\",0.0174532925199433,\n"
              "        AUTHORITY[\"EPSG\",9122]],\n"
              "    AXIS[\"Latitude\",NORTH],\n"
              "    AXIS[\"Longitude\",EAST],\n"
              "    AUTHORITY[\"EPSG\",\"4326\"]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4979_as_WKT2_SIMPLIFIED) {
    auto crs = GeographicCRS::EPSG_4979;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_SIMPLIFIED));
    crs->exportToWKT(f);
    EXPECT_EQ(f->toString(),
              "GEODCRS[\"WGS 84\",\n"
              "    DATUM[\"WGS_1984\",\n"
              "        ELLIPSOID[\"WGS 84\",6378137,298.257223563]],\n"
              "    CS[ellipsoidal,3],\n"
              "        AXIS[\"latitude\",north,\n"
              "            UNIT[\"degree\",0.0174532925199433]],\n"
              "        AXIS[\"longitude\",east,\n"
              "            UNIT[\"degree\",0.0174532925199433]],\n"
              "        AXIS[\"ellipsoidal height\",up,\n"
              "            UNIT[\"metre\",1]],\n"
              "    ID[\"EPSG\",4979]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4979_as_WKT2_2018_SIMPLIFIED) {
    auto crs = GeographicCRS::EPSG_4979;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_2018_SIMPLIFIED));
    crs->exportToWKT(f);
    EXPECT_EQ(f->toString(),
              "GEOGCRS[\"WGS 84\",\n"
              "    DATUM[\"WGS_1984\",\n"
              "        ELLIPSOID[\"WGS 84\",6378137,298.257223563]],\n"
              "    CS[ellipsoidal,3],\n"
              "        AXIS[\"latitude\",north,\n"
              "            UNIT[\"degree\",0.0174532925199433]],\n"
              "        AXIS[\"longitude\",east,\n"
              "            UNIT[\"degree\",0.0174532925199433]],\n"
              "        AXIS[\"ellipsoidal height\",up,\n"
              "            UNIT[\"metre\",1]],\n"
              "    ID[\"EPSG\",4979]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4979_as_WKT1_GDAL) {
    auto crs = GeographicCRS::EPSG_4979;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL));
    crs->exportToWKT(f);
    // FIXME? WKT1 only supports 2 axis for GEOGCS. So this is an extension of
    // WKT1 as it
    // and GDAL doesn't really export such as beast, although it can import it
    EXPECT_EQ(f->toString(),
              "GEOGCS[\"WGS 84\",\n"
              "    DATUM[\"WGS_1984\",\n"
              "        SPHEROID[\"WGS 84\",6378137,298.257223563,\n"
              "            AUTHORITY[\"EPSG\",\"7030\"]],\n"
              "        AUTHORITY[\"EPSG\",\"6326\"]],\n"
              "    PRIMEM[\"Greenwich\",0,\n"
              "        AUTHORITY[\"EPSG\",\"8901\"]],\n"
              "    UNIT[\"degree\",0.0174532925199433,\n"
              "        AUTHORITY[\"EPSG\",9122]],\n"
              "    AXIS[\"Latitude\",NORTH],\n"
              "    AXIS[\"Longitude\",EAST],\n"
              "    AXIS[\"Ellipsoidal height\",UP],\n"
              "    AUTHORITY[\"EPSG\",\"4979\"]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4807_as_WKT2) {
    auto crs = GeographicCRS::EPSG_4807;
    WKTFormatterNNPtr f(WKTFormatter::create(WKTFormatter::Convention::WKT2));
    crs->exportToWKT(f);
    EXPECT_EQ(
        f->toString(),
        "GEODCRS[\"NTF (Paris)\",\n"
        "    DATUM[\"Nouvelle_Triangulation_Francaise_Paris\",\n"
        "        ELLIPSOID[\"Clarke 1880 (IGN)\",6378249.2,293.466021293627,\n"
        "            LENGTHUNIT[\"metre\",1,\n"
        "                ID[\"EPSG\",9001]],\n"
        "            ID[\"EPSG\",6807]],\n"
        "        ID[\"EPSG\",6807]],\n"
        "    PRIMEM[\"Paris\",2.5969213,\n"
        "        ANGLEUNIT[\"grad\",0.015707963267949,\n"
        "            ID[\"EPSG\",9105]],\n"
        "        ID[\"EPSG\",8903]],\n"
        "    CS[ellipsoidal,2],\n"
        "        AXIS[\"latitude\",north,\n"
        "            ORDER[1],\n"
        "            ANGLEUNIT[\"grad\",0.015707963267949,\n"
        "                ID[\"EPSG\",9105]]],\n"
        "        AXIS[\"longitude\",east,\n"
        "            ORDER[2],\n"
        "            ANGLEUNIT[\"grad\",0.015707963267949,\n"
        "                ID[\"EPSG\",9105]]],\n"
        "    ID[\"EPSG\",4807]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4807_as_WKT2_SIMPLIFIED) {
    auto crs = GeographicCRS::EPSG_4807;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_SIMPLIFIED));
    crs->exportToWKT(f);
    EXPECT_EQ(f->toString(),
              "GEODCRS[\"NTF (Paris)\",\n"
              "    DATUM[\"Nouvelle_Triangulation_Francaise_Paris\",\n"
              "        ELLIPSOID[\"Clarke 1880 "
              "(IGN)\",6378249.2,293.466021293627]],\n"
              "    PRIMEM[\"Paris\",2.5969213],\n"
              "    CS[ellipsoidal,2],\n"
              "        AXIS[\"latitude\",north],\n"
              "        AXIS[\"longitude\",east],\n"
              "        UNIT[\"grad\",0.015707963267949],\n"
              "    ID[\"EPSG\",4807]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4807_as_WKT1_GDAL) {
    auto crs = GeographicCRS::EPSG_4807;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL));
    crs->exportToWKT(f);
    EXPECT_EQ(
        f->toString(),
        "GEOGCS[\"NTF (Paris)\",\n"
        "    DATUM[\"Nouvelle_Triangulation_Francaise_Paris\",\n"
        "        SPHEROID[\"Clarke 1880 (IGN)\",6378249.2,293.466021293627,\n"
        "            AUTHORITY[\"EPSG\",\"6807\"]],\n"
        "        AUTHORITY[\"EPSG\",\"6807\"]],\n"
        "    PRIMEM[\"Paris\",2.33722917,\n" /* WKT1_GDAL weirdness: PRIMEM is
                                                converted to degree */
        "        AUTHORITY[\"EPSG\",\"8903\"]],\n"
        "    UNIT[\"grad\",0.015707963267949,\n"
        "        AUTHORITY[\"EPSG\",9105]],\n"
        "    AXIS[\"Latitude\",NORTH],\n"
        "    AXIS[\"Longitude\",EAST],\n"
        "    AUTHORITY[\"EPSG\",\"4807\"]]");
}

// ---------------------------------------------------------------------------

static GeodeticCRSNNPtr createGeocentric() {
    PropertyMap propertiesCRS;
    propertiesCRS.set(Identifier::AUTHORITY_KEY, "EPSG")
        .set(Identifier::CODE_KEY, 4328)
        .set(Identifier::DESCRIPTION_KEY, "WGS 84");
    return GeodeticCRS::create(propertiesCRS, GeodeticReferenceFrame::EPSG_6326,
                               CartesianCS::createGeocentric());
}

// ---------------------------------------------------------------------------

TEST(crs, geocentricCRS_as_WKT2) {
    auto crs = createGeocentric();

    auto expected = "GEODCRS[\"WGS 84\",\n"
                    "    DATUM[\"WGS_1984\",\n"
                    "        ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
                    "            LENGTHUNIT[\"metre\",1,\n"
                    "                ID[\"EPSG\",9001]],\n"
                    "            ID[\"EPSG\",7030]],\n"
                    "        ID[\"EPSG\",6326]],\n"
                    "    PRIMEM[\"Greenwich\",0,\n"
                    "        ANGLEUNIT[\"degree\",0.0174532925199433,\n"
                    "            ID[\"EPSG\",9122]],\n"
                    "        ID[\"EPSG\",8901]],\n"
                    "    CS[Cartesian,3],\n"
                    "        AXIS[\"(X)\",geocentricX,\n"
                    "            ORDER[1],\n"
                    "            LENGTHUNIT[\"metre\",1,\n"
                    "                ID[\"EPSG\",9001]]],\n"
                    "        AXIS[\"(Y)\",geocentricY,\n"
                    "            ORDER[2],\n"
                    "            LENGTHUNIT[\"metre\",1,\n"
                    "                ID[\"EPSG\",9001]]],\n"
                    "        AXIS[\"(Z)\",geocentricZ,\n"
                    "            ORDER[3],\n"
                    "            LENGTHUNIT[\"metre\",1,\n"
                    "                ID[\"EPSG\",9001]]],\n"
                    "    ID[\"EPSG\",4328]]";

    EXPECT_EQ(crs->exportToWKT(WKTFormatter::create()), expected);
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT2_2018)),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, geocentricCRS_as_WKT2_simplified) {
    auto crs = createGeocentric();

    auto expected = "GEODCRS[\"WGS 84\",\n"
                    "    DATUM[\"WGS_1984\",\n"
                    "        ELLIPSOID[\"WGS 84\",6378137,298.257223563]],\n"
                    "    CS[Cartesian,3],\n"
                    "        AXIS[\"(X)\",geocentricX],\n"
                    "        AXIS[\"(Y)\",geocentricY],\n"
                    "        AXIS[\"(Z)\",geocentricZ],\n"
                    "        UNIT[\"metre\",1],\n"
                    "    ID[\"EPSG\",4328]]";

    EXPECT_EQ(crs->exportToWKT(WKTFormatter::create(
                  WKTFormatter::Convention::WKT2_SIMPLIFIED)),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, geocentricCRS_as_WKT1_GDAL) {
    auto crs = createGeocentric();
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL));
    crs->exportToWKT(f);
    EXPECT_EQ(f->toString(),
              "GEOCCS[\"WGS 84\",\n"
              "    DATUM[\"WGS_1984\",\n"
              "        SPHEROID[\"WGS 84\",6378137,298.257223563,\n"
              "            AUTHORITY[\"EPSG\",\"7030\"]],\n"
              "        AUTHORITY[\"EPSG\",\"6326\"]],\n"
              "    PRIMEM[\"Greenwich\",0,\n"
              "        AUTHORITY[\"EPSG\",\"8901\"]],\n"
              "    UNIT[\"metre\",1,\n"
              "        AUTHORITY[\"EPSG\",9001]],\n"
              "    AXIS[\"Geocentric X\",OTHER],\n"
              "    AXIS[\"Geocentric Y\",OTHER],\n"
              "    AXIS[\"Geocentric Z\",NORTH],\n"
              "    AUTHORITY[\"EPSG\",\"4328\"]]");
}

// ---------------------------------------------------------------------------

static ProjectedCRSNNPtr createProjected() {
    PropertyMap propertiesCRS;
    propertiesCRS.set(Identifier::AUTHORITY_KEY, "EPSG")
        .set(Identifier::CODE_KEY, 32631)
        .set(Identifier::DESCRIPTION_KEY, "WGS 84 / UTM zone 31N");
    return ProjectedCRS::create(propertiesCRS, GeographicCRS::EPSG_4326,
                                Conversion::createUTM(31, true),
                                CartesianCS::createEastingNorthingMetre());
}

// ---------------------------------------------------------------------------

TEST(crs, projectedCRS_as_WKT2) {
    auto crs = createProjected();

    auto expected =
        "PROJCRS[\"WGS 84 / UTM zone 31N\",\n"
        "    BASEGEODCRS[\"WGS 84\",\n"
        "        DATUM[\"WGS_1984\",\n"
        "            ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
        "                LENGTHUNIT[\"metre\",1,\n"
        "                    ID[\"EPSG\",9001]],\n"
        "                ID[\"EPSG\",7030]],\n"
        "            ID[\"EPSG\",6326]],\n"
        "        PRIMEM[\"Greenwich\",0,\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
        "                ID[\"EPSG\",9122]],\n"
        "            ID[\"EPSG\",8901]]],\n"
        "    CONVERSION[\"UTM zone 31N\",\n"
        "        METHOD[\"Transverse Mercator\",\n"
        "            ID[\"EPSG\",9807]],\n"
        "        PARAMETER[\"Latitude of natural origin\",0,\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
        "                ID[\"EPSG\",9122]],\n"
        "            ID[\"EPSG\",8801]],\n"
        "        PARAMETER[\"Longitude of natural origin\",3,\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
        "                ID[\"EPSG\",9122]],\n"
        "            ID[\"EPSG\",8802]],\n"
        "        PARAMETER[\"Scale factor at natural origin\",0.9996,\n"
        "            SCALEUNIT[\"unity\",1,\n"
        "                ID[\"EPSG\",9201]],\n"
        "            ID[\"EPSG\",8805]],\n"
        "        PARAMETER[\"False easting\",500000,\n"
        "            LENGTHUNIT[\"metre\",1,\n"
        "                ID[\"EPSG\",9001]],\n"
        "            ID[\"EPSG\",8806]],\n"
        "        PARAMETER[\"False northing\",0,\n"
        "            LENGTHUNIT[\"metre\",1,\n"
        "                ID[\"EPSG\",9001]],\n"
        "            ID[\"EPSG\",8807]],\n"
        "        ID[\"EPSG\",16031]],\n"
        "    CS[Cartesian,2],\n"
        "        AXIS[\"(E)\",east,\n"
        "            ORDER[1],\n"
        "            LENGTHUNIT[\"metre\",1,\n"
        "                ID[\"EPSG\",9001]]],\n"
        "        AXIS[\"(N)\",north,\n"
        "            ORDER[2],\n"
        "            LENGTHUNIT[\"metre\",1,\n"
        "                ID[\"EPSG\",9001]]],\n"
        "    ID[\"EPSG\",32631]]";

    EXPECT_EQ(crs->exportToWKT(WKTFormatter::create()), expected);
}

// ---------------------------------------------------------------------------

TEST(crs, projectedCRS_as_WKT2_simplified) {
    auto crs = createProjected();

    auto expected =
        "PROJCRS[\"WGS 84 / UTM zone 31N\",\n"
        "    BASEGEODCRS[\"WGS 84\",\n"
        "        DATUM[\"WGS_1984\",\n"
        "            ELLIPSOID[\"WGS 84\",6378137,298.257223563]],\n"
        "        UNIT[\"degree\",0.0174532925199433]],\n"
        "    CONVERSION[\"UTM zone 31N\",\n"
        "        METHOD[\"Transverse Mercator\"],\n"
        "        PARAMETER[\"Latitude of natural origin\",0],\n"
        "        PARAMETER[\"Longitude of natural origin\",3],\n"
        "        PARAMETER[\"Scale factor at natural origin\",0.9996],\n"
        "        PARAMETER[\"False easting\",500000],\n"
        "        PARAMETER[\"False northing\",0]],\n"
        "    CS[Cartesian,2],\n"
        "        AXIS[\"(E)\",east],\n"
        "        AXIS[\"(N)\",north],\n"
        "        UNIT[\"metre\",1],\n"
        "    ID[\"EPSG\",32631]]";

    EXPECT_EQ(crs->exportToWKT(WKTFormatter::create(
                  WKTFormatter::Convention::WKT2_SIMPLIFIED)),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, projectedCRS_as_WKT2_2018_simplified) {
    auto crs = createProjected();

    auto expected =
        "PROJCRS[\"WGS 84 / UTM zone 31N\",\n"
        "    BASEGEOGCRS[\"WGS 84\",\n"
        "        DATUM[\"WGS_1984\",\n"
        "            ELLIPSOID[\"WGS 84\",6378137,298.257223563]],\n"
        "        UNIT[\"degree\",0.0174532925199433]],\n"
        "    CONVERSION[\"UTM zone 31N\",\n"
        "        METHOD[\"Transverse Mercator\"],\n"
        "        PARAMETER[\"Latitude of natural origin\",0],\n"
        "        PARAMETER[\"Longitude of natural origin\",3],\n"
        "        PARAMETER[\"Scale factor at natural origin\",0.9996],\n"
        "        PARAMETER[\"False easting\",500000],\n"
        "        PARAMETER[\"False northing\",0]],\n"
        "    CS[Cartesian,2],\n"
        "        AXIS[\"(E)\",east],\n"
        "        AXIS[\"(N)\",north],\n"
        "        UNIT[\"metre\",1],\n"
        "    ID[\"EPSG\",32631]]";

    EXPECT_EQ(crs->exportToWKT(WKTFormatter::create(
                  WKTFormatter::Convention::WKT2_2018_SIMPLIFIED)),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, projectedCRS_as_WKT1_GDAL) {
    auto crs = createProjected();

    auto expected = "PROJCS[\"WGS 84 / UTM zone 31N\",\n"
                    "    GEOGCS[\"WGS 84\",\n"
                    "        DATUM[\"WGS_1984\",\n"
                    "            SPHEROID[\"WGS 84\",6378137,298.257223563,\n"
                    "                AUTHORITY[\"EPSG\",\"7030\"]],\n"
                    "            AUTHORITY[\"EPSG\",\"6326\"]],\n"
                    "        PRIMEM[\"Greenwich\",0,\n"
                    "            AUTHORITY[\"EPSG\",\"8901\"]],\n"
                    "        UNIT[\"degree\",0.0174532925199433,\n"
                    "            AUTHORITY[\"EPSG\",9122]],\n"
                    "        AXIS[\"Latitude\",NORTH],\n"
                    "        AXIS[\"Longitude\",EAST],\n"
                    "        AUTHORITY[\"EPSG\",\"4326\"]],\n"
                    "    PROJECTION[\"Transverse_Mercator\"],\n"
                    "    PARAMETER[\"latitude_of_origin\",0],\n"
                    "    PARAMETER[\"central_meridian\",3],\n"
                    "    PARAMETER[\"scale_factor\",0.9996],\n"
                    "    PARAMETER[\"false_easting\",500000],\n"
                    "    PARAMETER[\"false_northing\",0],\n"
                    "    UNIT[\"metre\",1,\n"
                    "        AUTHORITY[\"EPSG\",9001]],\n"
                    "    AXIS[\"Easting\",EAST],\n"
                    "    AXIS[\"Northing\",NORTH],\n"
                    "    AUTHORITY[\"EPSG\",\"32631\"]]";

    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL)),
              expected);
}

// ---------------------------------------------------------------------------

TEST(datum, datum_with_ANCHOR) {
    auto datum = GeodeticReferenceFrame::create(
        PropertyMap().set(Identifier::DESCRIPTION_KEY, "WGS_1984 with anchor"),
        Ellipsoid::EPSG_7030, optional<std::string>("My anchor"),
        PrimeMeridian::GREENWICH);

    auto expected = "DATUM[\"WGS_1984 with anchor\",\n"
                    "    ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
                    "        LENGTHUNIT[\"metre\",1,\n"
                    "            ID[\"EPSG\",9001]],\n"
                    "        ID[\"EPSG\",7030]],\n"
                    "    ANCHOR[\"My anchor\"]]";

    EXPECT_EQ(datum->exportToWKT(WKTFormatter::create()), expected);
}

// ---------------------------------------------------------------------------

TEST(datum, cs_with_MERIDIAN) {
    std::vector<CoordinateSystemAxisPtr> axis{
        CoordinateSystemAxis::create(
            PropertyMap().set(Identifier::DESCRIPTION_KEY, "Easting"), "X",
            AxisDirection::SOUTH, UnitOfMeasure::METRE,
            Meridian::create(Angle(90.0))),
        CoordinateSystemAxis::create(
            PropertyMap().set(Identifier::DESCRIPTION_KEY, "Northing"), "Y",
            AxisDirection::SOUTH, UnitOfMeasure::METRE,
            Meridian::create(Angle(180.0)))};
    auto cs(CartesianCS::create(PropertyMap(), axis[0], axis[1]));

    auto expected = "CS[Cartesian,2]\n"
                    "    AXIS[\"easting (X)\",south,\n"
                    "        MERIDIAN[90,\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
                    "                ID[\"EPSG\",9122]]],\n"
                    "        ORDER[1],\n"
                    "        LENGTHUNIT[\"metre\",1,\n"
                    "            ID[\"EPSG\",9001]]],\n"
                    "    AXIS[\"northing (Y)\",south,\n"
                    "        MERIDIAN[180,\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
                    "                ID[\"EPSG\",9122]]],\n"
                    "        ORDER[2],\n"
                    "        LENGTHUNIT[\"metre\",1,\n"
                    "            ID[\"EPSG\",9001]]]";

    EXPECT_EQ(cs->exportToWKT(WKTFormatter::create()), expected);
}
