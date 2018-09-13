/******************************************************************************
 *
 * Project:  PROJ
 * Purpose:  ISO19111:2018 implementation
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

#ifndef FROM_PROJ_CPP
#error This file should only be included from a PROJ cpp file
#endif

#ifndef IO_INTERNAL_HH_INCLUDED
#define IO_INTERNAL_HH_INCLUDED

#include <string>

#include "util.hpp"

//! @cond Doxygen_Suppress

NS_PROJ_START

namespace io {

// ---------------------------------------------------------------------------

class WKTConstants {
  public:
    // WKT1
    static const std::string GEOCCS;
    static const std::string GEOGCS;
    static const std::string DATUM; // WKT2 preferred too
    static const std::string UNIT;
    static const std::string SPHEROID;
    static const std::string AXIS;   // WKT2 too
    static const std::string PRIMEM; // WKT2 too
    static const std::string AUTHORITY;
    static const std::string PROJCS;
    static const std::string PROJECTION;
    static const std::string PARAMETER; // WKT2 too
    static const std::string VERT_CS;
    static const std::string VERT_DATUM;
    static const std::string COMPD_CS;
    static const std::string TOWGS84;   // WKT1 only
    static const std::string EXTENSION; // WK1 only - GDAL specific

    // WKT2 preferred
    static const std::string GEODCRS;
    static const std::string LENGTHUNIT;
    static const std::string ANGLEUNIT;
    static const std::string SCALEUNIT;
    static const std::string TIMEUNIT;
    static const std::string ELLIPSOID;
    static const std::string CS;
    static const std::string ID;
    static const std::string PROJCRS;
    static const std::string BASEGEODCRS;
    static const std::string MERIDIAN;
    static const std::string ORDER;
    static const std::string ANCHOR;
    static const std::string CONVERSION;
    static const std::string METHOD;
    static const std::string REMARK;
    static const std::string GEOGCRS;     // WKT2-2018
    static const std::string BASEGEOGCRS; // WKT2-2018
    static const std::string SCOPE;
    static const std::string AREA;
    static const std::string BBOX;
    static const std::string CITATION;
    static const std::string URI;
    static const std::string VERTCRS;
    static const std::string VDATUM;
    static const std::string COMPOUNDCRS;
    static const std::string PARAMETERFILE;
    static const std::string COORDINATEOPERATION;
    static const std::string SOURCECRS;
    static const std::string TARGETCRS;
    static const std::string INTERPOLATIONCRS;
    static const std::string OPERATIONACCURACY;
    static const std::string CONCATENATEDOPERATION; // WKT2-2018
    static const std::string STEP;                  // WKT2-2018
    static const std::string BOUNDCRS;
    static const std::string ABRIDGEDTRANSFORMATION;
    static const std::string DERIVINGCONVERSION;
    static const std::string TDATUM;
    static const std::string CALENDAR; // WKT2-2018
    static const std::string TIMEORIGIN;
    static const std::string TIMECRS;
    static const std::string VERTICALEXTENT;
    static const std::string TIMEEXTENT;

    // WKT2 long
    static const std::string GEODETICCRS;
    static const std::string GEODETICDATUM;
    static const std::string PROJECTEDCRS;
    static const std::string PRIMEMERIDIAN;
    static const std::string GEOGRAPHICCRS; // WKT2-2018
    static const std::string TRF;           // WKT2-2018
    static const std::string VERTICALCRS;
    static const std::string VERTICALDATUM;
    static const std::string VRF; // WKT2-2018
    static const std::string TIMEDATUM;
};

} // namespace io

NS_PROJ_END

//! @endcond

#endif // IO_INTERNAL_HH_INCLUDED
