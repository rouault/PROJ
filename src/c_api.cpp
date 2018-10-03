/******************************************************************************
 *
 * Project:  PROJ
 * Purpose:  C API wraper of C++ API
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

#include <cassert>
#include <cstdarg>
#include <cstring>
#include <map>

#include "proj/common.hpp"
#include "proj/coordinateoperation.hpp"
#include "proj/crs.hpp"
#include "proj/datum.hpp"
#include "proj/io.hpp"
#include "proj/util.hpp"

// PROJ include order is sensitive
// clang-format off
#include "proj_internal.h"
#include "proj.h"
#include "projects.h"
// clang-format on

using namespace NS_PROJ::common;
using namespace NS_PROJ::crs;
using namespace NS_PROJ::datum;
using namespace NS_PROJ::io;
using namespace NS_PROJ::operation;
using namespace NS_PROJ::util;

// ---------------------------------------------------------------------------

static void proj_log_error(PJ_CONTEXT *ctx, const char *function,
                           const char *text) {
    std::string msg(function);
    msg += ": ";
    msg += text;
    ctx->logger(ctx->app_data, PJ_LOG_ERROR, msg.c_str());
}

// ---------------------------------------------------------------------------

/** \brief Opaque object representing a Ellipsoid, Datum, CRS or Coordinate
 * Operation. */
struct PJ_OBJ {
    //! @cond Doxygen_Suppress
    PJ_CONTEXT *ctx;
    BaseObjectNNPtr obj;

    // cached results
    std::map<PJ_WKT_TYPE, std::string> mapWKTString;
    std::map<PJ_PROJ_STRING_TYPE, std::string> mapPROJString;

    explicit PJ_OBJ(PJ_CONTEXT *ctxIn, BaseObjectNNPtr objIn)
        : ctx(ctxIn), obj(objIn) {}
    //! @endcond
};

// ---------------------------------------------------------------------------

/** \brief Instanciate an object from a WKT string.
 *
 * This function calls osgeo::proj::io::WKTParser::createFromWKT()
 *
 * The returned object must be unreferenced with proj_obj_unref() after use.
 *
 * @param ctx PROJ context, or NULL for default context
 * @param wkt WKT string (must not be NULL)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL in
 * case of error.
 */
PJ_OBJ *proj_obj_create_from_wkt(PJ_CONTEXT *ctx, const char *wkt) {
    if (ctx == nullptr) {
        ctx = pj_get_default_ctx();
    }
    assert(wkt);
    try {
        return new PJ_OBJ(ctx, WKTParser().createFromWKT(wkt));
    } catch (const std::exception &e) {
        proj_log_error(ctx, __FUNCTION__, e.what());
        return nullptr;
    }
}

// ---------------------------------------------------------------------------

/** \brief Instanciate an object from a PROJ string.
 *
 * This function calls osgeo::proj::io::PROJStringParser::createFromPROJString()
 *
 * The returned object must be unreferenced with proj_obj_unref() after use.
 *
 * @param ctx PROJ context, or NULL for default context
 * @param proj_string PROJ string (must not be NULL)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL in
 * case of error.
 */
PJ_OBJ *proj_obj_create_from_proj_string(PJ_CONTEXT *ctx,
                                         const char *proj_string) {
    if (ctx == nullptr) {
        ctx = pj_get_default_ctx();
    }
    assert(proj_string);
    try {
        return new PJ_OBJ(ctx,
                          PROJStringParser().createFromPROJString(proj_string));
    } catch (const std::exception &e) {
        proj_log_error(ctx, __FUNCTION__, e.what());
        return nullptr;
    }
}

// ---------------------------------------------------------------------------

/** \brief Opaque object representing a database context. */
struct PJ_DATABASE_CONTEXT {
    //! @cond Doxygen_Suppress
    PJ_CONTEXT *ctx;
    DatabaseContextNNPtr obj;

