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

#ifndef METADATA_HH_INCLUDED
#define METADATA_HH_INCLUDED

#include <memory>
#include <string>
#include <vector>

#include "io.hpp"
#include "util.hpp"

NS_PROJ_START

namespace common {
class UnitOfMeasure;
using UnitOfMeasurePtr = std::shared_ptr<UnitOfMeasure>;
using UnitOfMeasureNNPtr = util::nn<UnitOfMeasurePtr>;
class IdentifiedObject;
} // namespace common

/** osgeo.proj.metadata namespace
 *
 * \brief Common classes from \ref ISO_19115 standard
 */
namespace metadata {

// ---------------------------------------------------------------------------

/** \brief Standardized resource reference.
 *
 * Local names are names which are directly accessible to and maintained by a
 * NameSpace within which they are local, indicated by the scope.
 *
 * \remark Simplified version of [Citation]
 * (http://www.geoapi.org/3.0/javadoc/org/opengis/metadata/citation/Citation.html)
 * from \ref GeoAPI
 */
class Citation : public util::BaseObject {
  public:
    PROJ_DLL explicit Citation(const std::string &titleIn);
    //! @cond Doxygen_Suppress
    PROJ_DLL Citation();
    PROJ_DLL Citation(const Citation &other);
    PROJ_DLL ~Citation();
    //! @endcond

    PROJ_DLL const util::optional<std::string> &title() const;

  protected:
    PROJ_FRIEND_OPTIONAL(Citation);
    Citation &operator=(const Citation &other);

  private:
    PROJ_OPAQUE_PRIVATE_DATA
};

// ---------------------------------------------------------------------------

class GeographicExtent;
/** Shared pointer of GeographicExtent. */
using GeographicExtentPtr = std::shared_ptr<GeographicExtent>;
/** Non-null shared pointer of GeographicExtent. */
using GeographicExtentNNPtr = util::nn<GeographicExtentPtr>;

/** \brief Base interface for geographic area of the dataset.
 *
 * \remark Simplified version of [GeographicExtent]
 * (http://www.geoapi.org/3.0/javadoc/org/opengis/metadata/extent/GeographicExtent.html)
 * from \ref GeoAPI
 */
class GeographicExtent : public util::BaseObject, public util::IComparable {
  public:
    //! @cond Doxygen_Suppress
    PROJ_DLL ~GeographicExtent() override;
    //! @endcond

    // GeoAPI has a getInclusion() method. We assume that it is included for our
    // use

    PROJ_DLL bool
    isEquivalentTo(const util::IComparable *other,
                   util::IComparable::Criterion criterion =
                       util::IComparable::Criterion::STRICT) const override = 0;

    /** \brief Returns whether this extent contains the other one. */
    PROJ_DLL virtual bool
    contains(const GeographicExtentNNPtr &other) const = 0;

    /** \brief Returns whether this extent intersects the other one. */
    PROJ_DLL virtual bool
    intersects(const GeographicExtentNNPtr &other) const = 0;

    /** \brief Returns the intersection of this extent with another one. */
    PROJ_DLL virtual GeographicExtentPtr
    intersection(const GeographicExtentNNPtr &other) const = 0;

  protected:
    GeographicExtent();

  private:
    PROJ_OPAQUE_PRIVATE_DATA
};

// ---------------------------------------------------------------------------

class GeographicBoundingBox;
/** Shared pointer of GeographicBoundingBox. */
using GeographicBoundingBoxPtr = std::shared_ptr<GeographicBoundingBox>;
/** Non-null shared pointer of GeographicBoundingBox. */
using GeographicBoundingBoxNNPtr = util::nn<GeographicBoundingBoxPtr>;

/** \brief Geographic position of the dataset.
 *
 * This is only an approximate so specifying the co-ordinate reference system is
 * unnecessary.
 *
 * \remark Implements [GeographicBoundingBox]
 * (http://www.geoapi.org/3.0/javadoc/org/opengis/metadata/extent/GeographicBoundingBox.html)
 * from \ref GeoAPI
 */
class GeographicBoundingBox : public GeographicExtent {
  public:
    //! @cond Doxygen_Suppress
    PROJ_DLL ~GeographicBoundingBox() override;
    //! @endcond

    PROJ_DLL double westBoundLongitude() const;
    PROJ_DLL double southBoundLatitude() const;
    PROJ_DLL double eastBoundLongitude() const;
    PROJ_DLL double northBoundLatitude() const;

    PROJ_DLL static GeographicBoundingBoxNNPtr
    create(double west, double south, double east, double north);

    PROJ_DLL bool
    isEquivalentTo(const util::IComparable *other,
                   util::IComparable::Criterion criterion =
                       util::IComparable::Criterion::STRICT) const override;

    PROJ_DLL bool contains(const GeographicExtentNNPtr &other) const override;

    PROJ_DLL bool intersects(const GeographicExtentNNPtr &other) const override;

    PROJ_DLL GeographicExtentPtr
    intersection(const GeographicExtentNNPtr &other) const override;

