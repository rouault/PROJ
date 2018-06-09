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

#ifndef CS_HH_INCLUDED
#define CS_HH_INCLUDED

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "common.hpp"
#include "io.hpp"
#include "util.hpp"

NS_PROJ_START

/** osgeo.proj.cs namespace

    \brief Coordinate systems and their axis.
*/
namespace cs {

// ---------------------------------------------------------------------------

class AxisDirection : public util::CodeList {
  public:
    PROJ_DLL static const AxisDirection *valueOf(const std::string &nameIn);
    PROJ_DLL static const std::set<std::string> &getKeys();

    PROJ_DLL static const AxisDirection NORTH;
    PROJ_DLL static const AxisDirection NORTH_NORTH_EAST;
    PROJ_DLL static const AxisDirection NORTH_EAST;
    PROJ_DLL static const AxisDirection EAST_NORTH_EAST;
    PROJ_DLL static const AxisDirection EAST;
    PROJ_DLL static const AxisDirection EAST_SOUTH_EAST;
    PROJ_DLL static const AxisDirection SOUTH_EAST;
    PROJ_DLL static const AxisDirection SOUTH_SOUTH_EAST;
    PROJ_DLL static const AxisDirection SOUTH;
    PROJ_DLL static const AxisDirection SOUTH_SOUTH_WEST;
    PROJ_DLL static const AxisDirection SOUTH_WEST;
    PROJ_DLL static const AxisDirection WEST_SOUTH_WEST;
    PROJ_DLL static const AxisDirection WEST;
    PROJ_DLL static const AxisDirection WEST_NORTH_WEST;
    PROJ_DLL static const AxisDirection NORTH_WEST;
    PROJ_DLL static const AxisDirection NORTH_NORTH_WEST;
    PROJ_DLL static const AxisDirection UP;
    PROJ_DLL static const AxisDirection DOWN;
    PROJ_DLL static const AxisDirection GEOCENTRIC_X;
    PROJ_DLL static const AxisDirection GEOCENTRIC_Y;
    PROJ_DLL static const AxisDirection GEOCENTRIC_Z;
    PROJ_DLL static const AxisDirection COLUMN_POSITIVE;
    PROJ_DLL static const AxisDirection COLUMN_NEGATIVE;
    PROJ_DLL static const AxisDirection ROW_POSITIVE;
    PROJ_DLL static const AxisDirection ROW_NEGATIVE;
    PROJ_DLL static const AxisDirection DISPLAY_RIGHT;
    PROJ_DLL static const AxisDirection DISPLAY_LEFT;
    PROJ_DLL static const AxisDirection DISPLAY_UP;
    PROJ_DLL static const AxisDirection DISPLAY_DOWN;
    PROJ_DLL static const AxisDirection FORWARD;
    PROJ_DLL static const AxisDirection AFT;
    PROJ_DLL static const AxisDirection PORT;
    PROJ_DLL static const AxisDirection STARBOARD;
    PROJ_DLL static const AxisDirection CLOCKWISE;
    PROJ_DLL static const AxisDirection COUNTER_CLOCKWISE;
    PROJ_DLL static const AxisDirection TOWARDS;
    PROJ_DLL static const AxisDirection AWAY_FROM;
    PROJ_DLL static const AxisDirection FUTURE;
    PROJ_DLL static const AxisDirection PAST;
    PROJ_DLL static const AxisDirection UNSPECIFIED;

  private:
    explicit AxisDirection(const std::string &nameIn);

    static std::map<std::string, const AxisDirection *> axisDirectionRegistry;
    static std::set<std::string> axisDirectionKeys;
};

// ---------------------------------------------------------------------------

class Meridian;
using MeridianPtr = std::shared_ptr<Meridian>;

/** For WKT2. Used by CoordinateSystemAxis optionaly */
class Meridian : public common::IdentifiedObject, public io::IWKTExportable {
  public:
    PROJ_DLL Meridian(const Meridian &other);
    PROJ_DLL Meridian &operator=(const Meridian &other) = delete;
    PROJ_DLL ~Meridian();

    PROJ_DLL const common::Angle &longitude() const;

    // non-standard
    PROJ_DLL static MeridianPtr create(const util::PropertyMap &properties,
                                       const common::Angle &longitude);

    PROJ_DLL std::string exportToWKT(io::WKTFormatterNNPtr formatter)
        const override; // throw(io::FormattingException)

  protected:
#ifdef DOXYGEN_ENABLED
    Angle angle_;
#endif

    Meridian();
    INLINED_MAKE_SHARED