    explicit PJ_DATABASE_CONTEXT(PJ_CONTEXT *ctxIn, DatabaseContextNNPtr objIn)
        : ctx(ctxIn), obj(objIn) {}
    //! @endcond
};

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

/** Auxiliary structure to PJ_CONTEXT storing C++ context stuff. */
struct projCppContext {
    DatabaseContextNNPtr databaseContext;

    explicit projCppContext(DatabaseContextNNPtr databaseContextIn)
        : databaseContext(databaseContextIn) {}
};

// ---------------------------------------------------------------------------

void proj_context_delete_cpp_context(struct projCppContext *cppContext) {
    delete cppContext;
}

//! @endcond

// ---------------------------------------------------------------------------

/** \brief Instanciate an object from a database lookup.
 *
 * The returned object must be unreferenced with proj_obj_unref() after use.
 *
 * @param ctx Context, or NULL for default context.
 * @param auth_name Authority name (must not be NULL)
 * @param code Object code (must not be NULL)
 * @param category Object category
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL in
 * case of error.
 */
PJ_OBJ *proj_obj_create_from_database(PJ_CONTEXT *ctx, const char *auth_name,
                                      const char *code,
                                      PJ_OBJ_CATEGORY category) {
    assert(auth_name);
    assert(code);

    try {
        if (ctx->cpp_context == nullptr) {
            ctx->cpp_context = new projCppContext(DatabaseContext::create());
        }
        auto factory = AuthorityFactory::create(
            ctx->cpp_context->databaseContext, auth_name);
        switch (category) {
        case PJ_OBJ_CATEGORY_ELLIPSOID:
            return new PJ_OBJ(ctx, factory->createEllipsoid(code));

        case PJ_OBJ_CATEGORY_DATUM:
            return new PJ_OBJ(ctx, factory->createDatum(code));

        case PJ_OBJ_CATEGORY_CRS:
            return new PJ_OBJ(ctx,
                              factory->createCoordinateReferenceSystem(code));

        case PJ_OBJ_CATEGORY_COORDINATE_OPERATION:
            return new PJ_OBJ(ctx, factory->createCoordinateOperation(code));
        }
    } catch (const std::exception &e) {
        proj_log_error(ctx, __FUNCTION__, e.what());
    }
    return nullptr;
}

// ---------------------------------------------------------------------------

/** \brief Drops a reference on an object.
 *
 * This method should be called one and exactly one for each function
 * returning a PJ_OBJ*
 *
 * @param obj Object, or NULL.
 */
void proj_obj_unref(PJ_OBJ *obj) { delete obj; }

// ---------------------------------------------------------------------------

/** \brief Return the type of an object.
 *
 * @param obj Object (must not be NULL)
 * @return its type.
 */