  protected:
    GeographicBoundingBox(double west, double south, double east, double north);
    INLINED_MAKE_SHARED

  private:
    PROJ_OPAQUE_PRIVATE_DATA
};

// ---------------------------------------------------------------------------

class TemporalExtent;
/** Shared pointer of TemporalExtent. */
using TemporalExtentPtr = std::shared_ptr<TemporalExtent>;
/** Non-null shared pointer of TemporalExtent. */
using TemporalExtentNNPtr = util::nn<TemporalExtentPtr>;

/** \brief Time period covered by the content of the dataset.
 *
 * \remark Simplified version of [TemporalExtent]
 * (http://www.geoapi.org/3.0/javadoc/org/opengis/metadata/extent/TemporalExtent.html)
 * from \ref GeoAPI
 */
class TemporalExtent : public util::BaseObject, public util::IComparable {
  public:
    //! @cond Doxygen_Suppress
    PROJ_DLL ~TemporalExtent() override;
    //! @endcond

    PROJ_DLL const std::string &start() const;
    PROJ_DLL const std::string &stop() const;

    PROJ_DLL static TemporalExtentNNPtr create(const std::string &start,
                                               const std::string &stop);

    PROJ_DLL bool
    isEquivalentTo(const util::IComparable *other,
                   util::IComparable::Criterion criterion =
                       util::IComparable::Criterion::STRICT) const override;

    PROJ_DLL bool contains(const TemporalExtentNNPtr &other) const;

    PROJ_DLL bool intersects(const TemporalExtentNNPtr &other) const;

  protected:
    TemporalExtent(const std::string &start, const std::string &stop);
    INLINED_MAKE_SHARED

  private:
    PROJ_OPAQUE_PRIVATE_DATA
};

// ---------------------------------------------------------------------------

class VerticalExtent;
/** Shared pointer of VerticalExtent. */
using VerticalExtentPtr = std::shared_ptr<VerticalExtent>;
/** Non-null shared pointer of VerticalExtent. */
using VerticalExtentNNPtr = util::nn<VerticalExtentPtr>;

/** \brief Vertical domain of dataset.
 *
 * \remark Simplified version of [VerticalExtent]
 * (http://www.geoapi.org/3.0/javadoc/org/opengis/metadata/extent/VerticalExtent.html)
 * from \ref GeoAPI
 */
class VerticalExtent : public util::BaseObject, public util::IComparable {
  public:
    //! @cond Doxygen_Suppress
    PROJ_DLL ~VerticalExtent() override;
    //! @endcond

    PROJ_DLL double minimumValue() const;
    PROJ_DLL double maximumValue() const;
    PROJ_DLL common::UnitOfMeasureNNPtr &unit() const;

    PROJ_DLL static VerticalExtentNNPtr
    create(double minimumValue, double maximumValue,
           const common::UnitOfMeasureNNPtr &unitIn);

    PROJ_DLL bool
    isEquivalentTo(const util::IComparable *other,
                   util::IComparable::Criterion criterion =
                       util::IComparable::Criterion::STRICT) const override;

    PROJ_DLL bool contains(const VerticalExtentNNPtr &other) const;

    PROJ_DLL bool intersects(const VerticalExtentNNPtr &other) const;

  protected:
    VerticalExtent(double minimumValue, double maximumValue,
                   const common::UnitOfMeasureNNPtr &unitIn);
    INLINED_MAKE_SHARED

  private:
    PROJ_OPAQUE_PRIVATE_DATA
};

// ---------------------------------------------------------------------------

class Extent;
/** Shared pointer of Extent. */
using ExtentPtr = std::shared_ptr<Extent>;
/** Non-null shared pointer of Extent. */
using ExtentNNPtr = util::nn<ExtentPtr>;

/** \brief Information about spatial, vertical, and temporal extent.
 *
 * \remark Simplified version of [Extent]
 * (http://www.geoapi.org/3.0/javadoc/org/opengis/metadata/extent/Extent.html)
 * from \ref GeoAPI
 */
class Extent : public util::BaseObject, public util::IComparable {
  public:
    //! @cond Doxygen_Suppress
    PROJ_DLL Extent(const Extent &other);
    PROJ_DLL ~Extent() override;
    //! @endcond

    PROJ_DLL const util::optional<std::string> &description() const;
    PROJ_DLL const std::vector<GeographicExtentNNPtr> &
    geographicElements() const;
    PROJ_DLL const std::vector<TemporalExtentNNPtr> &temporalElements() const;
    PROJ_DLL const std::vector<VerticalExtentNNPtr> &verticalElements() const;

    PROJ_DLL static ExtentNNPtr
    create(const util::optional<std::string> &descriptionIn,
           const std::vector<GeographicExtentNNPtr> &geographicElementsIn,
           const std::vector<VerticalExtentNNPtr> &verticalElementsIn,
           const std::vector<TemporalExtentNNPtr> &temporalElementsIn);

