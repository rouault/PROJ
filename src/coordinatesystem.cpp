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
#define FROM_PROJ_CPP
#endif

#include "proj/coordinatesystem.hpp"
#include "proj/common.hpp"
#include "proj/io.hpp"
#include "proj/metadata.hpp"
#include "proj/util.hpp"

#include "proj/internal/coordinatesystem_internal.hpp"
#include "proj/internal/internal.hpp"
#include "proj/internal/io_internal.hpp"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace NS_PROJ::internal;

NS_PROJ_START
namespace cs {

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct Meridian::Private {
    common::Angle longitude_{};

    explicit Private(const common::Angle &longitude) : longitude_(longitude) {}
};
//! @endcond

// ---------------------------------------------------------------------------

Meridian::Meridian(const common::Angle &longitudeIn)
    : d(internal::make_unique<Private>(longitudeIn)) {}

// ---------------------------------------------------------------------------

#ifdef notdef
Meridian::Meridian(const Meridian &other)
    : IdentifiedObject(other), d(internal::make_unique<Private>(*other.d)) {}
#endif

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
Meridian::~Meridian() = default;
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Return the longitude of the meridian that the axis follows from the
 * pole.
 *
 * @return the longitude.
 */
const common::Angle &Meridian::longitude() PROJ_CONST_DEFN {
    return d->longitude_;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a Meridian.
 *
 * @param longitudeIn longitude of the meridian that the axis follows from the
 * pole.
 * @return new Meridian.
 */
MeridianNNPtr Meridian::create(const common::Angle &longitudeIn) {
    return Meridian::nn_make_shared<Meridian>(longitudeIn);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void Meridian::_exportToWKT(
    io::WKTFormatter *formatter) const // throw(FormattingException)
{
    formatter->startNode(io::WKTConstants::MERIDIAN, !identifiers().empty());
    formatter->add(longitude().value());
    longitude().unit()._exportToWKT(formatter, io::WKTConstants::ANGLEUNIT);
    if (formatter->outputId()) {
        formatID(formatter);
    }
    formatter->endNode();
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct CoordinateSystemAxis::Private {
    std::string abbreviation{};
    const AxisDirection *direction = &(AxisDirection::UNSPECIFIED);
    common::UnitOfMeasure unit{};
    util::optional<double> minimumValue{};
    util::optional<double> maximumValue{};
    MeridianPtr meridian{};
    // TODO rangeMeaning
};
//! @endcond

// ---------------------------------------------------------------------------

CoordinateSystemAxis::CoordinateSystemAxis()
    : d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

#ifdef notdef
CoordinateSystemAxis::CoordinateSystemAxis(const CoordinateSystemAxis &other)
    : IdentifiedObject(other), d(internal::make_unique<Private>(*other.d)) {}
#endif

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
CoordinateSystemAxis::~CoordinateSystemAxis() = default;
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Return the axis abbreviation.
 *
 * The abbreviation used for this coordinate system axis; this abbreviation
 * is also used to identify the coordinates in the coordinate tuple.
 * Examples are X and Y.
 *
 * @return the abbreviation.
 */
const std::string &CoordinateSystemAxis::abbreviation() PROJ_CONST_DEFN {
    return d->abbreviation;
}

// ---------------------------------------------------------------------------

/** \brief Return the axis direction.
 *
 * The direction of this coordinate system axis (or in the case of Cartesian
 * projected coordinates, the direction of this coordinate system axis locally)
 * Examples: north or south, east or west, up or down. Within any set of
 * coordinate system axes, only one of each pair of terms can be used. For
 * Earth-fixed CRSs, this direction is often approximate and intended to
 * provide a human interpretable meaning to the axis. When a geodetic reference
 * frame is used, the precise directions of the axes may therefore vary
 * slightly from this approximate direction. Note that an EngineeringCRS often
 * requires specific descriptions of the directions of its coordinate system
 * axes.
 *
 * @return the direction.
 */
const AxisDirection &CoordinateSystemAxis::direction() PROJ_CONST_DEFN {
    return *(d->direction);
}

// ---------------------------------------------------------------------------

/** \brief Return the axis unit.
 *
 * This is the spatial unit or temporal quantity used for this coordinate
 * system axis. The value of a coordinate in a coordinate tuple shall be
 * recorded using this unit.
 *
 * @return the axis unit.
 */
const common::UnitOfMeasure &CoordinateSystemAxis::unit() PROJ_CONST_DEFN {
    return d->unit;
}

// ---------------------------------------------------------------------------

/** \brief Return the minimum value normally allowed for this axis, in the unit
 * for the axis.
 *
 * @return the minimum value, or empty.
 */
const util::optional<double> &
CoordinateSystemAxis::minimumValue() PROJ_CONST_DEFN {
    return d->minimumValue;
}

// ---------------------------------------------------------------------------

/** \brief Return the maximum value normally allowed for this axis, in the unit
 * for the axis.
 *
 * @return the maximum value, or empty.
 */
const util::optional<double> &
CoordinateSystemAxis::maximumValue() PROJ_CONST_DEFN {
    return d->maximumValue;
}

// ---------------------------------------------------------------------------

/** \brief Return the meridian that the axis follows from the pole, for a
 * coordinate
 * reference system centered on a pole.
 *
 * @return the meridian, or null.
 */
const MeridianPtr &CoordinateSystemAxis::meridian() PROJ_CONST_DEFN {
    return d->meridian;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a CoordinateSystemAxis.
 *
 * @param properties See \ref general_properties. The name should generally be
 * defined.
 * @param abbreviationIn Axis abbreviation (might be empty)
 * @param directionIn Axis direction
 * @param unitIn Axis unit
 * @param meridianIn The meridian that the axis follows from the pole, for a
 * coordinate
 * reference system centered on a pole, or nullptr
 * @return a new CoordinateSystemAxis.
 */
CoordinateSystemAxisNNPtr CoordinateSystemAxis::create(
    const util::PropertyMap &properties, const std::string &abbreviationIn,
    const AxisDirection &directionIn, const common::UnitOfMeasure &unitIn,
    const MeridianPtr &meridianIn) {
    auto csa(CoordinateSystemAxis::nn_make_shared<CoordinateSystemAxis>());
    csa->setProperties(properties);
    csa->d->abbreviation = abbreviationIn;
    csa->d->direction = &directionIn;
    csa->d->unit = unitIn;
    csa->d->meridian = meridianIn;
    return csa;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void CoordinateSystemAxis::_exportToWKT(
    // cppcheck-suppress passedByValue
    io::WKTFormatter *formatter) const // throw(FormattingException)
{
    _exportToWKT(formatter, 0, false);
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
std::string CoordinateSystemAxis::normalizeAxisName(const std::string &str) {
    if (str.empty()) {
        return str;
    }
    // on import, transform from WKT2 "longitude" to "Longitude", as in the
    // EPSG database.
    return toupper(str.substr(0, 1)) + str.substr(1);
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void CoordinateSystemAxis::_exportToWKT(io::WKTFormatter *formatter, int order,
                                        bool disableAbbrev) const {
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;
    formatter->startNode(io::WKTConstants::AXIS, !identifiers().empty());
    std::string axisName = *(name()->description());
    std::string abbrev = abbreviation();
    std::string parenthesedAbbrev = "(" + abbrev + ")";
    std::string dir = direction().toString();
    std::string axisDesignation;

    // It seems that the convention in WKT2 for axis name is first letter in
    // lower case. Whereas in WKT1 GDAL, it is in upper case (as in the EPSG
    // database)
    if (!axisName.empty()) {
        if (isWKT2) {
            axisDesignation =
                tolower(axisName.substr(0, 1)) + axisName.substr(1);
        } else {
            axisDesignation = axisName;
        }
    }

    if (!disableAbbrev && isWKT2 &&
        // For geodetic CS, export the axis name without abbreviation
        !(axisName == AxisName::Latitude || axisName == AxisName::Longitude)) {
        if (!axisDesignation.empty() && !abbrev.empty()) {
            axisDesignation += " ";
        }
        if (!abbrev.empty()) {
            axisDesignation += parenthesedAbbrev;
        }
    }
    if (!isWKT2) {
        dir = toupper(dir);

        if (direction() == AxisDirection::GEOCENTRIC_Z) {
            dir = AxisDirectionWKT1::NORTH;
        } else if (AxisDirectionWKT1::valueOf(dir) == nullptr) {
            dir = AxisDirectionWKT1::OTHER;
        }
    } else if (!abbrev.empty()) {
        // For geocentric CS, just put the abbreviation
        if (direction() == AxisDirection::GEOCENTRIC_X ||
            direction() == AxisDirection::GEOCENTRIC_Y ||
            direction() == AxisDirection::GEOCENTRIC_Z) {
            axisDesignation = parenthesedAbbrev;
        }
        // For cartesian CS with Easting/Northing, export only the abbreviation
        else if ((order == 1 && axisName == AxisName::Easting &&
                  abbrev == AxisAbbreviation::E) ||
                 (order == 2 && axisName == AxisName::Northing &&
                  abbrev == AxisAbbreviation::N)) {
            axisDesignation = parenthesedAbbrev;
        }
    }
    formatter->addQuotedString(axisDesignation);
    formatter->add(dir);
    if (meridian()) {
        meridian()->_exportToWKT(formatter);
    }
    if (formatter->outputAxisOrder() && order > 0) {
        formatter->startNode(io::WKTConstants::ORDER, false);
        formatter->add(order);
        formatter->endNode();
    }
    if (formatter->outputUnit() &&
        unit().type() != common::UnitOfMeasure::Type::NONE) {
        unit()._exportToWKT(formatter);
    }
    if (formatter->outputId()) {
        formatID(formatter);
    }
    formatter->endNode();
}
//! @endcond

// ---------------------------------------------------------------------------

bool CoordinateSystemAxis::isEquivalentTo(
    const util::IComparable *other,
    util::IComparable::Criterion criterion) const {
    auto otherCSA = dynamic_cast<const CoordinateSystemAxis *>(other);
    if (otherCSA == nullptr) {
        return false;
    }
    // For approximate comparison, only care about axis direction and unit.
    if (!(*(d->direction) == *(otherCSA->d->direction) &&
          d->unit == otherCSA->d->unit)) {
        return false;
    }
    if (criterion == util::IComparable::Criterion::STRICT) {
        if (!IdentifiedObject::isEquivalentTo(other, criterion)) {
            return false;
        }
        if (abbreviation() != otherCSA->abbreviation()) {
            return false;
        }
        // TODO other metadata
    }

    return true;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct CoordinateSystem::Private {
    std::vector<CoordinateSystemAxisNNPtr> axisList{};

    explicit Private(const std::vector<CoordinateSystemAxisNNPtr> &axisListIn)
        : axisList(axisListIn) {}
};
//! @endcond

// ---------------------------------------------------------------------------

CoordinateSystem::CoordinateSystem(
    const std::vector<CoordinateSystemAxisNNPtr> &axisIn)
    : d(internal::make_unique<Private>(axisIn)) {}

// ---------------------------------------------------------------------------

#ifdef notdef
CoordinateSystem::CoordinateSystem(const CoordinateSystem &other)
    : IdentifiedObject(other), d(internal::make_unique<Private>(*other.d)) {}
#endif

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
CoordinateSystem::~CoordinateSystem() = default;
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Return the list of axes of this coordinate system.
 *
 * @return the axes.
 */
const std::vector<CoordinateSystemAxisNNPtr> &
CoordinateSystem::axisList() PROJ_CONST_DEFN {
    return d->axisList;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void CoordinateSystem::_exportToWKT(
    io::WKTFormatter *formatter) const // throw(FormattingException)
{
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;

    const auto &l_axisList = axisList();
    if (isWKT2) {
        formatter->startNode(io::WKTConstants::CS, !identifiers().empty());
        formatter->add(getWKT2Type(formatter->use2018Keywords()));
        formatter->add(l_axisList.size());
        formatter->endNode();
        formatter->startNode(std::string(),
                             false); // anonymous indentation level
    }

    common::UnitOfMeasure unit = common::UnitOfMeasure::NONE;
    bool bAllSameUnit = true;
    bool bFirstUnit = true;
    for (const auto &axis : l_axisList) {
        const auto &l_unit = axis->unit();
        if (bFirstUnit) {
            unit = l_unit;
            bFirstUnit = false;
        } else if (unit != l_unit) {
            bAllSameUnit = false;
        }
    }

    formatter->pushOutputUnit(
        isWKT2 && (!bAllSameUnit || !formatter->outputCSUnitOnlyOnceIfSame()));

    int order = 1;
    const bool disableAbbrev =
        (l_axisList.size() == 3 &&
         l_axisList[0]->nameStr() == AxisName::Latitude &&
         l_axisList[1]->nameStr() == AxisName::Longitude &&
         l_axisList[2]->nameStr() == AxisName::Ellipsoidal_height);

    for (auto &axis : l_axisList) {
        int axisOrder = (isWKT2 && l_axisList.size() > 1) ? order : 0;
        axis->_exportToWKT(formatter, axisOrder, disableAbbrev);
        order++;
    }
    if (isWKT2 && !l_axisList.empty() && bAllSameUnit &&
        formatter->outputCSUnitOnlyOnceIfSame()) {
        unit._exportToWKT(formatter);
    }

    formatter->popOutputUnit();

    if (isWKT2) {
        formatter->endNode();
    }
}
//! @endcond

// ---------------------------------------------------------------------------

bool CoordinateSystem::isEquivalentTo(
    const util::IComparable *other,
    util::IComparable::Criterion criterion) const {
    auto otherCS = dynamic_cast<const CoordinateSystem *>(other);
    if (otherCS == nullptr ||
        !IdentifiedObject::isEquivalentTo(other, criterion)) {
        return false;
    }
    const auto &list = axisList();
    const auto &otherList = otherCS->axisList();
    if (list.size() != otherList.size()) {
        return false;
    }
    if (getWKT2Type(true) != otherCS->getWKT2Type(true)) {
        return false;
    }
    for (size_t i = 0; i < list.size(); i++) {
        if (!list[i]->isEquivalentTo(otherList[i].get(), criterion)) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
SphericalCS::~SphericalCS() = default;
//! @endcond

// ---------------------------------------------------------------------------

SphericalCS::SphericalCS(const std::vector<CoordinateSystemAxisNNPtr> &axisIn)
    : CoordinateSystem(axisIn) {}

// ---------------------------------------------------------------------------

#ifdef notdef
SphericalCS::SphericalCS(const SphericalCS &) = default;
#endif

// ---------------------------------------------------------------------------

/** \brief Instanciate a SphericalCS.
 *
 * @param properties See \ref general_properties.
 * @param axis1 The first axis.
 * @param axis2 The second axis.
 * @param axis3 The third axis.
 * @return a new SphericalCS.
 */
SphericalCSNNPtr SphericalCS::create(const util::PropertyMap &properties,
                                     const CoordinateSystemAxisNNPtr &axis1,
                                     const CoordinateSystemAxisNNPtr &axis2,
                                     const CoordinateSystemAxisNNPtr &axis3) {
    std::vector<CoordinateSystemAxisNNPtr> axis{axis1, axis2, axis3};
    auto cs(SphericalCS::nn_make_shared<SphericalCS>(axis));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
EllipsoidalCS::~EllipsoidalCS() = default;
//! @endcond

// ---------------------------------------------------------------------------

EllipsoidalCS::EllipsoidalCS(
    const std::vector<CoordinateSystemAxisNNPtr> &axisIn)
    : CoordinateSystem(axisIn) {}

// ---------------------------------------------------------------------------

#ifdef notdef
EllipsoidalCS::EllipsoidalCS(const EllipsoidalCS &) = default;
#endif

// ---------------------------------------------------------------------------

/** \brief Instanciate a EllipsoidalCS.
 *
 * @param properties See \ref general_properties.
 * @param axis1 The first axis.
 * @param axis2 The second axis.
 * @return a new EllipsoidalCS.
 */
EllipsoidalCSNNPtr
EllipsoidalCS::create(const util::PropertyMap &properties,
                      const CoordinateSystemAxisNNPtr &axis1,
                      const CoordinateSystemAxisNNPtr &axis2) {
    std::vector<CoordinateSystemAxisNNPtr> axis{axis1, axis2};
    auto cs(EllipsoidalCS::nn_make_shared<EllipsoidalCS>(axis));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a EllipsoidalCS.
 *
 * @param properties See \ref general_properties.
 * @param axis1 The first axis.
 * @param axis2 The second axis.
 * @param axis3 The third axis.
 * @return a new EllipsoidalCS.
 */
EllipsoidalCSNNPtr
EllipsoidalCS::create(const util::PropertyMap &properties,
                      const CoordinateSystemAxisNNPtr &axis1,
                      const CoordinateSystemAxisNNPtr &axis2,
                      const CoordinateSystemAxisNNPtr &axis3) {
    std::vector<CoordinateSystemAxisNNPtr> axis{axis1, axis2, axis3};
    auto cs(EllipsoidalCS::nn_make_shared<EllipsoidalCS>(axis));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a EllipsoidalCS with a Latitude (first) and Longitude
 * (second) axis.
 *
 * @param unit Angular unit of the axes.
 * @return a new EllipsoidalCS.
 */
EllipsoidalCSNNPtr
EllipsoidalCS::createLatitudeLongitude(const common::UnitOfMeasure &unit) {
    std::vector<CoordinateSystemAxisNNPtr> axis{
        CoordinateSystemAxis::create(
            util::PropertyMap().set(IdentifiedObject::NAME_KEY,
                                    AxisName::Latitude),
            AxisAbbreviation::lat, AxisDirection::NORTH, unit),
        CoordinateSystemAxis::create(
            util::PropertyMap().set(IdentifiedObject::NAME_KEY,
                                    AxisName::Longitude),
            AxisAbbreviation::lon, AxisDirection::EAST, unit)};
    auto cs(EllipsoidalCS::nn_make_shared<EllipsoidalCS>(axis));
    return cs;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a EllipsoidalCS with a Latitude (first), Longitude
 * (second) axis and ellipsoidal height (third) axis.
 *
 * @param angularUnit Angular unit of the latitude and longitude axes.
 * @param linearUnit Linear unit of the ellipsoidal height axis.
 * @return a new EllipsoidalCS.
 */
EllipsoidalCSNNPtr EllipsoidalCS::createLatitudeLongitudeEllipsoidalHeight(
    const common::UnitOfMeasure &angularUnit,
    const common::UnitOfMeasure &linearUnit) {
    std::vector<CoordinateSystemAxisNNPtr> axis{
        CoordinateSystemAxis::create(
            util::PropertyMap().set(IdentifiedObject::NAME_KEY,
                                    AxisName::Latitude),
            AxisAbbreviation::lat, AxisDirection::NORTH, angularUnit),
        CoordinateSystemAxis::create(
            util::PropertyMap().set(IdentifiedObject::NAME_KEY,
                                    AxisName::Longitude),
            AxisAbbreviation::lon, AxisDirection::EAST, angularUnit),
        CoordinateSystemAxis::create(
            util::PropertyMap().set(IdentifiedObject::NAME_KEY,
                                    AxisName::Ellipsoidal_height),
            AxisAbbreviation::h, AxisDirection::UP, linearUnit)};
    auto cs(EllipsoidalCS::nn_make_shared<EllipsoidalCS>(axis));
    return cs;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a EllipsoidalCS with a Longitude (first) and Latitude
 * (second) axis.
 *
 * @param unit Angular unit of the axes.
 * @return a new EllipsoidalCS.
 */
EllipsoidalCSNNPtr
EllipsoidalCS::createLongitudeLatitude(const common::UnitOfMeasure &unit) {
    std::vector<CoordinateSystemAxisNNPtr> axis{
        CoordinateSystemAxis::create(
            util::PropertyMap().set(IdentifiedObject::NAME_KEY,
                                    AxisName::Longitude),
            AxisAbbreviation::lon, AxisDirection::EAST, unit),
        CoordinateSystemAxis::create(
            util::PropertyMap().set(IdentifiedObject::NAME_KEY,
                                    AxisName::Latitude),
            AxisAbbreviation::lat, AxisDirection::NORTH, unit)};
    auto cs(EllipsoidalCS::nn_make_shared<EllipsoidalCS>(axis));
    return cs;
}

// ---------------------------------------------------------------------------

/** \brief Return the axis order in an enumerated way. */
EllipsoidalCS::AxisOrder EllipsoidalCS::axisOrder() const {
    const auto &l_axisList = CoordinateSystem::getPrivate()->axisList;
    const auto &dir0 = l_axisList[0]->direction();
    const auto &dir1 = l_axisList[1]->direction();
    if (&dir0 == &AxisDirection::NORTH && &dir1 == &AxisDirection::EAST) {
        if (l_axisList.size() == 2) {
            return AxisOrder::LAT_NORTH_LONG_EAST;
        } else if (&l_axisList[2]->direction() == &AxisDirection::UP) {
            return AxisOrder::LAT_NORTH_LONG_EAST_HEIGHT_UP;
        }
    } else if (&dir0 == &AxisDirection::EAST &&
               &dir1 == &AxisDirection::NORTH) {
        if (l_axisList.size() == 2) {
            return AxisOrder::LONG_EAST_LAT_NORTH;
        } else if (&l_axisList[2]->direction() == &AxisDirection::UP) {
            return AxisOrder::LONG_EAST_LAT_NORTH_HEIGHT_UP;
        }
    }

    return AxisOrder::OTHER;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
VerticalCS::~VerticalCS() = default;
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
VerticalCS::VerticalCS(const CoordinateSystemAxisNNPtr &axisIn)
    : CoordinateSystem(std::vector<CoordinateSystemAxisNNPtr>{axisIn}) {}
//! @endcond

// ---------------------------------------------------------------------------

#ifdef notdef
VerticalCS::VerticalCS(const VerticalCS &) = default;
#endif

// ---------------------------------------------------------------------------

/** \brief Instanciate a VerticalCS.
 *
 * @param properties See \ref general_properties.
 * @param axis The axis.
 * @return a new VerticalCS.
 */
VerticalCSNNPtr VerticalCS::create(const util::PropertyMap &properties,
                                   const CoordinateSystemAxisNNPtr &axis) {
    auto cs(VerticalCS::nn_make_shared<VerticalCS>(axis));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a VerticalCS with a Gravity-related height axis
 *
 * @param unit linear unit.
 * @return a new VerticalCS.
 */
VerticalCSNNPtr
VerticalCS::createGravityRelatedHeight(const common::UnitOfMeasure &unit) {
    auto cs(VerticalCS::nn_make_shared<VerticalCS>(CoordinateSystemAxis::create(
        util::PropertyMap().set(IdentifiedObject::NAME_KEY,
                                "Gravity-related height"),
        "H", AxisDirection::UP, unit)));
    return cs;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
CartesianCS::~CartesianCS() = default;
//! @endcond

// ---------------------------------------------------------------------------

CartesianCS::CartesianCS(const std::vector<CoordinateSystemAxisNNPtr> &axisIn)
    : CoordinateSystem(axisIn) {}

// ---------------------------------------------------------------------------

#ifdef notdef
CartesianCS::CartesianCS(const CartesianCS &) = default;
#endif

// ---------------------------------------------------------------------------

/** \brief Instanciate a CartesianCS.
 *
 * @param properties See \ref general_properties.
 * @param axis1 The first axis.
 * @param axis2 The second axis.
 * @return a new CartesianCS.
 */
CartesianCSNNPtr CartesianCS::create(const util::PropertyMap &properties,
                                     const CoordinateSystemAxisNNPtr &axis1,
                                     const CoordinateSystemAxisNNPtr &axis2) {
    std::vector<CoordinateSystemAxisNNPtr> axis{axis1, axis2};
    auto cs(CartesianCS::nn_make_shared<CartesianCS>(axis));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a CartesianCS.
 *
 * @param properties See \ref general_properties.
 * @param axis1 The first axis.
 * @param axis2 The second axis.
 * @param axis3 The third axis.
 * @return a new CartesianCS.
 */
CartesianCSNNPtr CartesianCS::create(const util::PropertyMap &properties,
                                     const CoordinateSystemAxisNNPtr &axis1,
                                     const CoordinateSystemAxisNNPtr &axis2,
                                     const CoordinateSystemAxisNNPtr &axis3) {
    std::vector<CoordinateSystemAxisNNPtr> axis{axis1, axis2, axis3};
    auto cs(CartesianCS::nn_make_shared<CartesianCS>(axis));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a CartesianCS with a Easting (first) and Northing
 * (second) axis.
 *
 * @param unit Linear unit of the axes.
 * @return a new CartesianCS.
 */
CartesianCSNNPtr
CartesianCS::createEastingNorthing(const common::UnitOfMeasure &unit) {
    std::vector<CoordinateSystemAxisNNPtr> axis{
        CoordinateSystemAxis::create(
            util::PropertyMap().set(IdentifiedObject::NAME_KEY,
                                    AxisName::Easting),
            AxisAbbreviation::E, AxisDirection::EAST, unit),
        CoordinateSystemAxis::create(
            util::PropertyMap().set(IdentifiedObject::NAME_KEY,
                                    AxisName::Northing),
            AxisAbbreviation::N, AxisDirection::NORTH, unit)};
    auto cs(CartesianCS::nn_make_shared<CartesianCS>(axis));
    return cs;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a CartesianCS with the three geocentric axes.
 *
 * @param unit Liinear unit of the axes.
 * @return a new CartesianCS.
 */
CartesianCSNNPtr
CartesianCS::createGeocentric(const common::UnitOfMeasure &unit) {
    std::vector<CoordinateSystemAxisNNPtr> axis{
        CoordinateSystemAxis::create(
            util::PropertyMap().set(IdentifiedObject::NAME_KEY,
                                    AxisName::Geocentric_X),
            AxisAbbreviation::X, AxisDirection::GEOCENTRIC_X, unit),
        CoordinateSystemAxis::create(
            util::PropertyMap().set(IdentifiedObject::NAME_KEY,
                                    AxisName::Geocentric_Y),
            AxisAbbreviation::Y, AxisDirection::GEOCENTRIC_Y, unit),
        CoordinateSystemAxis::create(
            util::PropertyMap().set(IdentifiedObject::NAME_KEY,
                                    AxisName::Geocentric_Z),
            AxisAbbreviation::Z, AxisDirection::GEOCENTRIC_Z, unit)};
    auto cs(CartesianCS::nn_make_shared<CartesianCS>(axis));
    return cs;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
OrdinalCS::~OrdinalCS() = default;
//! @endcond

// ---------------------------------------------------------------------------

OrdinalCS::OrdinalCS(const std::vector<CoordinateSystemAxisNNPtr> &axisIn)
    : CoordinateSystem(axisIn) {}

// ---------------------------------------------------------------------------

#ifdef notdef
OrdinalCS::OrdinalCS(const OrdinalCS &) = default;
#endif

// ---------------------------------------------------------------------------

/** \brief Instanciate a OrdinalCS.
 *
 * @param properties See \ref general_properties.
 * @param axisIn List of axis.
 * @return a new OrdinalCS.
 */
OrdinalCSNNPtr
OrdinalCS::create(const util::PropertyMap &properties,
                  const std::vector<CoordinateSystemAxisNNPtr> &axisIn) {
    auto cs(OrdinalCS::nn_make_shared<OrdinalCS>(axisIn));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
ParametricCS::~ParametricCS() = default;
//! @endcond

// ---------------------------------------------------------------------------

ParametricCS::ParametricCS(const std::vector<CoordinateSystemAxisNNPtr> &axisIn)
    : CoordinateSystem(axisIn) {}

// ---------------------------------------------------------------------------

#ifdef notdef
ParametricCS::ParametricCS(const ParametricCS &) = default;
#endif

// ---------------------------------------------------------------------------

/** \brief Instanciate a ParametricCS.
 *
 * @param properties See \ref general_properties.
 * @param axisIn Axis.
 * @return a new ParametricCS.
 */
ParametricCSNNPtr
ParametricCS::create(const util::PropertyMap &properties,
                     const CoordinateSystemAxisNNPtr &axisIn) {
    auto cs(ParametricCS::nn_make_shared<ParametricCS>(
        std::vector<CoordinateSystemAxisNNPtr>{axisIn}));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

AxisDirection::AxisDirection(const std::string &nameIn) : CodeList(nameIn) {
    assert(keys.find(nameIn) == keys.end());
    registry[nameIn] = this;
    keys.insert(nameIn);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
const AxisDirection *AxisDirection::valueOf(const std::string &nameIn) {
    auto iter = registry.find(nameIn);
    if (iter == registry.end())
        return nullptr;
    return iter->second;
}

// ---------------------------------------------------------------------------

const std::set<std::string> &AxisDirection::getKeys() { return keys; }
//! @endcond

//! @cond Doxygen_Suppress
// ---------------------------------------------------------------------------

AxisDirectionWKT1::AxisDirectionWKT1(const std::string &nameIn)
    : CodeList(nameIn) {
    assert(keys.find(nameIn) == keys.end());
    registry[nameIn] = this;
    keys.insert(nameIn);
}

// ---------------------------------------------------------------------------

const AxisDirectionWKT1 *AxisDirectionWKT1::valueOf(const std::string &nameIn) {
    auto iter = registry.find(nameIn);
    if (iter == registry.end())
        return nullptr;
    return iter->second;
}

// ---------------------------------------------------------------------------

const std::set<std::string> &AxisDirectionWKT1::getKeys() { return keys; }
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
TemporalCS::~TemporalCS() = default;
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
TemporalCS::TemporalCS(const CoordinateSystemAxisNNPtr &axisIn)
    : CoordinateSystem(std::vector<CoordinateSystemAxisNNPtr>{axisIn}) {}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
DateTimeTemporalCS::~DateTimeTemporalCS() = default;
//! @endcond

// ---------------------------------------------------------------------------

DateTimeTemporalCS::DateTimeTemporalCS(const CoordinateSystemAxisNNPtr &axisIn)
    : TemporalCS(axisIn) {}

// ---------------------------------------------------------------------------

/** \brief Instanciate a DateTimeTemporalCS.
 *
 * @param properties See \ref general_properties.
 * @param axisIn The axis.
 * @return a new DateTimeTemporalCS.
 */
DateTimeTemporalCSNNPtr
DateTimeTemporalCS::create(const util::PropertyMap &properties,
                           const CoordinateSystemAxisNNPtr &axisIn) {
    auto cs(DateTimeTemporalCS::nn_make_shared<DateTimeTemporalCS>(axisIn));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

std::string DateTimeTemporalCS::getWKT2Type(bool use2018Keywords) const {
    return use2018Keywords ? "TemporalDateTime" : "temporal";
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
TemporalCountCS::~TemporalCountCS() = default;
//! @endcond

// ---------------------------------------------------------------------------

TemporalCountCS::TemporalCountCS(const CoordinateSystemAxisNNPtr &axisIn)
    : TemporalCS(axisIn) {}

// ---------------------------------------------------------------------------

/** \brief Instanciate a TemporalCountCS.
 *
 * @param properties See \ref general_properties.
 * @param axisIn The axis.
 * @return a new TemporalCountCS.
 */
TemporalCountCSNNPtr
TemporalCountCS::create(const util::PropertyMap &properties,
                        const CoordinateSystemAxisNNPtr &axisIn) {
    auto cs(TemporalCountCS::nn_make_shared<TemporalCountCS>(axisIn));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

std::string TemporalCountCS::getWKT2Type(bool use2018Keywords) const {
    return use2018Keywords ? "TemporalCount" : "temporal";
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
TemporalMeasureCS::~TemporalMeasureCS() = default;
//! @endcond

// ---------------------------------------------------------------------------

TemporalMeasureCS::TemporalMeasureCS(const CoordinateSystemAxisNNPtr &axisIn)
    : TemporalCS(axisIn) {}

// ---------------------------------------------------------------------------

/** \brief Instanciate a TemporalMeasureCS.
 *
 * @param properties See \ref general_properties.
 * @param axisIn The axis.
 * @return a new TemporalMeasureCS.
 */
TemporalMeasureCSNNPtr
TemporalMeasureCS::create(const util::PropertyMap &properties,
                          const CoordinateSystemAxisNNPtr &axisIn) {
    auto cs(TemporalMeasureCS::nn_make_shared<TemporalMeasureCS>(axisIn));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

std::string TemporalMeasureCS::getWKT2Type(bool use2018Keywords) const {
    return use2018Keywords ? "TemporalMeasure" : "temporal";
}

} // namespace cs
NS_PROJ_END