PJ_OBJ_TYPE proj_obj_get_type(PJ_OBJ *obj) {
    assert(obj);
    if (nn_dynamic_pointer_cast<Ellipsoid>(obj->obj)) {
        return PJ_OBJ_TYPE_ELLIPSOID;
    }

    if (nn_dynamic_pointer_cast<DynamicGeodeticReferenceFrame>(obj->obj)) {
        return PJ_OBJ_TYPE_DYNAMIC_GEODETIC_REFERENCE_FRAME;
    }
    if (nn_dynamic_pointer_cast<GeodeticReferenceFrame>(obj->obj)) {
        return PJ_OBJ_TYPE_GEODETIC_REFERENCE_FRAME;
    }
    if (nn_dynamic_pointer_cast<DynamicVerticalReferenceFrame>(obj->obj)) {
        return PJ_OBJ_TYPE_DYNAMIC_VERTICAL_REFERENCE_FRAME;
    }
    if (nn_dynamic_pointer_cast<VerticalReferenceFrame>(obj->obj)) {
        return PJ_OBJ_TYPE_VERTICAL_REFERENCE_FRAME;
    }
    if (nn_dynamic_pointer_cast<DatumEnsemble>(obj->obj)) {
        return PJ_OBJ_TYPE_DATUM_ENSEMBLE;
    }

    if (nn_dynamic_pointer_cast<GeographicCRS>(obj->obj)) {
        return PJ_OBJ_TYPE_GEOGRAPHIC_CRS;
    }
    if (nn_dynamic_pointer_cast<GeodeticCRS>(obj->obj)) {
        return PJ_OBJ_TYPE_GEODETIC_CRS;
    }
    if (nn_dynamic_pointer_cast<VerticalCRS>(obj->obj)) {
        return PJ_OBJ_TYPE_VERTICAL_CRS;
    }
    if (nn_dynamic_pointer_cast<ProjectedCRS>(obj->obj)) {
        return PJ_OBJ_TYPE_PROJECTED_CRS;
    }
    if (nn_dynamic_pointer_cast<CompoundCRS>(obj->obj)) {
        return PJ_OBJ_TYPE_COMPOUND_CRS;
    }
    if (nn_dynamic_pointer_cast<TemporalCRS>(obj->obj)) {
        return PJ_OBJ_TYPE_TEMPORAL_CRS;
    }
    if (nn_dynamic_pointer_cast<BoundCRS>(obj->obj)) {
        return PJ_OBJ_TYPE_BOUND_CRS;
    }
    if (nn_dynamic_pointer_cast<CRS>(obj->obj)) {
        return PJ_OBJ_TYPE_OTHER_CRS;
    }

    if (nn_dynamic_pointer_cast<Conversion>(obj->obj)) {
        return PJ_OBJ_TYPE_CONVERSION;
    }
    if (nn_dynamic_pointer_cast<Transformation>(obj->obj)) {
        return PJ_OBJ_TYPE_TRANSFORMATION;
    }
    if (nn_dynamic_pointer_cast<ConcatenatedOperation>(obj->obj)) {
        return PJ_OBJ_TYPE_CONCATENATED_OPERATION;
    }
    if (nn_dynamic_pointer_cast<CoordinateOperation>(obj->obj)) {
        return PJ_OBJ_TYPE_OTHER_COORDINATE_OPERATION;
    }

    return PJ_OBJ_TYPE_UNKNOWN;
}

// ---------------------------------------------------------------------------

/** \brief Return whether an object is a CRS
 *
 * @param obj Object (must not be NULL)
 */
int PROJ_DLL proj_obj_is_crs(PJ_OBJ *obj) {
    assert(obj);
    return nn_dynamic_pointer_cast<CRS>(obj->obj) != nullptr;
}

// ---------------------------------------------------------------------------

/** \brief Get the name of an object.
 *
 * The lifetime of the returned string is the same as the input obj parameter.
 *
 * @param obj Object (must not be NULL)
 * @return a string, or NULL in case of error or missing name.
 */
const char *proj_obj_get_name(PJ_OBJ *obj) {
    assert(obj);
    auto identifiable = nn_dynamic_pointer_cast<IdentifiedObject>(obj->obj);
    if (!identifiable) {
        proj_log_error(obj->ctx, __FUNCTION__,
                       "Object type not castable to IdentifiedObject");
        return nullptr;
    }
    if (!identifiable->name()->description().has_value()) {
        return nullptr;
    }
    return identifiable->name()->description()->c_str();
}

// ---------------------------------------------------------------------------

/** \brief Get the authority name / codespace of an identifier of an object.
 *
 * The lifetime of the returned string is the same as the input obj parameter.
 *
 * @param obj Object (must not be NULL)
 * @param index Index of the identifier. 0 = first identifier
 * @return a string, or NULL in case of error or missing name.
 */
const char *proj_obj_get_id_auth_name(PJ_OBJ *obj, int index) {
    assert(obj);
    auto identifiable = nn_dynamic_pointer_cast<IdentifiedObject>(obj->obj);
    if (!identifiable) {
        proj_log_error(obj->ctx, __FUNCTION__,
                       "Object type not castable to IdentifiedObject");
        return nullptr;
    }
    if (index < 0 ||
        static_cast<size_t>(index) >= identifiable->identifiers().size()) {
        return nullptr;
    }
    if (!identifiable->identifiers()[index]->codeSpace().has_value()) {
        return nullptr;
    }
    return identifiable->identifiers()[index]->codeSpace()->c_str();
}