    PROJ_DLL static ExtentNNPtr
    createFromBBOX(double west, double south, double east, double north,
                   const util::optional<std::string> &descriptionIn =
                       util::optional<std::string>());

    PROJ_DLL bool
    isEquivalentTo(const util::IComparable *other,
                   util::IComparable::Criterion criterion =
                       util::IComparable::Criterion::STRICT) const override;

    PROJ_DLL bool contains(const ExtentNNPtr &other) const;

    PROJ_DLL bool intersects(const ExtentNNPtr &other) const;

    PROJ_DLL ExtentPtr intersection(const ExtentNNPtr &other) const;

    PROJ_DLL static const ExtentNNPtr WORLD;

  protected:
    Extent();
    INLINED_MAKE_SHARED

  private:
    PROJ_OPAQUE_PRIVATE_DATA
    Extent &operator=(const Extent &other) = delete;
};

// ---------------------------------------------------------------------------

class Identifier;
/** Shared pointer of Identifier. */
using IdentifierPtr = std::shared_ptr<Identifier>;
/** Non-null shared pointer of Identifier. */
using IdentifierNNPtr = util::nn<IdentifierPtr>;

/** \brief Value uniquely identifying an object within a namespace.
 *
 * \remark Implements Identifier as described in \ref ISO_19111_2018 but which
 * originates from \ref ISO_19115
 */
class Identifier : public util::BaseObject, public io::IWKTExportable {
  public:
    //! @cond Doxygen_Suppress
    PROJ_DLL Identifier(const Identifier &other);
    PROJ_DLL ~Identifier() override;
    //! @endcond

    PROJ_DLL static IdentifierNNPtr
    create(const std::string &codeIn = std::string(),
           const util::PropertyMap &properties =
               util::PropertyMap()); // throw(InvalidValueTypeException)

    PROJ_DLL static const std::string AUTHORITY_KEY;
    PROJ_DLL static const std::string CODE_KEY;
    PROJ_DLL static const std::string CODESPACE_KEY;
    PROJ_DLL static const std::string VERSION_KEY;
    PROJ_DLL static const std::string DESCRIPTION_KEY;
    PROJ_DLL static const std::string URI_KEY;

    PROJ_DLL static const std::string EPSG;
    PROJ_DLL static const std::string OGC;

    PROJ_DLL const util::optional<Citation> &authority() const;
    PROJ_DLL const std::string &code() const;
    PROJ_DLL const util::optional<std::string> &codeSpace() const;
    PROJ_DLL const util::optional<std::string> &version() const;
    PROJ_DLL const util::optional<std::string> &description() const;
    PROJ_DLL const util::optional<std::string> &uri() const;

    PROJ_DLL static bool isEquivalentName(const std::string &a,
                                          const std::string &b);

    std::string exportToWKT(io::WKTFormatterNNPtr formatter)
        const override; // throw(io::FormattingException)

    PROJ_PRIVATE :
        //! @cond Doxygen_Suppress
        static std::string
        canonicalizeName(const std::string &str);
    //! @endcond

  protected:
    explicit Identifier(const std::string &codeIn = std::string());

    PROJ_FRIEND_OPTIONAL(Identifier);
    INLINED_MAKE_SHARED
    Identifier &operator=(const Identifier &other);

    PROJ_FRIEND(common::IdentifiedObject);

    // Non-standard
    void setProperties(const util::PropertyMap
                           &properties); // throw(InvalidValueTypeException)

  private:
    PROJ_OPAQUE_PRIVATE_DATA
};

// ---------------------------------------------------------------------------

class PositionalAccuracy;
/** Shared pointer of PositionalAccuracy. */
using PositionalAccuracyPtr = std::shared_ptr<PositionalAccuracy>;
/** Non-null shared pointer of PositionalAccuracy. */
using PositionalAccuracyNNPtr = util::nn<PositionalAccuracyPtr>;

/** \brief Accuracy of the position of features.
 *
 * \remark Simplified version of [PositionalAccuracy]
 * (http://www.geoapi.org/3.0/javadoc/org/opengis/metadata/quality/PositionalAccuracy.html)
 * from \ref GeoAPI, which originates from \ref ISO_19115
 */
class PositionalAccuracy : public util::BaseObject {
  public:
    //! @cond Doxygen_Suppress
    PROJ_DLL ~PositionalAccuracy() override;
    //! @endcond

    PROJ_DLL const std::string &value() const;

    PROJ_DLL static PositionalAccuracyNNPtr create(const std::string &valueIn);

  protected:
    explicit PositionalAccuracy(const std::string &valueIn);
    INLINED_MAKE_SHARED

  private:
    PROJ_OPAQUE_PRIVATE_DATA
    PositionalAccuracy(const PositionalAccuracy &other) = delete;
    PositionalAccuracy &operator=(const PositionalAccuracy &other) = delete;
};

} // namespace metadata

NS_PROJ_END

#endif //  METADATA_HH_INCLUDED