  private:
    PROJ_OPAQUE_PRIVATE_DATA
};

// ---------------------------------------------------------------------------

class CoordinateSystemAxis;
using CoordinateSystemAxisPtr = std::shared_ptr<CoordinateSystemAxis>;
using CoordinateSystemAxisNNPtr = util::nn<CoordinateSystemAxisPtr>;

class CoordinateSystemAxis : public common::IdentifiedObject,
                             public io::IWKTExportable {
  public:
    PROJ_DLL CoordinateSystemAxis(const CoordinateSystemAxis &other);
    PROJ_DLL CoordinateSystemAxis &
    operator=(const CoordinateSystemAxis &other) = delete;
    PROJ_DLL ~CoordinateSystemAxis();

    PROJ_DLL const std::string &axisAbbrev() const;
    PROJ_DLL const AxisDirection &axisDirection() const;
    PROJ_DLL const common::UnitOfMeasure &axisUnitID() const;
    PROJ_DLL const util::optional<double> &minimumValue() const;
    PROJ_DLL const util::optional<double> &maximumValue() const;
    PROJ_DLL const MeridianPtr &meridian() const;

    // Non-standard
    PROJ_DLL static CoordinateSystemAxisNNPtr
    create(const util::PropertyMap &properties, const std::string &abbreviation,
           const AxisDirection &direction, const common::UnitOfMeasure &unit,
           const MeridianPtr &meridian = nullptr);

    PROJ_DLL std::string exportToWKT(io::WKTFormatterNNPtr formatter)
        const override; // throw(io::FormattingException)
    PROJ_DLL std::string exportToWKT(io::WKTFormatterNNPtr formatter,
                                     int order) const;

  private:
    PROJ_OPAQUE_PRIVATE_DATA

    CoordinateSystemAxis();
    INLINED_MAKE_SHARED
};

// ---------------------------------------------------------------------------

class CoordinateSystem : public common::IdentifiedObject,
                         public io::IWKTExportable {
  public:
    PROJ_DLL virtual ~CoordinateSystem();
    PROJ_DLL CoordinateSystem &
    operator=(const CoordinateSystem &other) = delete;

    PROJ_DLL virtual const std::vector<CoordinateSystemAxisPtr> &
    axisList() const;

    PROJ_DLL std::string exportToWKT(io::WKTFormatterNNPtr formatter)
        const override; // throw(io::FormattingException)

    PROJ_DLL virtual std::string getWKT2Type() const = 0;

  protected:
    CoordinateSystem();
    explicit CoordinateSystem(
        const std::vector<CoordinateSystemAxisPtr> &axisIn);
    CoordinateSystem(const CoordinateSystem &other);

  private:
    PROJ_OPAQUE_PRIVATE_DATA
};

using CoordinateSystemPtr = std::shared_ptr<CoordinateSystem>;
using CoordinateSystemNNPtr = util::nn<CoordinateSystemPtr>;

// ---------------------------------------------------------------------------

class SphericalCS;
using SphericalCSPtr = std::shared_ptr<SphericalCS>;
using SphericalCSNNPtr = util::nn<SphericalCSPtr>;

class SphericalCS : public CoordinateSystem {
  public:
    PROJ_DLL ~SphericalCS() override;

    // non-standard

    PROJ_DLL static SphericalCSNNPtr
    create(const util::PropertyMap &properties,
           const CoordinateSystemAxisPtr &axis0,
           const CoordinateSystemAxisPtr &axis1,
           const CoordinateSystemAxisPtr &axis2);

    PROJ_DLL std::string getWKT2Type() const override { return "spherical"; }

  protected:
    SphericalCS();
    SphericalCS(const SphericalCS &other);
    explicit SphericalCS(const std::vector<CoordinateSystemAxisPtr> &axisIn);
    INLINED_MAKE_SHARED
};

// ---------------------------------------------------------------------------

class EllipsoidalCS;
using EllipsoidalCSPtr = std::shared_ptr<EllipsoidalCS>;
using EllipsoidalCSNNPtr = util::nn<EllipsoidalCSPtr>;

class EllipsoidalCS : public CoordinateSystem {
  public:
    PROJ_DLL ~EllipsoidalCS() override;

    // non-standard
    PROJ_DLL static EllipsoidalCSNNPtr
    create(const util::PropertyMap &properties,
           const CoordinateSystemAxisPtr &axis0,
           const CoordinateSystemAxisPtr &axis1);
    PROJ_DLL static EllipsoidalCSNNPtr
    create(const util::PropertyMap &properties,
           const CoordinateSystemAxisPtr &axis0,
           const CoordinateSystemAxisPtr &axis1,
           const CoordinateSystemAxisPtr &axis2);
    PROJ_DLL static EllipsoidalCSNNPtr createLatitudeLongitudeDegree();
    PROJ_DLL static EllipsoidalCSNNPtr
    createLatitudeLongitudeDegreeEllipsoidalHeightMetre();

    PROJ_DLL std::string getWKT2Type() const override { return "ellipsoidal"; }

  protected:
    EllipsoidalCS();
    EllipsoidalCS(const EllipsoidalCS &other);
    explicit EllipsoidalCS(const std::vector<CoordinateSystemAxisPtr> &axisIn);
    INLINED_MAKE_SHARED
};

// ---------------------------------------------------------------------------

class VerticalCS : public CoordinateSystem {};

using VerticalCSPtr = std::shared_ptr<VerticalCS>;
using VerticalCSNNPtr = util::nn<VerticalCSPtr>;

// ---------------------------------------------------------------------------

class CartesianCS;
using CartesianCSPtr = std::shared_ptr<CartesianCS>;
using CartesianCSNNPtr = util::nn<CartesianCSPtr>;

class CartesianCS : public CoordinateSystem {
  public:
    PROJ_DLL ~CartesianCS() override;

    PROJ_DLL static CartesianCSNNPtr
    create(const util::PropertyMap &properties,
           const CoordinateSystemAxisPtr &axis0,
           const CoordinateSystemAxisPtr &axis1);
    PROJ_DLL static CartesianCSNNPtr
    create(const util::PropertyMap &properties,
           const CoordinateSystemAxisPtr &axis0,
           const CoordinateSystemAxisPtr &axis1,
           const CoordinateSystemAxisPtr &axis2);
    PROJ_DLL static CartesianCSNNPtr createEastingNorthingMetre();
    PROJ_DLL static CartesianCSNNPtr createGeocentric();

    PROJ_DLL std::string getWKT2Type() const override { return "Cartesian"; }

  protected:
    CartesianCS();
    CartesianCS(const CartesianCS &other);
    explicit CartesianCS(const std::vector<CoordinateSystemAxisPtr> &axisIn);
    INLINED_MAKE_SHARED
};

} // namespace cs

NS_PROJ_END

#endif //  CS_HH_INCLUDED