// ---------------------------------------------------------------------------

/** \brief Get the code of an identifier of an object.
 *
 * The lifetime of the returned string is the same as the input obj parameter.
 *
 * @param obj Object (must not be NULL)
 * @param index Index of the identifier. 0 = first identifier
 * @return a string, or NULL in case of error or missing name.
 */
const char *proj_obj_get_id_code(PJ_OBJ *obj, int index) {
    assert(obj);
    auto identifiable = nn_dynamic_pointer_cast<IdentifiedObject>(obj->obj);
    if (!identifiable) {
        proj_log_error(obj->ctx, __FUNCTION__,
                       "Object type not castable to IdentifiedObject");
        return nullptr;
    }
    if (index < 0 ||
        static_cast<size_t>(index) >= identifiable->identifiers().size()) {
        return nullptr;
    }
    return identifiable->identifiers()[index]->code().c_str();
}

// ---------------------------------------------------------------------------

/** \brief Get a WKT representation of an object.
 *
 * The lifetime of the returned string is the same as the input obj parameter.
 *
 * This function calls osgeo::proj::io::IWKTExportable::exportToWKT().
 *
 * This function may return NULL if the object is not compatible with an
 * export to the requested type.
 *
 * @param obj Object (must not be NULL)
 * @param type WKT version.
 * @return a string, or NULL in case of error.
 */
const char *proj_obj_as_wkt(PJ_OBJ *obj, PJ_WKT_TYPE type) {
    assert(obj);
    auto iter = obj->mapWKTString.find(type);
    if (iter != obj->mapWKTString.end()) {
        return iter->second.c_str();
    }
    auto wktExportable = nn_dynamic_pointer_cast<IWKTExportable>(obj->obj);
    if (!wktExportable) {
        proj_log_error(obj->ctx, __FUNCTION__,
                       "Object type not exportable to WKT");
        return nullptr;
    }
    WKTFormatter::Convention convention = WKTFormatter::Convention::WKT2_2018;
    switch (type) {
    case PJ_WKT2_2018:
        convention = WKTFormatter::Convention::WKT2_2018;
        break;
    case PJ_WKT2_2018_SIMPLIFIED:
        convention = WKTFormatter::Convention::WKT2_2018_SIMPLIFIED;
        break;
    case PJ_WKT2_2015:
        convention = WKTFormatter::Convention::WKT2_2015;
        break;
    case PJ_WKT2_2015_SIMPLIFIED:
        convention = WKTFormatter::Convention::WKT2_2015_SIMPLIFIED;
        break;
    case PJ_WKT1_GDAL:
        convention = WKTFormatter::Convention::WKT1_GDAL;
        break;
    }
    try {
        auto wkt = wktExportable->exportToWKT(WKTFormatter::create(convention));
        obj->mapWKTString[type] = wkt;
        return obj->mapWKTString[type].c_str();
    } catch (const std::exception &e) {
        proj_log_error(obj->ctx, __FUNCTION__, e.what());
        return nullptr;
    }
}

// ---------------------------------------------------------------------------

/** \brief Get a PROJ string representation of an object.
 *
 * The lifetime of the returned string is the same as the input obj parameter.
 *
 * This function calls
 * osgeo::proj::io::IPROJStringExportable::exportToPROJString().
 *
 * This function may return NULL if the object is not compatible with an
 * export to the requested type.
 *
 * @param obj Object (must not be NULL)
 * @param type PROJ String version.
 * @return a string, or NULL in case of error.
 */
const char *proj_obj_as_proj_string(PJ_OBJ *obj, PJ_PROJ_STRING_TYPE type) {
    assert(obj);
    auto iter = obj->mapPROJString.find(type);
    if (iter != obj->mapPROJString.end()) {
        return iter->second.c_str();
    }
    auto exportable = nn_dynamic_pointer_cast<IPROJStringExportable>(obj->obj);
    if (!exportable) {
        proj_log_error(obj->ctx, __FUNCTION__,
                       "Object type not exportable to PROJ");
        return nullptr;
    }
    PROJStringFormatter::Convention convention =
        PROJStringFormatter::Convention::PROJ_5;
    switch (type) {
    case PJ_PROJ_5:
        convention = PROJStringFormatter::Convention::PROJ_5;
        break;
    case PJ_PROJ_4:
        convention = PROJStringFormatter::Convention::PROJ_4;
        break;
    }
    try {
        auto wkt = exportable->exportToPROJString(
            PROJStringFormatter::create(convention));
        obj->mapPROJString[type] = wkt;
        return obj->mapPROJString[type].c_str();
    } catch (const std::exception &e) {
        proj_log_error(obj->ctx, __FUNCTION__, e.what());
        return nullptr;
    }
}

// ---------------------------------------------------------------------------

static GeographicCRSPtr extractGeographicCRS(PJ_OBJ *crs, const char *fname) {
    assert(crs);
    auto l_crs = nn_dynamic_pointer_cast<CRS>(crs->obj);
    if (!l_crs) {
        proj_log_error(crs->ctx, fname, "Object is not a CRS");
        return nullptr;
    }
    auto geogCRS = l_crs->extractGeographicCRS();
    if (!geogCRS) {
        proj_log_error(crs->ctx, fname, "CRS has no geographic CRS");
        return nullptr;
    }
    return geogCRS;
}

// ---------------------------------------------------------------------------

/** \brief Extract the geographicCRS from a CRS
 *
 * The returned object must be unreferenced with proj_obj_unref() after
 * use.
 *
 * @param crs Objet of type CRS (must not be NULL)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL
 * in case of error.
 */
PJ_OBJ *proj_obj_crs_get_geographic_crs(PJ_OBJ *crs) {
    auto geogCRS = extractGeographicCRS(crs, __FUNCTION__);
    if (!geogCRS) {
        return nullptr;
    }
    return new PJ_OBJ(crs->ctx, NN_CHECK_ASSERT(geogCRS));
}

// ---------------------------------------------------------------------------

/** \brief Extract a CRS component from a CompoundCRS
 *
 * The returned object must be unreferenced with proj_obj_unref() after
 * use.
 *
 * @param crs Objet of type CRS (must not be NULL)
 * @param index Index of the CRS component (typically 0 = horizontal, 1 =
 * vertical)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL
 * in case of error.
 */
PJ_OBJ *proj_obj_crs_get_sub_crs(PJ_OBJ *crs, int index) {
    assert(crs);
    auto l_crs = nn_dynamic_pointer_cast<CompoundCRS>(crs->obj);
    if (!l_crs) {
        proj_log_error(crs->ctx, __FUNCTION__, "Object is not a CompoundCRS");
        return nullptr;
    }
    auto components = l_crs->componentReferenceSystems();
    if (index < 0 || static_cast<size_t>(index) >= components.size()) {
        return nullptr;
    }
    return new PJ_OBJ(crs->ctx, components[index]);
}

// ---------------------------------------------------------------------------

/** \brief Returns potentially
 * a BoundCRS, with a transformation to EPSG:4326, wrapping this CRS
 *
 * The returned object must be unreferenced with proj_obj_unref() after
 * use.
 *
 * This is the same as method
 * osgeo::proj::crs::CRS::createBoundCRSToWGS84IfPossible()
 *
 * @param crs Objet of type CRS (must not be NULL)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL
 * in case of error.
 */
PJ_OBJ *proj_obj_crs_create_bound_crs_to_WGS84(PJ_OBJ *crs) {
    assert(crs);
    auto l_crs = nn_dynamic_pointer_cast<CRS>(crs->obj);
    if (!l_crs) {
        proj_log_error(crs->ctx, __FUNCTION__, "Object is not a CRS");
        return nullptr;
    }
    return new PJ_OBJ(crs->ctx, l_crs->createBoundCRSToWGS84IfPossible());
}

// ---------------------------------------------------------------------------

/** \brief Extract the ellipsoid from a CRS
 *
 * The returned object must be unreferenced with proj_obj_unref() after
 * use.
 *
 * @param crs Objet of type CRS (must not be NULL)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL
 * in case of error.
 */
PJ_OBJ *proj_obj_crs_get_ellipsoid(PJ_OBJ *crs) {
    auto geogCRS = extractGeographicCRS(crs, __FUNCTION__);
    if (!geogCRS) {
        return nullptr;
    }
    return new PJ_OBJ(crs->ctx, geogCRS->ellipsoid());
}

// ---------------------------------------------------------------------------

/** \brief Extract the horizontal datum from a CRS
 *
 * The returned object must be unreferenced with proj_obj_unref() after
 * use.
 *
 * @param crs Objet of type CRS (must not be NULL)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL
 * in case of error.
 */
PJ_OBJ *proj_obj_crs_get_horizontal_datum(PJ_OBJ *crs) {
    auto geogCRS = extractGeographicCRS(crs, __FUNCTION__);
    if (!geogCRS) {
        return nullptr;
    }
    auto datum = geogCRS->datum();
    if (!datum) {
        auto datumEnsemble = geogCRS->datumEnsemble();
        if (datumEnsemble) {
            return new PJ_OBJ(crs->ctx, NN_CHECK_ASSERT(datumEnsemble));
        }
        proj_log_error(crs->ctx, __FUNCTION__, "CRS has no datum");
        return nullptr;
    }
    return new PJ_OBJ(crs->ctx, NN_CHECK_ASSERT(datum));
}

// ---------------------------------------------------------------------------

/** \brief Return ellipsoid parameters.
 *
 * @param ellipsoid Object of type Ellipsoid (must not be NULL)
 * @param pSemiMajorMetre Pointer to a value to store the semi-major axis in
 * metre.
 * @param pSemiMinorMetre Pointer to a value to store the semi-minor axis in
 * metre.
 * @param pIsSemiMinorComputed Pointer to a boolean value to indicate if the
 * semi-minor value was computed. If FALSE, its value comes from the
 * definition.
 * @param pInverseFlattening Pointer to a value to store the inverse
 * flattening.
 * @return TRUE in case of success.
 */
int proj_obj_ellipsoid_get_parameters(PJ_OBJ *ellipsoid,
                                      double *pSemiMajorMetre,
                                      double *pSemiMinorMetre,
                                      int *pIsSemiMinorComputed,
                                      double *pInverseFlattening) {
    assert(ellipsoid);
    auto l_ellipsoid = nn_dynamic_pointer_cast<Ellipsoid>(ellipsoid->obj);
    if (!l_ellipsoid) {
        proj_log_error(ellipsoid->ctx, __FUNCTION__,
                       "Object is not a Ellipsoid");
        return FALSE;
    }

    if (pSemiMajorMetre) {
        *pSemiMajorMetre = l_ellipsoid->semiMajorAxis().getSIValue();
    }
    if (pSemiMinorMetre) {
        *pSemiMinorMetre = l_ellipsoid->computeSemiMinorAxis().getSIValue();
    }
    if (pIsSemiMinorComputed) {
        *pIsSemiMinorComputed = !(l_ellipsoid->semiMinorAxis().has_value());
    }
    if (pInverseFlattening) {
        *pInverseFlattening =
            l_ellipsoid->computeInverseFlattening().getSIValue();
    }
    return TRUE;
}

// ---------------------------------------------------------------------------

/** \brief Return the base CRS of a BoundCRS or the source CRS of a
 * CoordinateOperation.
 *
 * The returned object must be unreferenced with proj_obj_unref() after
 * use.
 *
 * @param obj Objet of type BoundCRS or CoordinateOperation (must not be NULL)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL
 * in case of error, or missing source CRS.
 */
PJ_OBJ *proj_obj_get_source_crs(PJ_OBJ *obj) {
    assert(obj);
    auto boundCRS = nn_dynamic_pointer_cast<BoundCRS>(obj->obj);
    if (boundCRS) {
        return new PJ_OBJ(obj->ctx, boundCRS->baseCRS());
    }
    auto co = nn_dynamic_pointer_cast<CoordinateOperation>(obj->obj);
    if (co) {
        auto sourceCRS = co->sourceCRS();
        if (sourceCRS) {
            return new PJ_OBJ(obj->ctx, NN_CHECK_ASSERT(sourceCRS));
        }
        return nullptr;
    }
    proj_log_error(obj->ctx, __FUNCTION__,
                   "Object is not a BoundCRS or a CoordinateOperation");
    return nullptr;
}

// ---------------------------------------------------------------------------

/** \brief Return the hub CRS of a BoundCRS or the target CRS of a
 * CoordinateOperation.
 *
 * The returned object must be unreferenced with proj_obj_unref() after
 * use.
 *
 * @param obj Objet of type BoundCRS or CoordinateOperation (must not be NULL)
 * @return Object that must be unreferenced with proj_obj_unref(), or NULL
 * in case of error, or missing target CRS.
 */
PJ_OBJ *proj_obj_get_target_crs(PJ_OBJ *obj) {
    assert(obj);
    auto boundCRS = nn_dynamic_pointer_cast<BoundCRS>(obj->obj);
    if (boundCRS) {
        return new PJ_OBJ(obj->ctx, boundCRS->hubCRS());
    }
    auto co = nn_dynamic_pointer_cast<CoordinateOperation>(obj->obj);
    if (co) {
        auto targetCRS = co->targetCRS();
        if (targetCRS) {
            return new PJ_OBJ(obj->ctx, NN_CHECK_ASSERT(targetCRS));
        }
        return nullptr;
    }
    proj_log_error(obj->ctx, __FUNCTION__,
                   "Object is not a BoundCRS or a CoordinateOperation");
    return nullptr;
}

// ---------------------------------------------------------------------------

static PROJ_STRING_LIST set_to_string_list(const std::set<std::string> &set) {
    auto ret = new char *[set.size() + 1];
    size_t i = 0;
    for (const auto &str : set) {
        ret[i] = new char[str.size() + 1];
        std::memcpy(ret[i], str.c_str(), str.size() + 1);
        i++;
    }
    ret[i] = nullptr;
    return ret;
}

// ---------------------------------------------------------------------------

/** \brief Return the list of authorities used in the database.
 *
 * The returned list is NULL terminated and must be freed with
 * proj_free_string_list().
 *
 * @param ctx PROJ context, or NULL for default context
 *
 * @return a NULL terminated list of NUL-terminated strings that must be
 * freed with proj_free_string_list(), or NULL in case of error.
 */
PROJ_STRING_LIST proj_get_authorities_from_database(PJ_CONTEXT *ctx) {
    try {
        if (ctx->cpp_context == nullptr) {
            ctx->cpp_context = new projCppContext(DatabaseContext::create());
        }
        return set_to_string_list(
            ctx->cpp_context->databaseContext->getAuthorities());
    } catch (const std::exception &e) {
        proj_log_error(ctx, __FUNCTION__, e.what());
    }
    return nullptr;
}

// ---------------------------------------------------------------------------

/** \brief Returns the set of authority codes of the given object type.
 *
 * The returned list is NULL terminated and must be freed with
 * proj_free_string_list().
 *
 * @param ctx PROJ context, or NULL for default context.
 * @param auth_name Authority name (must not be NULL)
 * @param type Object type.
 * @param allow_deprecated whether we should return deprecated objects as well.
 *
 * @return a NULL terminated list of NUL-terminated strings that must be
 * freed with proj_free_string_list(), or NULL in case of error.
 */
PROJ_STRING_LIST proj_get_codes_from_database(PJ_CONTEXT *ctx,
                                              const char *auth_name,
                                              PJ_OBJ_TYPE type,
                                              int allow_deprecated) {
    assert(auth_name);
    try {
        if (ctx->cpp_context == nullptr) {
            ctx->cpp_context = new projCppContext(DatabaseContext::create());
        }
        auto factory = AuthorityFactory::create(
            ctx->cpp_context->databaseContext, auth_name);

        AuthorityFactory::ObjectType typeInternal =
            AuthorityFactory::ObjectType::CRS;
        switch (type) {
        case PJ_OBJ_TYPE_ELLIPSOID:
            typeInternal = AuthorityFactory::ObjectType::ELLIPSOID;
            break;

        case PJ_OBJ_TYPE_GEODETIC_REFERENCE_FRAME:
        case PJ_OBJ_TYPE_DYNAMIC_GEODETIC_REFERENCE_FRAME:
            typeInternal =
                AuthorityFactory::ObjectType::GEODETIC_REFERENCE_FRAME;
            break;

        case PJ_OBJ_TYPE_VERTICAL_REFERENCE_FRAME:
        case PJ_OBJ_TYPE_DYNAMIC_VERTICAL_REFERENCE_FRAME:
            typeInternal =
                AuthorityFactory::ObjectType::VERTICAL_REFERENCE_FRAME;
            break;

        case PJ_OBJ_TYPE_DATUM_ENSEMBLE:
            typeInternal = AuthorityFactory::ObjectType::DATUM;
            break;

        case PJ_OBJ_TYPE_GEODETIC_CRS:
            typeInternal = AuthorityFactory::ObjectType::GEODETIC_CRS;
            break;

        case PJ_OBJ_TYPE_GEOGRAPHIC_CRS:
            typeInternal = AuthorityFactory::ObjectType::GEOGRAPHIC_CRS;
            break;

        case PJ_OBJ_TYPE_VERTICAL_CRS:
            typeInternal = AuthorityFactory::ObjectType::VERTICAL_CRS;
            break;

        case PJ_OBJ_TYPE_PROJECTED_CRS:
            typeInternal = AuthorityFactory::ObjectType::PROJECTED_CRS;
            break;

        case PJ_OBJ_TYPE_COMPOUND_CRS:
            typeInternal = AuthorityFactory::ObjectType::COMPOUND_CRS;
            break;

        case PJ_OBJ_TYPE_TEMPORAL_CRS:
            return nullptr;

        case PJ_OBJ_TYPE_BOUND_CRS:
            return nullptr;

        case PJ_OBJ_TYPE_OTHER_CRS:
            typeInternal = AuthorityFactory::ObjectType::CRS;
            break;

        case PJ_OBJ_TYPE_CONVERSION:
            typeInternal = AuthorityFactory::ObjectType::CONVERSION;
            break;

        case PJ_OBJ_TYPE_TRANSFORMATION:
            typeInternal = AuthorityFactory::ObjectType::TRANSFORMATION;
            break;

        case PJ_OBJ_TYPE_CONCATENATED_OPERATION:
            typeInternal = AuthorityFactory::ObjectType::CONCATENATED_OPERATION;
            break;

        case PJ_OBJ_TYPE_OTHER_COORDINATE_OPERATION:
            typeInternal = AuthorityFactory::ObjectType::COORDINATE_OPERATION;
            break;

        case PJ_OBJ_TYPE_UNKNOWN:
            return nullptr;
        }

        return set_to_string_list(
            factory->getAuthorityCodes(typeInternal, allow_deprecated != 0));

    } catch (const std::exception &e) {
        proj_log_error(ctx, __FUNCTION__, e.what());
    }
    return nullptr;
}

// ---------------------------------------------------------------------------

/** Free a list of NULL terminated strings. */
void proj_free_string_list(PROJ_STRING_LIST list) {
    if (list) {
        for (size_t i = 0; list[i] != nullptr; i++) {
            delete[] list[i];
        }
        delete[] list;
    }
}
