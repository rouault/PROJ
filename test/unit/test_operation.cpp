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

// to be able to use internal::replaceAll
#define FROM_PROJ_CPP

#include "proj/common.hpp"
#include "proj/coordinateoperation.hpp"
#include "proj/coordinatesystem.hpp"
#include "proj/crs.hpp"
#include "proj/datum.hpp"
#include "proj/io.hpp"
#include "proj/metadata.hpp"
#include "proj/util.hpp"

#include "proj/internal/internal.hpp"

#include "proj_constants.h"

#include <string>
#include <vector>

using namespace osgeo::proj::common;
using namespace osgeo::proj::crs;
using namespace osgeo::proj::cs;
using namespace osgeo::proj::datum;
using namespace osgeo::proj::io;
using namespace osgeo::proj::internal;
using namespace osgeo::proj::metadata;
using namespace osgeo::proj::operation;
using namespace osgeo::proj::util;

namespace {
struct UnrelatedObject : public IComparable {
    UnrelatedObject() = default;

    bool _isEquivalentTo(const IComparable *, Criterion) const override {
        assert(false);
        return false;
    }
};

static nn<std::shared_ptr<UnrelatedObject>> createUnrelatedObject() {
    return nn_make_shared<UnrelatedObject>();
}
} // namespace

// ---------------------------------------------------------------------------

TEST(operation, method) {

    auto method = OperationMethod::create(
        PropertyMap(), std::vector<OperationParameterNNPtr>{});
    EXPECT_TRUE(method->isEquivalentTo(method.get()));
    EXPECT_FALSE(method->isEquivalentTo(createUnrelatedObject().get()));
    auto otherMethod = OperationMethod::create(
        PropertyMap(),
        std::vector<OperationParameterNNPtr>{OperationParameter::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName"))});
    EXPECT_TRUE(otherMethod->isEquivalentTo(otherMethod.get()));
    EXPECT_FALSE(method->isEquivalentTo(otherMethod.get()));
    auto otherMethod2 = OperationMethod::create(
        PropertyMap(),
        std::vector<OperationParameterNNPtr>{OperationParameter::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName2"))});
    EXPECT_FALSE(otherMethod->isEquivalentTo(otherMethod2.get()));
    EXPECT_FALSE(otherMethod->isEquivalentTo(
        otherMethod2.get(), IComparable::Criterion::EQUIVALENT));
}

// ---------------------------------------------------------------------------

TEST(operation, method_parameter_different_order) {

    auto method1 = OperationMethod::create(
        PropertyMap(), std::vector<OperationParameterNNPtr>{
                           OperationParameter::create(PropertyMap().set(
                               IdentifiedObject::NAME_KEY, "paramName")),
                           OperationParameter::create(PropertyMap().set(
                               IdentifiedObject::NAME_KEY, "paramName2"))});

    auto method2 = OperationMethod::create(
        PropertyMap(), std::vector<OperationParameterNNPtr>{
                           OperationParameter::create(PropertyMap().set(
                               IdentifiedObject::NAME_KEY, "paramName2")),
                           OperationParameter::create(PropertyMap().set(
                               IdentifiedObject::NAME_KEY, "paramName"))});

    auto method3 = OperationMethod::create(
        PropertyMap(), std::vector<OperationParameterNNPtr>{
                           OperationParameter::create(PropertyMap().set(
                               IdentifiedObject::NAME_KEY, "paramName3")),
                           OperationParameter::create(PropertyMap().set(
                               IdentifiedObject::NAME_KEY, "paramName"))});

    EXPECT_FALSE(method1->isEquivalentTo(method2.get()));
    EXPECT_TRUE(method1->isEquivalentTo(method2.get(),
                                        IComparable::Criterion::EQUIVALENT));
    EXPECT_FALSE(method1->isEquivalentTo(method3.get(),
                                         IComparable::Criterion::EQUIVALENT));
}

// ---------------------------------------------------------------------------

TEST(operation, ParameterValue) {

    auto valStr1 = ParameterValue::create("str1");
    auto valStr2 = ParameterValue::create("str2");
    EXPECT_TRUE(valStr1->isEquivalentTo(valStr1.get()));
    EXPECT_FALSE(valStr1->isEquivalentTo(createUnrelatedObject().get()));
    EXPECT_FALSE(valStr1->isEquivalentTo(valStr2.get()));

    auto valMeasure1 = ParameterValue::create(Angle(-90.0));
    auto valMeasure1Eps = ParameterValue::create(Angle(-90.0 - 1e-11));
    auto valMeasure2 = ParameterValue::create(Angle(-89.0));
    EXPECT_TRUE(valMeasure1->isEquivalentTo(valMeasure1.get()));
    EXPECT_TRUE(valMeasure1->isEquivalentTo(
        valMeasure1.get(), IComparable::Criterion::EQUIVALENT));
    EXPECT_FALSE(valMeasure1->isEquivalentTo(valMeasure1Eps.get()));
    EXPECT_TRUE(valMeasure1->isEquivalentTo(
        valMeasure1Eps.get(), IComparable::Criterion::EQUIVALENT));

    EXPECT_FALSE(valMeasure1->isEquivalentTo(valStr1.get()));
    EXPECT_FALSE(valMeasure1->isEquivalentTo(valMeasure2.get()));
    EXPECT_FALSE(valMeasure1->isEquivalentTo(
        valMeasure2.get(), IComparable::Criterion::EQUIVALENT));

    auto valInt1 = ParameterValue::create(1);
    auto valInt2 = ParameterValue::create(2);
    EXPECT_TRUE(valInt1->isEquivalentTo(valInt1.get()));
    EXPECT_FALSE(valInt1->isEquivalentTo(valInt2.get()));

    auto valTrue = ParameterValue::create(true);
    auto valFalse = ParameterValue::create(false);
    EXPECT_TRUE(valTrue->isEquivalentTo(valTrue.get()));
    EXPECT_FALSE(valTrue->isEquivalentTo(valFalse.get()));
}

// ---------------------------------------------------------------------------

TEST(operation, OperationParameter) {

    auto op1 = OperationParameter::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName"));
    auto op2 = OperationParameter::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName2"));
    EXPECT_TRUE(op1->isEquivalentTo(op1.get()));
    EXPECT_FALSE(op1->isEquivalentTo(createUnrelatedObject().get()));
    EXPECT_FALSE(op1->isEquivalentTo(op2.get()));
}

// ---------------------------------------------------------------------------

TEST(operation, OperationParameterValue) {

    auto op1 = OperationParameter::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName"));
    auto op2 = OperationParameter::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName2"));
    auto valStr1 = ParameterValue::create("str1");
    auto valStr2 = ParameterValue::create("str2");
    auto opv11 = OperationParameterValue::create(op1, valStr1);
    EXPECT_TRUE(opv11->isEquivalentTo(opv11.get()));
    EXPECT_FALSE(opv11->isEquivalentTo(createUnrelatedObject().get()));
    auto opv12 = OperationParameterValue::create(op1, valStr2);
    EXPECT_FALSE(opv11->isEquivalentTo(opv12.get()));
    auto opv21 = OperationParameterValue::create(op2, valStr1);
    EXPECT_FALSE(opv11->isEquivalentTo(opv12.get()));
}

// ---------------------------------------------------------------------------

TEST(operation, SingleOperation) {

    auto sop1 = Transformation::create(
        PropertyMap(), nn_static_pointer_cast<CRS>(GeographicCRS::EPSG_4326),
        nn_static_pointer_cast<CRS>(GeographicCRS::EPSG_4807),
        static_cast<CRSPtr>(GeographicCRS::EPSG_4979.as_nullable()),
        PropertyMap(),
        std::vector<OperationParameterNNPtr>{OperationParameter::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName"))},
        std::vector<ParameterValueNNPtr>{
            ParameterValue::createFilename("foo.bin")},
        std::vector<PositionalAccuracyNNPtr>{
            PositionalAccuracy::create("0.1")});

    EXPECT_TRUE(sop1->isEquivalentTo(sop1.get()));
    EXPECT_FALSE(sop1->isEquivalentTo(createUnrelatedObject().get()));

    auto sop2 = Transformation::create(
        PropertyMap(), nn_static_pointer_cast<CRS>(GeographicCRS::EPSG_4326),
        nn_static_pointer_cast<CRS>(GeographicCRS::EPSG_4807),
        static_cast<CRSPtr>(GeographicCRS::EPSG_4979.as_nullable()),
        PropertyMap(),
        std::vector<OperationParameterNNPtr>{OperationParameter::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName2"))},
        std::vector<ParameterValueNNPtr>{
            ParameterValue::createFilename("foo.bin")},
        std::vector<PositionalAccuracyNNPtr>{
            PositionalAccuracy::create("0.1")});
    EXPECT_FALSE(sop1->isEquivalentTo(sop2.get()));

    auto sop3 = Transformation::create(
        PropertyMap(), nn_static_pointer_cast<CRS>(GeographicCRS::EPSG_4326),
        nn_static_pointer_cast<CRS>(GeographicCRS::EPSG_4807),
        static_cast<CRSPtr>(GeographicCRS::EPSG_4979.as_nullable()),
        PropertyMap(),
        std::vector<OperationParameterNNPtr>{
            OperationParameter::create(
                PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName")),
            OperationParameter::create(
                PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName2"))},
        std::vector<ParameterValueNNPtr>{
            ParameterValue::createFilename("foo.bin"),
            ParameterValue::createFilename("foo2.bin")},
        std::vector<PositionalAccuracyNNPtr>{
            PositionalAccuracy::create("0.1")});
    EXPECT_FALSE(sop1->isEquivalentTo(sop3.get()));

    auto sop4 = Transformation::create(
        PropertyMap(), nn_static_pointer_cast<CRS>(GeographicCRS::EPSG_4326),
        nn_static_pointer_cast<CRS>(GeographicCRS::EPSG_4807),
        static_cast<CRSPtr>(GeographicCRS::EPSG_4979.as_nullable()),
        PropertyMap(),
        std::vector<OperationParameterNNPtr>{OperationParameter::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName"))},
        std::vector<ParameterValueNNPtr>{
            ParameterValue::createFilename("foo2.bin")},
        std::vector<PositionalAccuracyNNPtr>{
            PositionalAccuracy::create("0.1")});
    EXPECT_FALSE(sop1->isEquivalentTo(sop4.get()));
}

// ---------------------------------------------------------------------------

TEST(operation, SingleOperation_different_order) {

    auto sop1 = Transformation::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "ignored1"),
        GeographicCRS::EPSG_4326, GeographicCRS::EPSG_4807, nullptr,
        PropertyMap(),
        std::vector<OperationParameterNNPtr>{
            OperationParameter::create(
                PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName")),
            OperationParameter::create(
                PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName2"))},
        std::vector<ParameterValueNNPtr>{
            ParameterValue::createFilename("foo.bin"),
            ParameterValue::createFilename("foo2.bin")},
        {});

    auto sop2 = Transformation::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "ignored2"),
        GeographicCRS::EPSG_4326, GeographicCRS::EPSG_4807, nullptr,
        PropertyMap(),
        std::vector<OperationParameterNNPtr>{
            OperationParameter::create(
                PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName2")),
            OperationParameter::create(
                PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName"))},
        std::vector<ParameterValueNNPtr>{
            ParameterValue::createFilename("foo2.bin"),
            ParameterValue::createFilename("foo.bin")},
        {});

    auto sop3 = Transformation::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "ignored3"),
        GeographicCRS::EPSG_4326, GeographicCRS::EPSG_4807, nullptr,
        PropertyMap(),
        std::vector<OperationParameterNNPtr>{
            OperationParameter::create(
                PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName")),
            OperationParameter::create(
                PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName2"))},
        std::vector<ParameterValueNNPtr>{
            ParameterValue::createFilename("foo2.bin"),
            ParameterValue::createFilename("foo.bin")},
        {});

    EXPECT_FALSE(sop1->isEquivalentTo(sop2.get()));
    EXPECT_TRUE(
        sop1->isEquivalentTo(sop2.get(), IComparable::Criterion::EQUIVALENT));
    EXPECT_FALSE(
        sop1->isEquivalentTo(sop3.get(), IComparable::Criterion::EQUIVALENT));
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_to_wkt) {
    PropertyMap propertiesTransformation;
    propertiesTransformation
        .set(Identifier::CODESPACE_KEY, "codeSpaceTransformation")
        .set(Identifier::CODE_KEY, "codeTransformation")
        .set(IdentifiedObject::NAME_KEY, "transformationName")
        .set(IdentifiedObject::REMARKS_KEY, "my remarks");

    auto transf = Transformation::create(
        propertiesTransformation,
        nn_static_pointer_cast<CRS>(GeographicCRS::EPSG_4326),
        nn_static_pointer_cast<CRS>(GeographicCRS::EPSG_4807),
        static_cast<CRSPtr>(GeographicCRS::EPSG_4979.as_nullable()),
        PropertyMap()
            .set(Identifier::CODESPACE_KEY, "codeSpaceOperationMethod")
            .set(Identifier::CODE_KEY, "codeOperationMethod")
            .set(IdentifiedObject::NAME_KEY, "operationMethodName"),
        std::vector<OperationParameterNNPtr>{OperationParameter::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName"))},
        std::vector<ParameterValueNNPtr>{
            ParameterValue::createFilename("foo.bin")},
        std::vector<PositionalAccuracyNNPtr>{
            PositionalAccuracy::create("0.1")});

    std::string src_wkt;
    {
        auto formatter = WKTFormatter::create();
        formatter->setOutputId(false);
        src_wkt = GeographicCRS::EPSG_4326->exportToWKT(formatter.get());
    }

    std::string dst_wkt;
    {
        auto formatter = WKTFormatter::create();
        formatter->setOutputId(false);
        dst_wkt = GeographicCRS::EPSG_4807->exportToWKT(formatter.get());
    }

    std::string interpolation_wkt;
    {
        auto formatter = WKTFormatter::create();
        formatter->setOutputId(false);
        interpolation_wkt =
            GeographicCRS::EPSG_4979->exportToWKT(formatter.get());
    }

    auto expected =
        "COORDINATEOPERATION[\"transformationName\",\n"
        "    SOURCECRS[" +
        src_wkt + "],\n"
                  "    TARGETCRS[" +
        dst_wkt +
        "],\n"
        "    METHOD[\"operationMethodName\",\n"
        "        ID[\"codeSpaceOperationMethod\",\"codeOperationMethod\"]],\n"
        "    PARAMETERFILE[\"paramName\",\"foo.bin\"],\n"
        "    INTERPOLATIONCRS[" +
        interpolation_wkt +
        "],\n"
        "    OPERATIONACCURACY[0.1],\n"
        "    ID[\"codeSpaceTransformation\",\"codeTransformation\"],\n"
        "    REMARK[\"my remarks\"]]";

    EXPECT_EQ(
        replaceAll(replaceAll(transf->exportToWKT(WKTFormatter::create().get()),
                              " ", ""),
                   "\n", ""),
        replaceAll(replaceAll(expected, " ", ""), "\n", ""));

    EXPECT_THROW(
        transf->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        FormattingException);

    EXPECT_TRUE(transf->isEquivalentTo(transf.get()));
    EXPECT_FALSE(transf->isEquivalentTo(createUnrelatedObject().get()));
}

// ---------------------------------------------------------------------------

TEST(operation, concatenated_operation) {

    PropertyMap propertiesTransformation;
    propertiesTransformation
        .set(Identifier::CODESPACE_KEY, "codeSpaceTransformation")
        .set(Identifier::CODE_KEY, "codeTransformation")
        .set(IdentifiedObject::NAME_KEY, "transformationName")
        .set(IdentifiedObject::REMARKS_KEY, "my remarks");

    auto transf_1 = Transformation::create(
        propertiesTransformation,
        nn_static_pointer_cast<CRS>(GeographicCRS::EPSG_4326),
        nn_static_pointer_cast<CRS>(GeographicCRS::EPSG_4807), nullptr,
        PropertyMap().set(IdentifiedObject::NAME_KEY, "operationMethodName"),
        std::vector<OperationParameterNNPtr>{OperationParameter::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName"))},
        std::vector<ParameterValueNNPtr>{
            ParameterValue::createFilename("foo.bin")},
        std::vector<PositionalAccuracyNNPtr>());

    auto transf_2 = Transformation::create(
        propertiesTransformation,
        nn_static_pointer_cast<CRS>(GeographicCRS::EPSG_4807),
        nn_static_pointer_cast<CRS>(GeographicCRS::EPSG_4979), nullptr,
        PropertyMap().set(IdentifiedObject::NAME_KEY, "operationMethodName"),
        std::vector<OperationParameterNNPtr>{OperationParameter::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName"))},
        std::vector<ParameterValueNNPtr>{
            ParameterValue::createFilename("foo.bin")},
        std::vector<PositionalAccuracyNNPtr>());

    auto concat = ConcatenatedOperation::create(
        PropertyMap()
            .set(Identifier::CODESPACE_KEY, "codeSpace")
            .set(Identifier::CODE_KEY, "code")
            .set(IdentifiedObject::NAME_KEY, "name")
            .set(IdentifiedObject::REMARKS_KEY, "my remarks"),
        std::vector<CoordinateOperationNNPtr>{transf_1, transf_2},
        std::vector<PositionalAccuracyNNPtr>{
            PositionalAccuracy::create("0.1")});

    std::string src_wkt;
    {
        auto formatter =
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2018);
        formatter->setOutputId(false);
        src_wkt = GeographicCRS::EPSG_4326->exportToWKT(formatter.get());
    }

    std::string dst_wkt;
    {
        auto formatter =
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2018);
        formatter->setOutputId(false);
        dst_wkt = GeographicCRS::EPSG_4979->exportToWKT(formatter.get());
    }

    std::string step1_wkt;
    {
        auto formatter =
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2018);
        formatter->setOutputId(false);
        step1_wkt = transf_1->exportToWKT(formatter.get());
    }

    std::string step2_wkt;
    {
        auto formatter =
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2018);
        formatter->setOutputId(false);
        step2_wkt = transf_2->exportToWKT(formatter.get());
    }

    auto expected = "CONCATENATEDOPERATION[\"name\",\n"
                    "    SOURCECRS[" +
                    src_wkt + "],\n"
                              "    TARGETCRS[" +
                    dst_wkt + "],\n"
                              "    STEP[" +
                    step1_wkt + "],\n"
                                "    STEP[" +
                    step2_wkt + "],\n"
                                "    ID[\"codeSpace\",\"code\"],\n"
                                "    REMARK[\"my remarks\"]]";

    EXPECT_EQ(replaceAll(replaceAll(concat->exportToWKT(
                                        WKTFormatter::create(
                                            WKTFormatter::Convention::WKT2_2018)
                                            .get()),
                                    " ", ""),
                         "\n", ""),
              replaceAll(replaceAll(expected, " ", ""), "\n", ""));

    EXPECT_THROW(concat->exportToWKT(WKTFormatter::create().get()),
                 FormattingException);

    EXPECT_THROW(ConcatenatedOperation::create(
                     PropertyMap().set(IdentifiedObject::NAME_KEY, "name"),
                     std::vector<CoordinateOperationNNPtr>{transf_1, transf_1},
                     std::vector<PositionalAccuracyNNPtr>()),
                 InvalidOperation);

    auto inv = concat->inverse();
    EXPECT_EQ(inv->nameStr(), "Inverse of name");
    EXPECT_EQ(inv->sourceCRS()->nameStr(), concat->targetCRS()->nameStr());
    EXPECT_EQ(inv->targetCRS()->nameStr(), concat->sourceCRS()->nameStr());
    auto inv_as_concat = nn_dynamic_pointer_cast<ConcatenatedOperation>(inv);
    ASSERT_TRUE(inv_as_concat != nullptr);

    ASSERT_EQ(inv_as_concat->operations().size(), 2);
    EXPECT_EQ(inv_as_concat->operations()[0]->nameStr(),
              "Inverse of transformationName");
    EXPECT_EQ(inv_as_concat->operations()[1]->nameStr(),
              "Inverse of transformationName");

    EXPECT_TRUE(concat->isEquivalentTo(concat.get()));
    EXPECT_FALSE(concat->isEquivalentTo(createUnrelatedObject().get()));
    EXPECT_FALSE(
        ConcatenatedOperation::create(PropertyMap(),
                                      std::vector<CoordinateOperationNNPtr>{
                                          transf_1, transf_1->inverse()},
                                      std::vector<PositionalAccuracyNNPtr>())
            ->isEquivalentTo(ConcatenatedOperation::create(
                                 PropertyMap(),
                                 std::vector<CoordinateOperationNNPtr>{
                                     transf_1->inverse(), transf_1},
                                 std::vector<PositionalAccuracyNNPtr>())
                                 .get()));
    EXPECT_FALSE(
        ConcatenatedOperation::create(PropertyMap(),
                                      std::vector<CoordinateOperationNNPtr>{
                                          transf_1, transf_1->inverse()},
                                      std::vector<PositionalAccuracyNNPtr>())
            ->isEquivalentTo(ConcatenatedOperation::create(
                                 PropertyMap(),
                                 std::vector<CoordinateOperationNNPtr>{
                                     transf_1, transf_1->inverse(), transf_1},
                                 std::vector<PositionalAccuracyNNPtr>())
                                 .get()));
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_createGeocentricTranslations) {

    auto transf = Transformation::createGeocentricTranslations(
        PropertyMap(), GeographicCRS::EPSG_4269, GeographicCRS::EPSG_4326, 1.0,
        2.0, 3.0, std::vector<PositionalAccuracyNNPtr>());

    auto params = transf->getTOWGS84Parameters();
    auto expected = std::vector<double>{1.0, 2.0, 3.0, 0.0, 0.0, 0.0, 0.0};
    EXPECT_EQ(params, expected);

    auto inv_transf = transf->inverse();
    auto inv_transf_as_transf =
        nn_dynamic_pointer_cast<Transformation>(inv_transf);
    ASSERT_TRUE(inv_transf_as_transf != nullptr);

    EXPECT_EQ(transf->sourceCRS()->nameStr(),
              inv_transf_as_transf->targetCRS()->nameStr());
    EXPECT_EQ(transf->targetCRS()->nameStr(),
              inv_transf_as_transf->sourceCRS()->nameStr());
    auto expected_inv =
        std::vector<double>{-1.0, -2.0, -3.0, 0.0, 0.0, 0.0, 0.0};
    EXPECT_EQ(inv_transf_as_transf->getTOWGS84Parameters(), expected_inv);

    EXPECT_EQ(transf->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=cart "
              "+ellps=GRS80 +step +proj=helmert +x=1 +y=2 +z=3 +step +inv "
              "+proj=cart +ellps=WGS84 +step +proj=unitconvert +xy_in=rad "
              "+xy_out=deg +step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

static GeodeticCRSNNPtr createGeocentricDatumWGS84() {
    PropertyMap propertiesCRS;
    propertiesCRS.set(Identifier::CODESPACE_KEY, "EPSG")
        .set(Identifier::CODE_KEY, 4328)
        .set(IdentifiedObject::NAME_KEY, "WGS 84");
    return GeodeticCRS::create(
        propertiesCRS, GeodeticReferenceFrame::EPSG_6326,
        CartesianCS::createGeocentric(UnitOfMeasure::METRE));
}

// ---------------------------------------------------------------------------

static GeodeticCRSNNPtr createGeocentricKM() {
    PropertyMap propertiesCRS;
    propertiesCRS.set(Identifier::CODESPACE_KEY, "EPSG")
        .set(Identifier::CODE_KEY, 4328)
        .set(IdentifiedObject::NAME_KEY, "WGS 84");
    return GeodeticCRS::create(
        propertiesCRS, GeodeticReferenceFrame::EPSG_6326,
        CartesianCS::createGeocentric(
            UnitOfMeasure("kilometre", 1000.0, UnitOfMeasure::Type::LINEAR)));
}

// ---------------------------------------------------------------------------

TEST(operation,
     transformation_createGeocentricTranslations_between_geocentricCRS) {

    auto transf1 = Transformation::createGeocentricTranslations(
        PropertyMap(), createGeocentricDatumWGS84(), createGeocentricKM(), 1.0,
        2.0, 3.0, std::vector<PositionalAccuracyNNPtr>());

    EXPECT_EQ(transf1->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=helmert +x=1 +y=2 +z=3 +step "
              "+proj=unitconvert +xy_in=m +z_in=m +xy_out=km +z_out=km");

    auto transf2 = Transformation::createGeocentricTranslations(
        PropertyMap(), createGeocentricKM(), createGeocentricDatumWGS84(), 1.0,
        2.0, 3.0, std::vector<PositionalAccuracyNNPtr>());

    EXPECT_EQ(transf2->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=unitconvert +xy_in=km +z_in=km "
              "+xy_out=m +z_out=m +step +proj=helmert +x=1 +y=2 +z=3");

    auto transf3 = Transformation::createGeocentricTranslations(
        PropertyMap(), createGeocentricKM(), createGeocentricKM(), 1.0, 2.0,
        3.0, std::vector<PositionalAccuracyNNPtr>());

    EXPECT_EQ(transf3->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=unitconvert +xy_in=km +z_in=km "
              "+xy_out=m +z_out=m +step +proj=helmert +x=1 +y=2 +z=3 +step "
              "+proj=unitconvert +xy_in=m +z_in=m +xy_out=km +z_out=km");
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_createGeocentricTranslations_null) {

    auto transf = Transformation::createGeocentricTranslations(
        PropertyMap(), createGeocentricDatumWGS84(),
        createGeocentricDatumWGS84(), 0.0, 0.0, 0.0,
        std::vector<PositionalAccuracyNNPtr>());

    EXPECT_EQ(transf->inverse()->exportToPROJString(
                  PROJStringFormatter::create().get()),
              "");
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_createGeocentricTranslations_neg_zero) {

    auto transf = Transformation::createGeocentricTranslations(
        PropertyMap(), createGeocentricDatumWGS84(),
        createGeocentricDatumWGS84(), 1.0, -0.0, 0.0,
        std::vector<PositionalAccuracyNNPtr>());

    EXPECT_EQ(transf->inverse()->exportToPROJString(
                  PROJStringFormatter::create().get()),
              "+proj=helmert +x=-1 +y=0 +z=0");
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_createPositionVector) {

    auto transf = Transformation::createPositionVector(
        PropertyMap(), GeographicCRS::EPSG_4269, GeographicCRS::EPSG_4326, 1.0,
        2.0, 3.0, 4.0, 5.0, 6.0, 7.0, std::vector<PositionalAccuracyNNPtr>{
                                          PositionalAccuracy::create("100")});
    ASSERT_EQ(transf->coordinateOperationAccuracies().size(), 1);

    auto expected = std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
    EXPECT_EQ(transf->getTOWGS84Parameters(), expected);

    EXPECT_EQ(transf->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=cart "
              "+ellps=GRS80 +step +proj=helmert +x=1 +y=2 +z=3 +rx=4 +ry=5 "
              "+rz=6 +s=7 +convention=position_vector +step +inv +proj=cart "
              "+ellps=WGS84 +step "
              "+proj=unitconvert +xy_in=rad +xy_out=deg +step +proj=axisswap "
              "+order=2,1");

    auto inv_transf = transf->inverse();
    ASSERT_EQ(inv_transf->coordinateOperationAccuracies().size(), 1);

    EXPECT_EQ(transf->sourceCRS()->nameStr(),
              inv_transf->targetCRS()->nameStr());
    EXPECT_EQ(transf->targetCRS()->nameStr(),
              inv_transf->sourceCRS()->nameStr());

#ifdef USE_APPROXIMATE_HELMERT_INVERSE
    auto inv_transf_as_transf =
        nn_dynamic_pointer_cast<Transformation>(inv_transf);
    ASSERT_TRUE(inv_transf_as_transf != nullptr);
#else
    EXPECT_EQ(
        inv_transf->exportToPROJString(PROJStringFormatter::create().get()),
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=cart "
        "+ellps=WGS84 +step +inv +proj=helmert +x=1 +y=2 +z=3 +rx=4 "
        "+ry=5 +rz=6 +s=7 +convention=position_vector +step +inv "
        "+proj=cart +ellps=GRS80 +step +proj=unitconvert +xy_in=rad "
        "+xy_out=deg +step +proj=axisswap +order=2,1");

    // In WKT, use approximate formula
    auto wkt = inv_transf->exportToWKT(WKTFormatter::create().get());
    EXPECT_TRUE(
        wkt.find("Transformation from WGS 84 to NAD83 (approx. inversion)") !=
        std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("Position Vector transformation (geog2D domain)") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("ID[\"EPSG\",9606]]") != std::string::npos) << wkt;
    EXPECT_TRUE(wkt.find("\"X-axis translation\",-1") != std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Y-axis translation\",-2") != std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Z-axis translation\",-3") != std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"X-axis rotation\",-4") != std::string::npos) << wkt;
    EXPECT_TRUE(wkt.find("\"Y-axis rotation\",-5") != std::string::npos) << wkt;
    EXPECT_TRUE(wkt.find("\"Z-axis rotation\",-6") != std::string::npos) << wkt;
    EXPECT_TRUE(wkt.find("\"Scale difference\",-7") != std::string::npos)
        << wkt;
#endif
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_createCoordinateFrameRotation) {

    auto transf = Transformation::createCoordinateFrameRotation(
        PropertyMap(), GeographicCRS::EPSG_4269, GeographicCRS::EPSG_4326, 1.0,
        2.0, 3.0, -4.0, -5.0, -6.0, 7.0,
        std::vector<PositionalAccuracyNNPtr>());

    auto params = transf->getTOWGS84Parameters();
    auto expected = std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
    EXPECT_EQ(params, expected);

    EXPECT_EQ(transf->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=cart "
              "+ellps=GRS80 +step +proj=helmert +x=1 +y=2 +z=3 +rx=-4 +ry=-5 "
              "+rz=-6 +s=7 +convention=coordinate_frame +step +inv +proj=cart "
              "+ellps=WGS84 +step "
              "+proj=unitconvert +xy_in=rad +xy_out=deg +step +proj=axisswap "
              "+order=2,1");

    auto inv_transf = transf->inverse();
    ASSERT_EQ(inv_transf->coordinateOperationAccuracies().size(), 0);

    EXPECT_EQ(transf->sourceCRS()->nameStr(),
              inv_transf->targetCRS()->nameStr());
    EXPECT_EQ(transf->targetCRS()->nameStr(),
              inv_transf->sourceCRS()->nameStr());

#ifdef USE_APPROXIMATE_HELMERT_INVERSE
    auto inv_transf_as_transf =
        nn_dynamic_pointer_cast<Transformation>(inv_transf);
    ASSERT_TRUE(inv_transf_as_transf != nullptr);
#else
    EXPECT_EQ(
        inv_transf->exportToPROJString(PROJStringFormatter::create().get()),
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=cart "
        "+ellps=WGS84 +step +inv +proj=helmert +x=1 +y=2 +z=3 +rx=-4 "
        "+ry=-5 +rz=-6 +s=7 +convention=coordinate_frame +step +inv "
        "+proj=cart +ellps=GRS80 +step +proj=unitconvert +xy_in=rad "
        "+xy_out=deg +step +proj=axisswap +order=2,1");

    // In WKT, use approximate formula
    auto wkt = inv_transf->exportToWKT(WKTFormatter::create().get());
    EXPECT_TRUE(
        wkt.find("Transformation from WGS 84 to NAD83 (approx. inversion)") !=
        std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("Coordinate Frame rotation (geog2D domain)") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("ID[\"EPSG\",9607]]") != std::string::npos) << wkt;
    EXPECT_TRUE(wkt.find("\"X-axis translation\",-1") != std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Y-axis translation\",-2") != std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Z-axis translation\",-3") != std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"X-axis rotation\",4") != std::string::npos) << wkt;
    EXPECT_TRUE(wkt.find("\"Y-axis rotation\",5") != std::string::npos) << wkt;
    EXPECT_TRUE(wkt.find("\"Z-axis rotation\",6") != std::string::npos) << wkt;
    EXPECT_TRUE(wkt.find("\"Scale difference\",-7") != std::string::npos)
        << wkt;
#endif
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_createTimeDependentPositionVector) {

    auto transf = Transformation::createTimeDependentPositionVector(
        PropertyMap(), GeographicCRS::EPSG_4269, GeographicCRS::EPSG_4326, 1.0,
        2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 2018.5,
        std::vector<PositionalAccuracyNNPtr>());

    auto inv_transf = transf->inverse();

    EXPECT_EQ(transf->sourceCRS()->nameStr(),
              inv_transf->targetCRS()->nameStr());
    EXPECT_EQ(transf->targetCRS()->nameStr(),
              inv_transf->sourceCRS()->nameStr());

    auto projString =
        inv_transf->exportToPROJString(PROJStringFormatter::create().get());
    EXPECT_TRUE(projString.find("+proj=helmert +x=1 +y=2 +z=3 +rx=4 +ry=5 "
                                "+rz=6 +s=7 +dx=0.1 +dy=0.2 +dz=0.3 +drx=0.4 "
                                "+dry=0.5 +drz=0.6 +ds=0.7 +t_epoch=2018.5 "
                                "+convention=position_vector") !=
                std::string::npos)
        << projString;

    // In WKT, use approximate formula
    auto wkt = inv_transf->exportToWKT(WKTFormatter::create().get());
    EXPECT_TRUE(
        wkt.find("Transformation from WGS 84 to NAD83 (approx. inversion)") !=
        std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("Time-dependent Position Vector tfm (geog2D)") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("ID[\"EPSG\",1054]]") != std::string::npos) << wkt;
    EXPECT_TRUE(wkt.find("\"X-axis translation\",-1") != std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Y-axis translation\",-2") != std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Z-axis translation\",-3") != std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"X-axis rotation\",-4") != std::string::npos) << wkt;
    EXPECT_TRUE(wkt.find("\"Y-axis rotation\",-5") != std::string::npos) << wkt;
    EXPECT_TRUE(wkt.find("\"Z-axis rotation\",-6") != std::string::npos) << wkt;
    EXPECT_TRUE(wkt.find("\"Scale difference\",-7") != std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Rate of change of X-axis translation\",-0.1") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Rate of change of Y-axis translation\",-0.2") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Rate of change of Z-axis translation\",-0.3") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Rate of change of X-axis rotation\",-0.4") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Rate of change of Y-axis rotation\",-0.5") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Rate of change of Z-axis rotation\",-0.6") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Rate of change of Scale difference\",-0.7") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Parameter reference epoch\",2018.5") !=
                std::string::npos)
        << wkt;
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_createTimeDependentCoordinateFrameRotation) {

    auto transf = Transformation::createTimeDependentCoordinateFrameRotation(
        PropertyMap(), GeographicCRS::EPSG_4269, GeographicCRS::EPSG_4326, 1.0,
        2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 2018.5,
        std::vector<PositionalAccuracyNNPtr>());

    auto inv_transf = transf->inverse();

    EXPECT_EQ(transf->sourceCRS()->nameStr(),
              inv_transf->targetCRS()->nameStr());
    EXPECT_EQ(transf->targetCRS()->nameStr(),
              inv_transf->sourceCRS()->nameStr());

    auto projString =
        inv_transf->exportToPROJString(PROJStringFormatter::create().get());
    EXPECT_TRUE(projString.find("+proj=helmert +x=1 +y=2 +z=3 +rx=4 +ry=5 "
                                "+rz=6 +s=7 +dx=0.1 +dy=0.2 +dz=0.3 +drx=0.4 "
                                "+dry=0.5 +drz=0.6 +ds=0.7 +t_epoch=2018.5 "
                                "+convention=coordinate_frame") !=
                std::string::npos)
        << projString;

    // In WKT, use approximate formula
    auto wkt = inv_transf->exportToWKT(WKTFormatter::create().get());
    EXPECT_TRUE(
        wkt.find("Transformation from WGS 84 to NAD83 (approx. inversion)") !=
        std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("Time-dependent Coordinate Frame rotation (geog2D)") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("ID[\"EPSG\",1057]]") != std::string::npos) << wkt;
    EXPECT_TRUE(wkt.find("\"X-axis translation\",-1") != std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Y-axis translation\",-2") != std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Z-axis translation\",-3") != std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"X-axis rotation\",-4") != std::string::npos) << wkt;
    EXPECT_TRUE(wkt.find("\"Y-axis rotation\",-5") != std::string::npos) << wkt;
    EXPECT_TRUE(wkt.find("\"Z-axis rotation\",-6") != std::string::npos) << wkt;
    EXPECT_TRUE(wkt.find("\"Scale difference\",-7") != std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Rate of change of X-axis translation\",-0.1") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Rate of change of Y-axis translation\",-0.2") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Rate of change of Z-axis translation\",-0.3") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Rate of change of X-axis rotation\",-0.4") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Rate of change of Y-axis rotation\",-0.5") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Rate of change of Z-axis rotation\",-0.6") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Rate of change of Scale difference\",-0.7") !=
                std::string::npos)
        << wkt;
    EXPECT_TRUE(wkt.find("\"Parameter reference epoch\",2018.5") !=
                std::string::npos)
        << wkt;
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_successive_helmert_noop) {

    auto transf_1 = Transformation::createPositionVector(
        PropertyMap(), GeographicCRS::EPSG_4326, GeographicCRS::EPSG_4269, 1.0,
        2.0, 3.0, 4.0, 5.0, 6.0, 7.0, std::vector<PositionalAccuracyNNPtr>());
    auto transf_2 = Transformation::createPositionVector(
        PropertyMap(), GeographicCRS::EPSG_4269, GeographicCRS::EPSG_4326, -1.0,
        -2.0, -3.0, -4.0, -5.0, -6.0, -7.0,
        std::vector<PositionalAccuracyNNPtr>());

    auto concat = ConcatenatedOperation::create(
        PropertyMap(),
        std::vector<CoordinateOperationNNPtr>{transf_1, transf_2},
        std::vector<PositionalAccuracyNNPtr>{});

    EXPECT_EQ(concat->exportToPROJString(PROJStringFormatter::create().get()),
              "");
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_successive_helmert_non_trivial_1) {

    auto transf_1 = Transformation::createPositionVector(
        PropertyMap(), GeographicCRS::EPSG_4326, GeographicCRS::EPSG_4269, 1.0,
        2.0, 3.0, 4.0, 5.0, 6.0, 7.0, std::vector<PositionalAccuracyNNPtr>());
    auto transf_2 = Transformation::createPositionVector(
        PropertyMap(), GeographicCRS::EPSG_4269, GeographicCRS::EPSG_4326, -1.0,
        -2.0, -3.0, -4.0, -5.0, -6.0, 7.0,
        std::vector<PositionalAccuracyNNPtr>());

    auto concat = ConcatenatedOperation::create(
        PropertyMap(),
        std::vector<CoordinateOperationNNPtr>{transf_1, transf_2},
        std::vector<PositionalAccuracyNNPtr>{});

    EXPECT_NE(concat->exportToPROJString(PROJStringFormatter::create().get()),
              "");
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_successive_helmert_non_trivial_2) {

    auto transf_1 = Transformation::createPositionVector(
        PropertyMap(), GeographicCRS::EPSG_4326, GeographicCRS::EPSG_4269, 1.0,
        2.0, 3.0, 4.0, 5.0, 6.0, 7.0, std::vector<PositionalAccuracyNNPtr>());
    auto transf_2 = Transformation::createCoordinateFrameRotation(
        PropertyMap(), GeographicCRS::EPSG_4269, GeographicCRS::EPSG_4326, -1.0,
        -2.0, -3.0, -4.0, -5.0, -6.0, -7.0,
        std::vector<PositionalAccuracyNNPtr>());

    auto concat = ConcatenatedOperation::create(
        PropertyMap(),
        std::vector<CoordinateOperationNNPtr>{transf_1, transf_2},
        std::vector<PositionalAccuracyNNPtr>{});

    EXPECT_NE(concat->exportToPROJString(PROJStringFormatter::create().get()),
              "");
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_createMolodensky) {

    auto transf = Transformation::createMolodensky(
        PropertyMap(), GeographicCRS::EPSG_4326, GeographicCRS::EPSG_4269, 1.0,
        2.0, 3.0, 4.0, 5.0, std::vector<PositionalAccuracyNNPtr>());

    auto wkt = transf->exportToWKT(WKTFormatter::create().get());
    EXPECT_TRUE(replaceAll(replaceAll(wkt, " ", ""), "\n", "")
                    .find("METHOD[\"Molodensky\",ID[\"EPSG\",9604]]") !=
                std::string::npos)
        << wkt;

    auto inv_transf = transf->inverse();
    auto inv_transf_as_transf =
        nn_dynamic_pointer_cast<Transformation>(inv_transf);
    ASSERT_TRUE(inv_transf_as_transf != nullptr);

    EXPECT_EQ(transf->sourceCRS()->nameStr(),
              inv_transf_as_transf->targetCRS()->nameStr());
    EXPECT_EQ(transf->targetCRS()->nameStr(),
              inv_transf_as_transf->sourceCRS()->nameStr());

    auto projString = inv_transf_as_transf->exportToPROJString(
        PROJStringFormatter::create().get());
    EXPECT_EQ(projString, "+proj=pipeline +step +proj=axisswap +order=2,1 "
                          "+step +proj=unitconvert +xy_in=deg +xy_out=rad "
                          "+step +proj=molodensky +ellps=GRS80 +dx=-1 +dy=-2 "
                          "+dz=-3 +da=-4 +df=-5 +step +proj=unitconvert "
                          "+xy_in=rad +xy_out=deg +step +proj=axisswap "
                          "+order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_createAbridgedMolodensky) {

    auto transf = Transformation::createAbridgedMolodensky(
        PropertyMap(), GeographicCRS::EPSG_4326, GeographicCRS::EPSG_4269, 1.0,
        2.0, 3.0, 4.0, 5.0, std::vector<PositionalAccuracyNNPtr>());

    auto wkt = transf->exportToWKT(WKTFormatter::create().get());
    EXPECT_TRUE(replaceAll(replaceAll(wkt, " ", ""), "\n", "")
                    .find(replaceAll(
                        "METHOD[\"Abridged Molodensky\",ID[\"EPSG\",9605]]",
                        " ", "")) != std::string::npos)
        << wkt;

    auto inv_transf = transf->inverse();
    auto inv_transf_as_transf =
        nn_dynamic_pointer_cast<Transformation>(inv_transf);
    ASSERT_TRUE(inv_transf_as_transf != nullptr);

    EXPECT_EQ(transf->sourceCRS()->nameStr(),
              inv_transf_as_transf->targetCRS()->nameStr());
    EXPECT_EQ(transf->targetCRS()->nameStr(),
              inv_transf_as_transf->sourceCRS()->nameStr());

    auto projString = inv_transf_as_transf->exportToPROJString(
        PROJStringFormatter::create().get());
    EXPECT_EQ(projString, "+proj=pipeline +step +proj=axisswap +order=2,1 "
                          "+step +proj=unitconvert +xy_in=deg +xy_out=rad "
                          "+step +proj=molodensky +ellps=GRS80 +dx=-1 +dy=-2 "
                          "+dz=-3 +da=-4 +df=-5 +abridged +step "
                          "+proj=unitconvert +xy_in=rad +xy_out=deg +step "
                          "+proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_inverse) {

    auto transf = Transformation::create(
        PropertyMap()
            .set(IdentifiedObject::NAME_KEY, "my transformation")
            .set(Identifier::CODESPACE_KEY, "my codeSpace")
            .set(Identifier::CODE_KEY, "my code"),
        GeographicCRS::EPSG_4326, GeographicCRS::EPSG_4269, nullptr,
        PropertyMap()
            .set(IdentifiedObject::NAME_KEY, "my operation")
            .set(Identifier::CODESPACE_KEY, "my codeSpace")
            .set(Identifier::CODE_KEY, "my code"),
        std::vector<OperationParameterNNPtr>{OperationParameter::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "paramName"))},
        std::vector<ParameterValueNNPtr>{
            ParameterValue::createFilename("foo.bin")},
        std::vector<PositionalAccuracyNNPtr>{
            PositionalAccuracy::create("0.1")});
    auto inv = transf->inverse();
    EXPECT_EQ(inv->inverse(), transf);
    EXPECT_EQ(
        inv->exportToWKT(WKTFormatter::create().get()),
        "COORDINATEOPERATION[\"Inverse of my transformation\",\n"
        "    SOURCECRS[\n"
        "        GEODCRS[\"NAD83\",\n"
        "            DATUM[\"North American Datum 1983\",\n"
        "                ELLIPSOID[\"GRS 1980\",6378137,298.257222101,\n"
        "                    LENGTHUNIT[\"metre\",1]]],\n"
        "            PRIMEM[\"Greenwich\",0,\n"
        "                ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
        "            CS[ellipsoidal,2],\n"
        "                AXIS[\"latitude\",north,\n"
        "                    ORDER[1],\n"
        "                    ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
        "                AXIS[\"longitude\",east,\n"
        "                    ORDER[2],\n"
        "                    ANGLEUNIT[\"degree\",0.0174532925199433]]]],\n"
        "    TARGETCRS[\n"
        "        GEODCRS[\"WGS 84\",\n"
        "            DATUM[\"World Geodetic System 1984\",\n"
        "                ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
        "                    LENGTHUNIT[\"metre\",1]]],\n"
        "            PRIMEM[\"Greenwich\",0,\n"
        "                ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
        "            CS[ellipsoidal,2],\n"
        "                AXIS[\"latitude\",north,\n"
        "                    ORDER[1],\n"
        "                    ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
        "                AXIS[\"longitude\",east,\n"
        "                    ORDER[2],\n"
        "                    ANGLEUNIT[\"degree\",0.0174532925199433]]]],\n"
        "    METHOD[\"Inverse of my operation\",\n"
        "        ID[\"INVERSE(my codeSpace)\",\"my code\"]],\n"
        "    PARAMETERFILE[\"paramName\",\"foo.bin\"],\n"
        "    OPERATIONACCURACY[0.1],\n"
        "    ID[\"INVERSE(my codeSpace)\",\"my code\"]]");

    EXPECT_THROW(inv->exportToPROJString(PROJStringFormatter::create().get()),
                 FormattingException);
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_createTOWGS84) {

    EXPECT_THROW(Transformation::createTOWGS84(GeographicCRS::EPSG_4326,
                                               std::vector<double>()),
                 InvalidOperation);

    auto crsIn = CompoundCRS::create(PropertyMap(), std::vector<CRSNNPtr>{});
    EXPECT_THROW(
        Transformation::createTOWGS84(crsIn, std::vector<double>(7, 0)),
        InvalidOperation);
}

// ---------------------------------------------------------------------------

TEST(operation, utm_export) {
    auto conv = Conversion::createUTM(PropertyMap(), 1, false);
    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=utm +zone=1 +south");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"UTM zone 1S\",\n"
              "    METHOD[\"Transverse Mercator\",\n"
              "        ID[\"EPSG\",9807]],\n"
              "    PARAMETER[\"Latitude of natural origin\",0,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",-177,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"Scale factor at natural origin\",0.9996,\n"
              "        SCALEUNIT[\"unity\",1],\n"
              "        ID[\"EPSG\",8805]],\n"
              "    PARAMETER[\"False easting\",500000,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",10000000,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]],\n"
              "    ID[\"EPSG\",17001]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Transverse_Mercator\"],\n"
        "PARAMETER[\"latitude_of_origin\",0],\n"
        "PARAMETER[\"central_meridian\",-177],\n"
        "PARAMETER[\"scale_factor\",0.9996],\n"
        "PARAMETER[\"false_easting\",500000],\n"
        "PARAMETER[\"false_northing\",10000000]");
}

// ---------------------------------------------------------------------------

TEST(operation, tmerc_export) {
    auto conv = Conversion::createTransverseMercator(
        PropertyMap(), Angle(1), Angle(2), Scale(3), Length(4), Length(5));
    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=tmerc +lat_0=1 +lon_0=2 +k_0=3 +x_0=4 +y_0=5");

    {
        auto formatter = PROJStringFormatter::create();
        formatter->setUseETMercForTMerc(true);
        EXPECT_EQ(conv->exportToPROJString(formatter.get()),
                  "+proj=etmerc +lat_0=1 +lon_0=2 +k_0=3 +x_0=4 +y_0=5");
    }

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Transverse Mercator\",\n"
              "    METHOD[\"Transverse Mercator\",\n"
              "        ID[\"EPSG\",9807]],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"Scale factor at natural origin\",3,\n"
              "        SCALEUNIT[\"unity\",1],\n"
              "        ID[\"EPSG\",8805]],\n"
              "    PARAMETER[\"False easting\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Transverse_Mercator\"],\n"
        "PARAMETER[\"latitude_of_origin\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"scale_factor\",3],\n"
        "PARAMETER[\"false_easting\",4],\n"
        "PARAMETER[\"false_northing\",5]");
}

// ---------------------------------------------------------------------------

TEST(operation, gstmerc_export) {
    auto conv = Conversion::createGaussSchreiberTransverseMercator(
        PropertyMap(), Angle(1), Angle(2), Scale(3), Length(4), Length(5));
    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=gstmerc +lat_0=1 +lon_0=2 +k_0=3 +x_0=4 +y_0=5");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Gauss Schreiber Transverse Mercator\",\n"
              "    METHOD[\"Gauss Schreiber Transverse Mercator\"],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"Scale factor at natural origin\",3,\n"
              "        SCALEUNIT[\"unity\",1],\n"
              "        ID[\"EPSG\",8805]],\n"
              "    PARAMETER[\"False easting\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Gauss_Schreiber_Transverse_Mercator\"],\n"
        "PARAMETER[\"latitude_of_origin\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"scale_factor\",3],\n"
        "PARAMETER[\"false_easting\",4],\n"
        "PARAMETER[\"false_northing\",5]");
}

// ---------------------------------------------------------------------------

TEST(operation, tmerc_south_oriented_export) {
    auto conv = Conversion::createTransverseMercatorSouthOriented(
        PropertyMap(), Angle(1), Angle(2), Scale(3), Length(4), Length(5));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=tmerc +axis=wsu +lat_0=1 +lon_0=2 +k_0=3 +x_0=4 +y_0=5");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Transverse Mercator (South Orientated)\",\n"
              "    METHOD[\"Transverse Mercator (South Orientated)\",\n"
              "        ID[\"EPSG\",9808]],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"Scale factor at natural origin\",3,\n"
              "        SCALEUNIT[\"unity\",1],\n"
              "        ID[\"EPSG\",8805]],\n"
              "    PARAMETER[\"False easting\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Transverse_Mercator_South_Orientated\"],\n"
        "PARAMETER[\"latitude_of_origin\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"scale_factor\",3],\n"
        "PARAMETER[\"false_easting\",4],\n"
        "PARAMETER[\"false_northing\",5]");

    auto wkt = "PROJCRS[\"Hartebeesthoek94 / Lo29\","
               "  BASEGEODCRS[\"Hartebeesthoek94\","
               "    DATUM[\"Hartebeesthoek94\","
               "      ELLIPSOID[\"WGS "
               "84\",6378137,298.257223563,LENGTHUNIT[\"metre\",1.0]]]],"
               "  CONVERSION[\"South African Survey Grid zone 29\","
               "    METHOD[\"Transverse Mercator (South "
               "Orientated)\",ID[\"EPSG\",9808]],"
               "    PARAMETER[\"Latitude of natural "
               "origin\",0,ANGLEUNIT[\"degree\",0.01745329252]],"
               "    PARAMETER[\"Longitude of natural "
               "origin\",29,ANGLEUNIT[\"degree\",0.01745329252]],"
               "    PARAMETER[\"Scale factor at natural "
               "origin\",1,SCALEUNIT[\"unity\",1.0]],"
               "    PARAMETER[\"False easting\",0,LENGTHUNIT[\"metre\",1.0]],"
               "    PARAMETER[\"False northing\",0,LENGTHUNIT[\"metre\",1.0]]],"
               "  CS[cartesian,2],"
               "    AXIS[\"westing (Y)\",west,ORDER[1]],"
               "    AXIS[\"southing (X)\",south,ORDER[2]],"
               "    LENGTHUNIT[\"metre\",1.0],"
               "  ID[\"EPSG\",2053]]";
    auto obj = WKTParser().createFromWKT(wkt);
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);
    EXPECT_EQ(crs->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=tmerc "
              "+axis=wsu +lat_0=0 +lon_0=29 +k_0=1 +x_0=0 +y_0=0 +ellps=WGS84");
}

// ---------------------------------------------------------------------------

TEST(operation, tped_export) {
    auto conv = Conversion::createTwoPointEquidistant(
        PropertyMap(), Angle(1), Angle(2), Angle(3), Angle(4), Length(5),
        Length(6));
    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=tpeqd +lat_1=1 +lon_1=2 +lat_2=3 +lon_2=4 +x_0=5 +y_0=6");

    auto formatter = WKTFormatter::create();
    formatter->simulCurNodeHasId();
    EXPECT_EQ(conv->exportToWKT(formatter.get()),
              "CONVERSION[\"Two Point Equidistant\",\n"
              "    METHOD[\"Two Point Equidistant\"],\n"
              "    PARAMETER[\"Latitude of 1st point\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "    PARAMETER[\"Longitude of 1st point\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "    PARAMETER[\"Latitude of 2nd point\",3,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "    PARAMETER[\"Longitude of 2nd point\",4,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "    PARAMETER[\"False easting\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",6,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Two_Point_Equidistant\"],\n"
        "PARAMETER[\"Latitude_Of_1st_Point\",1],\n"
        "PARAMETER[\"Longitude_Of_1st_Point\",2],\n"
        "PARAMETER[\"Latitude_Of_2nd_Point\",3],\n"
        "PARAMETER[\"Longitude_Of_2nd_Point\",4],\n"
        "PARAMETER[\"false_easting\",5],\n"
        "PARAMETER[\"false_northing\",6]");
}

// ---------------------------------------------------------------------------

TEST(operation, tmg_export) {
    auto conv = Conversion::createTunisiaMappingGrid(
        PropertyMap(), Angle(1), Angle(2), Length(3), Length(4));
    EXPECT_THROW(conv->exportToPROJString(PROJStringFormatter::create().get()),
                 FormattingException);

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Tunisia Mapping Grid\",\n"
              "    METHOD[\"Tunisia Mapping Grid\",\n"
              "        ID[\"EPSG\",9816]],\n"
              "    PARAMETER[\"Latitude of false origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8821]],\n"
              "    PARAMETER[\"Longitude of false origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8822]],\n"
              "    PARAMETER[\"Easting at false origin\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8826]],\n"
              "    PARAMETER[\"Northing at false origin\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8827]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Tunisia_Mapping_Grid\"],\n"
        "PARAMETER[\"latitude_of_origin\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"false_easting\",3],\n"
        "PARAMETER[\"false_northing\",4]");
}

// ---------------------------------------------------------------------------

TEST(operation, aea_export) {
    auto conv = Conversion::createAlbersEqualArea(PropertyMap(), Angle(1),
                                                  Angle(2), Angle(3), Angle(4),
                                                  Length(5), Length(6));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=aea +lat_0=1 +lon_0=2 +lat_1=3 +lat_2=4 +x_0=5 +y_0=6");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Albers Equal Area\",\n"
              "    METHOD[\"Albers Equal Area\",\n"
              "        ID[\"EPSG\",9822]],\n"
              "    PARAMETER[\"Latitude of false origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8821]],\n"
              "    PARAMETER[\"Longitude of false origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8822]],\n"
              "    PARAMETER[\"Latitude of 1st standard parallel\",3,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8823]],\n"
              "    PARAMETER[\"Latitude of 2nd standard parallel\",4,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8824]],\n"
              "    PARAMETER[\"Easting at false origin\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8826]],\n"
              "    PARAMETER[\"Northing at false origin\",6,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8827]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Albers_Conic_Equal_Area\"],\n"
        "PARAMETER[\"latitude_of_center\",1],\n"
        "PARAMETER[\"longitude_of_center\",2],\n"
        "PARAMETER[\"standard_parallel_1\",3],\n"
        "PARAMETER[\"standard_parallel_2\",4],\n"
        "PARAMETER[\"false_easting\",5],\n"
        "PARAMETER[\"false_northing\",6]");
}

// ---------------------------------------------------------------------------

TEST(operation, azimuthal_equidistant_export) {
    auto conv = Conversion::createAzimuthalEquidistant(
        PropertyMap(), Angle(1), Angle(2), Length(3), Length(4));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=aeqd +lat_0=1 +lon_0=2 +x_0=3 +y_0=4");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Modified Azimuthal Equidistant\",\n"
              "    METHOD[\"Modified Azimuthal Equidistant\",\n"
              "        ID[\"EPSG\",9832]],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Azimuthal_Equidistant\"],\n"
        "PARAMETER[\"latitude_of_center\",1],\n"
        "PARAMETER[\"longitude_of_center\",2],\n"
        "PARAMETER[\"false_easting\",3],\n"
        "PARAMETER[\"false_northing\",4]");
}

// ---------------------------------------------------------------------------

TEST(operation, guam_projection_export) {
    auto conv = Conversion::createGuamProjection(
        PropertyMap(), Angle(1), Angle(2), Length(3), Length(4));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=aeqd +guam +lat_0=1 +lon_0=2 +x_0=3 +y_0=4");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Guam Projection\",\n"
              "    METHOD[\"Guam Projection\",\n"
              "        ID[\"EPSG\",9831]],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_THROW(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        FormattingException);
}

// ---------------------------------------------------------------------------

TEST(operation, bonne_export) {
    auto conv = Conversion::createBonne(PropertyMap(), Angle(1), Angle(2),
                                        Length(3), Length(4));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=bonne +lat_1=1 +lon_0=2 +x_0=3 +y_0=4");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Bonne\",\n"
              "    METHOD[\"Bonne\",\n"
              "        ID[\"EPSG\",9827]],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Bonne\"],\n"
        "PARAMETER[\"standard_parallel_1\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"false_easting\",3],\n"
        "PARAMETER[\"false_northing\",4]");

    auto obj = WKTParser().createFromWKT(
        "PROJCS[\"unnamed\","
        "GEOGCS[\"unnamed ellipse\","
        "    DATUM[\"unknown\","
        "        SPHEROID[\"unnamed\",6378137,298.257223563]],"
        "    PRIMEM[\"Greenwich\",0],"
        "    UNIT[\"degree\",0.0174532925199433]],"
        "PROJECTION[\"Bonne\"],"
        "PARAMETER[\"standard_parallel_1\",1],"
        "PARAMETER[\"central_meridian\",2],"
        "PARAMETER[\"false_easting\",3],"
        "PARAMETER[\"false_northing\",4],"
        "UNIT[\"metre\",1]]");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);
    EXPECT_EQ(crs->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=unitconvert +xy_in=deg +xy_out=rad "
              "+step +proj=bonne +lat_1=1 +lon_0=2 +x_0=3 +y_0=4 +ellps=WGS84");
}

// ---------------------------------------------------------------------------

TEST(operation, lambert_cylindrical_equal_area_spherical_export) {
    auto conv = Conversion::createLambertCylindricalEqualAreaSpherical(
        PropertyMap(), Angle(1), Angle(2), Length(3), Length(4));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=cea +lat_ts=1 +lon_0=2 +x_0=3 +y_0=4");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Lambert Cylindrical Equal Area (Spherical)\",\n"
              "    METHOD[\"Lambert Cylindrical Equal Area (Spherical)\",\n"
              "        ID[\"EPSG\",9834]],\n"
              "    PARAMETER[\"Latitude of 1st standard parallel\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8823]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Cylindrical_Equal_Area\"],\n"
        "PARAMETER[\"standard_parallel_1\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"false_easting\",3],\n"
        "PARAMETER[\"false_northing\",4]");
}

// ---------------------------------------------------------------------------

TEST(operation, lambert_cylindrical_equal_area_export) {
    auto conv = Conversion::createLambertCylindricalEqualArea(
        PropertyMap(), Angle(1), Angle(2), Length(3), Length(4));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=cea +lat_ts=1 +lon_0=2 +x_0=3 +y_0=4");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Lambert Cylindrical Equal Area\",\n"
              "    METHOD[\"Lambert Cylindrical Equal Area\",\n"
              "        ID[\"EPSG\",9835]],\n"
              "    PARAMETER[\"Latitude of 1st standard parallel\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8823]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Cylindrical_Equal_Area\"],\n"
        "PARAMETER[\"standard_parallel_1\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"false_easting\",3],\n"
        "PARAMETER[\"false_northing\",4]");
}

// ---------------------------------------------------------------------------

TEST(operation, lcc1sp_export) {
    auto conv = Conversion::createLambertConicConformal_1SP(
        PropertyMap(), Angle(1), Angle(2), Scale(3), Length(4), Length(5));
    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=lcc +lat_1=1 +lat_0=1 +lon_0=2 +k_0=3 +x_0=4 +y_0=5");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Lambert Conic Conformal (1SP)\",\n"
              "    METHOD[\"Lambert Conic Conformal (1SP)\",\n"
              "        ID[\"EPSG\",9801]],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"Scale factor at natural origin\",3,\n"
              "        SCALEUNIT[\"unity\",1],\n"
              "        ID[\"EPSG\",8805]],\n"
              "    PARAMETER[\"False easting\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Lambert_Conformal_Conic_1SP\"],\n"
        "PARAMETER[\"latitude_of_origin\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"scale_factor\",3],\n"
        "PARAMETER[\"false_easting\",4],\n"
        "PARAMETER[\"false_northing\",5]");
}

// ---------------------------------------------------------------------------

TEST(operation, lcc2sp_export) {
    auto conv = Conversion::createLambertConicConformal_2SP(
        PropertyMap(), Angle(1), Angle(2), Angle(3), Angle(4), Length(5),
        Length(6));
    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=lcc +lat_0=1 +lon_0=2 +lat_1=3 +lat_2=4 +x_0=5 +y_0=6");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Lambert Conic Conformal (2SP)\",\n"
              "    METHOD[\"Lambert Conic Conformal (2SP)\",\n"
              "        ID[\"EPSG\",9802]],\n"
              "    PARAMETER[\"Latitude of false origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8821]],\n"
              "    PARAMETER[\"Longitude of false origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8822]],\n"
              "    PARAMETER[\"Latitude of 1st standard parallel\",3,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8823]],\n"
              "    PARAMETER[\"Latitude of 2nd standard parallel\",4,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8824]],\n"
              "    PARAMETER[\"Easting at false origin\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8826]],\n"
              "    PARAMETER[\"Northing at false origin\",6,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8827]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Lambert_Conformal_Conic_2SP\"],\n"
        "PARAMETER[\"latitude_of_origin\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"standard_parallel_1\",3],\n"
        "PARAMETER[\"standard_parallel_2\",4],\n"
        "PARAMETER[\"false_easting\",5],\n"
        "PARAMETER[\"false_northing\",6]");
}

// ---------------------------------------------------------------------------

TEST(operation, lcc2sp_isEquivalentTo_parallels_switched) {
    auto conv1 = Conversion::createLambertConicConformal_2SP(
        PropertyMap(), Angle(1), Angle(2), Angle(3), Angle(4), Length(5),
        Length(6));
    auto conv2 = Conversion::createLambertConicConformal_2SP(
        PropertyMap(), Angle(1), Angle(2), Angle(4), Angle(3), Length(5),
        Length(6));

    EXPECT_TRUE(
        conv1->isEquivalentTo(conv2.get(), IComparable::Criterion::EQUIVALENT));

    auto conv3 = Conversion::createLambertConicConformal_2SP(
        PropertyMap(), Angle(1), Angle(2), Angle(3), Angle(3), Length(5),
        Length(6));

    EXPECT_FALSE(
        conv1->isEquivalentTo(conv3.get(), IComparable::Criterion::EQUIVALENT));
}

// ---------------------------------------------------------------------------

TEST(operation, lcc2sp_michigan_export) {
    auto conv = Conversion::createLambertConicConformal_2SP_Michigan(
        PropertyMap(), Angle(1), Angle(2), Angle(3), Angle(4), Length(5),
        Length(6), Scale(7));
    EXPECT_EQ(
        conv->exportToPROJString(PROJStringFormatter::create().get()),
        "+proj=lcc +lat_0=1 +lon_0=2 +lat_1=3 +lat_2=4 +x_0=5 +y_0=6 +k_0=7");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Lambert Conic Conformal (2SP Michigan)\",\n"
              "    METHOD[\"Lambert Conic Conformal (2SP Michigan)\",\n"
              "        ID[\"EPSG\",1051]],\n"
              "    PARAMETER[\"Latitude of false origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8821]],\n"
              "    PARAMETER[\"Longitude of false origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8822]],\n"
              "    PARAMETER[\"Latitude of 1st standard parallel\",3,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8823]],\n"
              "    PARAMETER[\"Latitude of 2nd standard parallel\",4,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8824]],\n"
              "    PARAMETER[\"Easting at false origin\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8826]],\n"
              "    PARAMETER[\"Northing at false origin\",6,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8827]],\n"
              "    PARAMETER[\"Ellipsoid scaling factor\",7,\n"
              "        SCALEUNIT[\"unity\",1],\n"
              "        ID[\"EPSG\",1038]]]");

    EXPECT_THROW(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        FormattingException);
}

// ---------------------------------------------------------------------------

TEST(operation, lcc2sp_belgium_export) {
    auto conv = Conversion::createLambertConicConformal_2SP_Belgium(
        PropertyMap(), Angle(1), Angle(2), Angle(3), Angle(4), Length(5),
        Length(6));
    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=lcc +lat_0=1 +lon_0=2 +lat_1=3 +lat_2=4 +x_0=5 +y_0=6");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Lambert Conic Conformal (2SP Belgium)\",\n"
              "    METHOD[\"Lambert Conic Conformal (2SP Belgium)\",\n"
              "        ID[\"EPSG\",9803]],\n"
              "    PARAMETER[\"Latitude of false origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8821]],\n"
              "    PARAMETER[\"Longitude of false origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8822]],\n"
              "    PARAMETER[\"Latitude of 1st standard parallel\",3,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8823]],\n"
              "    PARAMETER[\"Latitude of 2nd standard parallel\",4,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8824]],\n"
              "    PARAMETER[\"Easting at false origin\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8826]],\n"
              "    PARAMETER[\"Northing at false origin\",6,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8827]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Lambert_Conformal_Conic_2SP_Belgium\"],\n"
        "PARAMETER[\"latitude_of_origin\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"standard_parallel_1\",3],\n"
        "PARAMETER[\"standard_parallel_2\",4],\n"
        "PARAMETER[\"false_easting\",5],\n"
        "PARAMETER[\"false_northing\",6]");
}

// ---------------------------------------------------------------------------

TEST(operation, cassini_soldner_export) {
    auto conv = Conversion::createCassiniSoldner(
        PropertyMap(), Angle(1), Angle(2), Length(4), Length(5));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=cass +lat_0=1 +lon_0=2 +x_0=4 +y_0=5");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Cassini-Soldner\",\n"
              "    METHOD[\"Cassini-Soldner\",\n"
              "        ID[\"EPSG\",9806]],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Cassini_Soldner\"],\n"
        "PARAMETER[\"latitude_of_origin\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"false_easting\",4],\n"
        "PARAMETER[\"false_northing\",5]");
}

// ---------------------------------------------------------------------------

TEST(operation, equidistant_conic_export) {
    auto conv = Conversion::createEquidistantConic(PropertyMap(), Angle(1),
                                                   Angle(2), Angle(3), Angle(4),
                                                   Length(5), Length(6));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=eqdc +lat_0=1 +lon_0=2 +lat_1=3 +lat_2=4 +x_0=5 +y_0=6");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Equidistant Conic\",\n"
              "    METHOD[\"Equidistant Conic\"],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"Latitude of 1st standard parallel\",3,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8823]],\n"
              "    PARAMETER[\"Latitude of 2nd standard parallel\",4,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8824]],\n"
              "    PARAMETER[\"False easting\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",6,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Equidistant_Conic\"],\n"
        "PARAMETER[\"latitude_of_center\",1],\n"
        "PARAMETER[\"longitude_of_center\",2],\n"
        "PARAMETER[\"standard_parallel_1\",3],\n"
        "PARAMETER[\"standard_parallel_2\",4],\n"
        "PARAMETER[\"false_easting\",5],\n"
        "PARAMETER[\"false_northing\",6]");
}

// ---------------------------------------------------------------------------

TEST(operation, eckert_export) {

    std::vector<std::string> numbers{"", "1", "2", "3", "4", "5", "6"};
    std::vector<std::string> latinNumbers{"",   "I", "II", "III",
                                          "IV", "V", "VI"};

    for (int i = 1; i <= 6; i++) {
        auto conv =
            (i == 1)
                ? Conversion::createEckertI(PropertyMap(), Angle(1), Length(2),
                                            Length(3))
                : (i == 2)
                      ? Conversion::createEckertII(PropertyMap(), Angle(1),
                                                   Length(2), Length(3))
                      : (i == 3)
                            ? Conversion::createEckertIII(
                                  PropertyMap(), Angle(1), Length(2), Length(3))
                            : (i == 4) ? Conversion::createEckertIV(
                                             PropertyMap(), Angle(1), Length(2),
                                             Length(3))
                                       : (i == 5) ? Conversion::createEckertV(
                                                        PropertyMap(), Angle(1),
                                                        Length(2), Length(3))
                                                  :

                                                  Conversion::createEckertVI(
                                                      PropertyMap(), Angle(1),
                                                      Length(2), Length(3));

        EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
                  "+proj=eck" + numbers[i] + " +lon_0=1 +x_0=2 +y_0=3");

        EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
                  "CONVERSION[\"Eckert " + latinNumbers[i] +
                      "\",\n"
                      "    METHOD[\"Eckert " +
                      latinNumbers[i] +
                      "\"],\n"
                      "    PARAMETER[\"Longitude of natural origin\",1,\n"
                      "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
                      "        ID[\"EPSG\",8802]],\n"
                      "    PARAMETER[\"False easting\",2,\n"
                      "        LENGTHUNIT[\"metre\",1],\n"
                      "        ID[\"EPSG\",8806]],\n"
                      "    PARAMETER[\"False northing\",3,\n"
                      "        LENGTHUNIT[\"metre\",1],\n"
                      "        ID[\"EPSG\",8807]]]");

        EXPECT_EQ(conv->exportToWKT(
                      WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL)
                          .get()),
                  "PROJECTION[\"Eckert_" + latinNumbers[i] +
                      "\"],\n"
                      "PARAMETER[\"central_meridian\",1],\n"
                      "PARAMETER[\"false_easting\",2],\n"
                      "PARAMETER[\"false_northing\",3]");
    }
}

// ---------------------------------------------------------------------------

TEST(operation, createEquidistantCylindrical) {
    auto conv = Conversion::createEquidistantCylindrical(
        PropertyMap(), Angle(1), Angle(2), Length(3), Length(4));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=eqc +lat_ts=1 +lon_0=2 +x_0=3 +y_0=4");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Equidistant Cylindrical\",\n"
              "    METHOD[\"Equidistant Cylindrical\",\n"
              "        ID[\"EPSG\",1028]],\n"
              "    PARAMETER[\"Latitude of 1st standard parallel\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8823]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Equirectangular\"],\n"
        "PARAMETER[\"standard_parallel_1\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"false_easting\",3],\n"
        "PARAMETER[\"false_northing\",4]");
}

// ---------------------------------------------------------------------------

TEST(operation, createEquidistantCylindricalSpherical) {
    auto conv = Conversion::createEquidistantCylindricalSpherical(
        PropertyMap(), Angle(1), Angle(2), Length(3), Length(4));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=eqc +lat_ts=1 +lon_0=2 +x_0=3 +y_0=4");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Equidistant Cylindrical (Spherical)\",\n"
              "    METHOD[\"Equidistant Cylindrical (Spherical)\",\n"
              "        ID[\"EPSG\",1029]],\n"
              "    PARAMETER[\"Latitude of 1st standard parallel\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8823]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Equirectangular\"],\n"
        "PARAMETER[\"standard_parallel_1\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"false_easting\",3],\n"
        "PARAMETER[\"false_northing\",4]");
}

// ---------------------------------------------------------------------------

TEST(operation, gall_export) {

    auto conv =
        Conversion::createGall(PropertyMap(), Angle(1), Length(2), Length(3));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=gall +lon_0=1 +x_0=2 +y_0=3");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Gall Stereographic\",\n"
              "    METHOD[\"Gall Stereographic\"],\n"
              "    PARAMETER[\"Longitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",2,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Gall_Stereographic\"],\n"
        "PARAMETER[\"central_meridian\",1],\n"
        "PARAMETER[\"false_easting\",2],\n"
        "PARAMETER[\"false_northing\",3]");
}

// ---------------------------------------------------------------------------

TEST(operation, goode_homolosine_export) {

    auto conv = Conversion::createGoodeHomolosine(PropertyMap(), Angle(1),
                                                  Length(2), Length(3));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=goode +lon_0=1 +x_0=2 +y_0=3");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Goode Homolosine\",\n"
              "    METHOD[\"Goode Homolosine\"],\n"
              "    PARAMETER[\"Longitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",2,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Goode_Homolosine\"],\n"
        "PARAMETER[\"central_meridian\",1],\n"
        "PARAMETER[\"false_easting\",2],\n"
        "PARAMETER[\"false_northing\",3]");
}

// ---------------------------------------------------------------------------

TEST(operation, interrupted_goode_homolosine_export) {

    auto conv = Conversion::createInterruptedGoodeHomolosine(
        PropertyMap(), Angle(1), Length(2), Length(3));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=igh +lon_0=1 +x_0=2 +y_0=3");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Interrupted Goode Homolosine\",\n"
              "    METHOD[\"Interrupted Goode Homolosine\"],\n"
              "    PARAMETER[\"Longitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",2,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Interrupted_Goode_Homolosine\"],\n"
        "PARAMETER[\"central_meridian\",1],\n"
        "PARAMETER[\"false_easting\",2],\n"
        "PARAMETER[\"false_northing\",3]");
}

// ---------------------------------------------------------------------------

TEST(operation, geostationary_satellite_sweep_x_export) {

    auto conv = Conversion::createGeostationarySatelliteSweepX(
        PropertyMap(), Angle(1), Length(2), Length(3), Length(4));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=geos +sweep=x +lon_0=1 +h=2 +x_0=3 +y_0=4");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Geostationary Satellite (Sweep X)\",\n"
              "    METHOD[\"Geostationary Satellite (Sweep X)\"],\n"
              "    PARAMETER[\"Longitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"Satellite Height\",2,\n"
              "        LENGTHUNIT[\"metre\",1,\n"
              "            ID[\"EPSG\",9001]]],\n"
              "    PARAMETER[\"False easting\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_THROW(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        FormattingException);
}

// ---------------------------------------------------------------------------

TEST(operation, geostationary_satellite_sweep_y_export) {

    auto conv = Conversion::createGeostationarySatelliteSweepY(
        PropertyMap(), Angle(1), Length(2), Length(3), Length(4));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=geos +lon_0=1 +h=2 +x_0=3 +y_0=4");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Geostationary Satellite (Sweep Y)\",\n"
              "    METHOD[\"Geostationary Satellite (Sweep Y)\"],\n"
              "    PARAMETER[\"Longitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"Satellite Height\",2,\n"
              "        LENGTHUNIT[\"metre\",1,\n"
              "            ID[\"EPSG\",9001]]],\n"
              "    PARAMETER[\"False easting\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Geostationary_Satellite\"],\n"
        "PARAMETER[\"central_meridian\",1],\n"
        "PARAMETER[\"satellite_height\",2],\n"
        "PARAMETER[\"false_easting\",3],\n"
        "PARAMETER[\"false_northing\",4]");
}

// ---------------------------------------------------------------------------

TEST(operation, gnomonic_export) {
    auto conv = Conversion::createGnomonic(PropertyMap(), Angle(1), Angle(2),
                                           Length(4), Length(5));
    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=gnom +lat_0=1 +lon_0=2 +x_0=4 +y_0=5");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Gnomonic\",\n"
              "    METHOD[\"Gnomonic\"],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Gnomonic\"],\n"
        "PARAMETER[\"latitude_of_origin\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"false_easting\",4],\n"
        "PARAMETER[\"false_northing\",5]");
}

// ---------------------------------------------------------------------------

TEST(operation, hotine_oblique_mercator_variant_A_export) {
    auto conv = Conversion::createHotineObliqueMercatorVariantA(
        PropertyMap(), Angle(1), Angle(2), Angle(3), Angle(4), Scale(5),
        Length(6), Length(7));
    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=omerc +no_uoff +lat_0=1 +lonc=2 +alpha=3 +gamma=4 +k=5 "
              "+x_0=6 +y_0=7");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Hotine Oblique Mercator (variant A)\",\n"
              "    METHOD[\"Hotine Oblique Mercator (variant A)\",\n"
              "        ID[\"EPSG\",9812]],\n"
              "    PARAMETER[\"Latitude of projection centre\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8811]],\n"
              "    PARAMETER[\"Longitude of projection centre\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8812]],\n"
              "    PARAMETER[\"Azimuth of initial line\",3,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8813]],\n"
              "    PARAMETER[\"Angle from Rectified to Skew Grid\",4,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8814]],\n"
              "    PARAMETER[\"Scale factor on initial line\",5,\n"
              "        SCALEUNIT[\"unity\",1],\n"
              "        ID[\"EPSG\",8815]],\n"
              "    PARAMETER[\"False easting\",6,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",7,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Hotine_Oblique_Mercator\"],\n"
        "PARAMETER[\"latitude_of_center\",1],\n"
        "PARAMETER[\"longitude_of_center\",2],\n"
        "PARAMETER[\"azimuth\",3],\n"
        "PARAMETER[\"rectified_grid_angle\",4],\n"
        "PARAMETER[\"scale_factor\",5],\n"
        "PARAMETER[\"false_easting\",6],\n"
        "PARAMETER[\"false_northing\",7]");
}

// ---------------------------------------------------------------------------

TEST(operation, hotine_oblique_mercator_variant_A_export_swiss_mercator) {
    auto conv = Conversion::createHotineObliqueMercatorVariantA(
        PropertyMap(), Angle(1), Angle(2), Angle(90), Angle(90), Scale(5),
        Length(6), Length(7));
    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=somerc +lat_0=1 +lon_0=2 +k_0=5 "
              "+x_0=6 +y_0=7");
}

// ---------------------------------------------------------------------------

TEST(operation, hotine_oblique_mercator_variant_B_export) {
    auto conv = Conversion::createHotineObliqueMercatorVariantB(
        PropertyMap(), Angle(1), Angle(2), Angle(3), Angle(4), Scale(5),
        Length(6), Length(7));
    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=omerc +lat_0=1 +lonc=2 +alpha=3 +gamma=4 +k=5 "
              "+x_0=6 +y_0=7");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Hotine Oblique Mercator (variant B)\",\n"
              "    METHOD[\"Hotine Oblique Mercator (variant B)\",\n"
              "        ID[\"EPSG\",9815]],\n"
              "    PARAMETER[\"Latitude of projection centre\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8811]],\n"
              "    PARAMETER[\"Longitude of projection centre\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8812]],\n"
              "    PARAMETER[\"Azimuth of initial line\",3,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8813]],\n"
              "    PARAMETER[\"Angle from Rectified to Skew Grid\",4,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8814]],\n"
              "    PARAMETER[\"Scale factor on initial line\",5,\n"
              "        SCALEUNIT[\"unity\",1],\n"
              "        ID[\"EPSG\",8815]],\n"
              "    PARAMETER[\"Easting at projection centre\",6,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8816]],\n"
              "    PARAMETER[\"Northing at projection centre\",7,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8817]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Hotine_Oblique_Mercator_Azimuth_Center\"],\n"
        "PARAMETER[\"latitude_of_center\",1],\n"
        "PARAMETER[\"longitude_of_center\",2],\n"
        "PARAMETER[\"azimuth\",3],\n"
        "PARAMETER[\"rectified_grid_angle\",4],\n"
        "PARAMETER[\"scale_factor\",5],\n"
        "PARAMETER[\"false_easting\",6],\n"
        "PARAMETER[\"false_northing\",7]");
}

// ---------------------------------------------------------------------------

TEST(operation, hotine_oblique_mercator_variant_B_export_swiss_mercator) {
    auto conv = Conversion::createHotineObliqueMercatorVariantB(
        PropertyMap(), Angle(1), Angle(2), Angle(90), Angle(90), Scale(5),
        Length(6), Length(7));
    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=somerc +lat_0=1 +lon_0=2 +k_0=5 "
              "+x_0=6 +y_0=7");
}

// ---------------------------------------------------------------------------

TEST(operation, hotine_oblique_mercator_two_point_natural_origin_export) {
    auto conv = Conversion::createHotineObliqueMercatorTwoPointNaturalOrigin(
        PropertyMap(), Angle(1), Angle(2), Angle(3), Angle(4), Angle(5),
        Scale(6), Length(7), Length(8));
    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=omerc +lat_0=1 +lat_1=2 +lon_1=3 +lat_2=4 +lon_2=5 +k=6 "
              "+x_0=7 +y_0=8");

    auto formatter = WKTFormatter::create();
    formatter->simulCurNodeHasId();
    EXPECT_EQ(
        conv->exportToWKT(formatter.get()),
        "CONVERSION[\"Hotine Oblique Mercator Two Point Natural Origin\",\n"
        "    METHOD[\"Hotine Oblique Mercator Two Point Natural Origin\"],\n"
        "    PARAMETER[\"Latitude of projection centre\",1,\n"
        "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
        "        ID[\"EPSG\",8811]],\n"
        "    PARAMETER[\"Latitude of 1st point\",2,\n"
        "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
        "    PARAMETER[\"Longitude of 1st point\",3,\n"
        "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
        "    PARAMETER[\"Latitude of 2nd point\",4,\n"
        "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
        "    PARAMETER[\"Longitude of 2nd point\",5,\n"
        "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
        "    PARAMETER[\"Scale factor on initial line\",6,\n"
        "        SCALEUNIT[\"unity\",1],\n"
        "        ID[\"EPSG\",8815]],\n"
        "    PARAMETER[\"Easting at projection centre\",7,\n"
        "        LENGTHUNIT[\"metre\",1],\n"
        "        ID[\"EPSG\",8816]],\n"
        "    PARAMETER[\"Northing at projection centre\",8,\n"
        "        LENGTHUNIT[\"metre\",1],\n"
        "        ID[\"EPSG\",8817]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Hotine_Oblique_Mercator_Two_Point_Natural_Origin\"],\n"
        "PARAMETER[\"latitude_of_center\",1],\n"
        "PARAMETER[\"latitude_of_point_1\",2],\n"
        "PARAMETER[\"longitude_of_point_1\",3],\n"
        "PARAMETER[\"latitude_of_point_2\",4],\n"
        "PARAMETER[\"longitude_of_point_2\",5],\n"
        "PARAMETER[\"scale_factor\",6],\n"
        "PARAMETER[\"false_easting\",7],\n"
        "PARAMETER[\"false_northing\",8]");
}

// ---------------------------------------------------------------------------

TEST(operation, imw_polyconic_export) {
    auto conv = Conversion::createInternationalMapWorldPolyconic(
        PropertyMap(), Angle(1), Angle(3), Angle(4), Length(5), Length(6));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=imw_p +lon_0=1 +lat_1=3 +lat_2=4 +x_0=5 +y_0=6");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"International Map of the World Polyconic\",\n"
              "    METHOD[\"International Map of the World Polyconic\"],\n"
              "    PARAMETER[\"Longitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"Latitude of 1st point\",3,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433,\n"
              "            ID[\"EPSG\",9122]]],\n"
              "    PARAMETER[\"Latitude of 2nd point\",4,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433,\n"
              "            ID[\"EPSG\",9122]]],\n"
              "    PARAMETER[\"False easting\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",6,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"International_Map_of_the_World_Polyconic\"],\n"
        "PARAMETER[\"central_meridian\",1],\n"
        "PARAMETER[\"Latitude_Of_1st_Point\",3],\n"
        "PARAMETER[\"Latitude_Of_2nd_Point\",4],\n"
        "PARAMETER[\"false_easting\",5],\n"
        "PARAMETER[\"false_northing\",6]");
}

// ---------------------------------------------------------------------------

TEST(operation, krovak_north_oriented_export) {
    auto conv = Conversion::createKrovakNorthOriented(
        PropertyMap(), Angle(49.5), Angle(42.5), Angle(30.28813972222222),
        Angle(78.5), Scale(0.9999), Length(5), Length(6));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=krovak +lat_0=49.5 +lon_0=42.5 +alpha=30.2881397222222 "
              "+k=0.9999 +x_0=5 +y_0=6");

    EXPECT_EQ(
        conv->exportToWKT(WKTFormatter::create().get()),
        "CONVERSION[\"Krovak (North Orientated)\",\n"
        "    METHOD[\"Krovak (North Orientated)\",\n"
        "        ID[\"EPSG\",1041]],\n"
        "    PARAMETER[\"Latitude of projection centre\",49.5,\n"
        "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
        "        ID[\"EPSG\",8811]],\n"
        "    PARAMETER[\"Longitude of origin\",42.5,\n"
        "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
        "        ID[\"EPSG\",8833]],\n"
        "    PARAMETER[\"Co-latitude of cone axis\",30.2881397222222,\n"
        "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
        "        ID[\"EPSG\",1036]],\n"
        "    PARAMETER[\"Latitude of pseudo standard parallel\",78.5,\n"
        "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
        "        ID[\"EPSG\",8818]],\n"
        "    PARAMETER[\"Scale factor on pseudo standard parallel\",0.9999,\n"
        "        SCALEUNIT[\"unity\",1],\n"
        "        ID[\"EPSG\",8819]],\n"
        "    PARAMETER[\"False easting\",5,\n"
        "        LENGTHUNIT[\"metre\",1],\n"
        "        ID[\"EPSG\",8806]],\n"
        "    PARAMETER[\"False northing\",6,\n"
        "        LENGTHUNIT[\"metre\",1],\n"
        "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Krovak\"],\n"
        "PARAMETER[\"latitude_of_center\",49.5],\n"
        "PARAMETER[\"longitude_of_center\",42.5],\n"
        "PARAMETER[\"azimuth\",30.2881397222222],\n"
        "PARAMETER[\"pseudo_standard_parallel_1\",78.5],\n"
        "PARAMETER[\"scale_factor\",0.9999],\n"
        "PARAMETER[\"false_easting\",5],\n"
        "PARAMETER[\"false_northing\",6]");
}

// ---------------------------------------------------------------------------

TEST(operation, krovak_export) {
    auto conv = Conversion::createKrovak(
        PropertyMap(), Angle(49.5), Angle(42.5), Angle(30.28813972222222),
        Angle(78.5), Scale(0.9999), Length(5), Length(6));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=krovak +axis=swu +lat_0=49.5 +lon_0=42.5 "
              "+alpha=30.2881397222222 +k=0.9999 +x_0=5 "
              "+y_0=6");

    EXPECT_EQ(
        conv->exportToWKT(WKTFormatter::create().get()),
        "CONVERSION[\"Krovak\",\n"
        "    METHOD[\"Krovak\",\n"
        "        ID[\"EPSG\",9819]],\n"
        "    PARAMETER[\"Latitude of projection centre\",49.5,\n"
        "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
        "        ID[\"EPSG\",8811]],\n"
        "    PARAMETER[\"Longitude of origin\",42.5,\n"
        "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
        "        ID[\"EPSG\",8833]],\n"
        "    PARAMETER[\"Co-latitude of cone axis\",30.2881397222222,\n"
        "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
        "        ID[\"EPSG\",1036]],\n"
        "    PARAMETER[\"Latitude of pseudo standard parallel\",78.5,\n"
        "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
        "        ID[\"EPSG\",8818]],\n"
        "    PARAMETER[\"Scale factor on pseudo standard parallel\",0.9999,\n"
        "        SCALEUNIT[\"unity\",1],\n"
        "        ID[\"EPSG\",8819]],\n"
        "    PARAMETER[\"False easting\",5,\n"
        "        LENGTHUNIT[\"metre\",1],\n"
        "        ID[\"EPSG\",8806]],\n"
        "    PARAMETER[\"False northing\",6,\n"
        "        LENGTHUNIT[\"metre\",1],\n"
        "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Krovak\"],\n"
        "PARAMETER[\"latitude_of_center\",49.5],\n"
        "PARAMETER[\"longitude_of_center\",42.5],\n"
        "PARAMETER[\"azimuth\",30.2881397222222],\n"
        "PARAMETER[\"pseudo_standard_parallel_1\",78.5],\n"
        "PARAMETER[\"scale_factor\",0.9999],\n"
        "PARAMETER[\"false_easting\",5],\n"
        "PARAMETER[\"false_northing\",6]");
}

// ---------------------------------------------------------------------------

TEST(operation, lambert_azimuthal_equal_area_export) {
    auto conv = Conversion::createLambertAzimuthalEqualArea(
        PropertyMap(), Angle(1), Angle(2), Length(3), Length(4));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=laea +lat_0=1 +lon_0=2 +x_0=3 +y_0=4");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Lambert Azimuthal Equal Area\",\n"
              "    METHOD[\"Lambert Azimuthal Equal Area\",\n"
              "        ID[\"EPSG\",9820]],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Lambert_Azimuthal_Equal_Area\"],\n"
        "PARAMETER[\"latitude_of_center\",1],\n"
        "PARAMETER[\"longitude_of_center\",2],\n"
        "PARAMETER[\"false_easting\",3],\n"
        "PARAMETER[\"false_northing\",4]");
}

// ---------------------------------------------------------------------------

TEST(operation, miller_cylindrical_export) {
    auto conv = Conversion::createMillerCylindrical(PropertyMap(), Angle(2),
                                                    Length(3), Length(4));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=mill +R_A +lon_0=2 +x_0=3 +y_0=4");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Miller Cylindrical\",\n"
              "    METHOD[\"Miller Cylindrical\"],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Miller_Cylindrical\"],\n"
        "PARAMETER[\"longitude_of_center\",2],\n"
        "PARAMETER[\"false_easting\",3],\n"
        "PARAMETER[\"false_northing\",4]");
}

// ---------------------------------------------------------------------------

TEST(operation, mercator_variant_A_export) {
    auto conv = Conversion::createMercatorVariantA(
        PropertyMap(), Angle(0), Angle(1), Scale(2), Length(3), Length(4));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=merc +lon_0=1 +k=2 +x_0=3 +y_0=4");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Mercator (variant A)\",\n"
              "    METHOD[\"Mercator (variant A)\",\n"
              "        ID[\"EPSG\",9804]],\n"
              "    PARAMETER[\"Latitude of natural origin\",0,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"Scale factor at natural origin\",2,\n"
              "        SCALEUNIT[\"unity\",1],\n"
              "        ID[\"EPSG\",8805]],\n"
              "    PARAMETER[\"False easting\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Mercator_1SP\"],\n"
        "PARAMETER[\"central_meridian\",1],\n"
        "PARAMETER[\"scale_factor\",2],\n"
        "PARAMETER[\"false_easting\",3],\n"
        "PARAMETER[\"false_northing\",4]");
}

// ---------------------------------------------------------------------------

TEST(operation, mercator_variant_A_export_latitude_origin_non_zero) {
    auto conv = Conversion::createMercatorVariantA(
        PropertyMap(), Angle(10), Angle(1), Scale(2), Length(3), Length(4));

    EXPECT_THROW(conv->exportToPROJString(PROJStringFormatter::create().get()),
                 FormattingException);
}

// ---------------------------------------------------------------------------

TEST(operation, wkt1_import_mercator_variant_A) {
    auto wkt = "PROJCS[\"test\",\n"
               "    GEOGCS[\"WGS 84\",\n"
               "        DATUM[\"WGS 1984\",\n"
               "            SPHEROID[\"WGS 84\",6378137,298.257223563]],\n"
               "        PRIMEM[\"Greenwich\",0],\n"
               "        UNIT[\"degree\",0.0174532925199433]],\n"
               "    PROJECTION[\"Mercator_1SP\"],\n"
               "    PARAMETER[\"central_meridian\",1],\n"
               "    PARAMETER[\"scale_factor\",2],\n"
               "    PARAMETER[\"false_easting\",3],\n"
               "    PARAMETER[\"false_northing\",4],\n"
               "    UNIT[\"metre\",1]]";
    auto obj = WKTParser().createFromWKT(wkt);
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto conversion = crs->derivingConversion();
    auto convRef = Conversion::createMercatorVariantA(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "unnamed"), Angle(0),
        Angle(1), Scale(2), Length(3), Length(4));

    EXPECT_EQ(conversion->exportToWKT(WKTFormatter::create().get()),
              convRef->exportToWKT(WKTFormatter::create().get()));
}

// ---------------------------------------------------------------------------

TEST(operation, wkt1_import_mercator_variant_A_that_is_variant_B) {
    // Adresses https://trac.osgeo.org/gdal/ticket/3026
    auto wkt = "PROJCS[\"test\",\n"
               "    GEOGCS[\"WGS 84\",\n"
               "        DATUM[\"WGS 1984\",\n"
               "            SPHEROID[\"WGS 84\",6378137,298.257223563]],\n"
               "        PRIMEM[\"Greenwich\",0],\n"
               "        UNIT[\"degree\",0.0174532925199433]],\n"
               "    PROJECTION[\"Mercator_1SP\"],\n"
               "    PARAMETER[\"latitude_of_origin\",-1],\n"
               "    PARAMETER[\"central_meridian\",2],\n"
               "    PARAMETER[\"scale_factor\",1],\n"
               "    PARAMETER[\"false_easting\",3],\n"
               "    PARAMETER[\"false_northing\",4],\n"
               "    UNIT[\"metre\",1]]";
    auto obj = WKTParser().createFromWKT(wkt);
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto conversion = crs->derivingConversion();
    auto convRef = Conversion::createMercatorVariantB(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "unnamed"), Angle(-1),
        Angle(2), Length(3), Length(4));

    EXPECT_TRUE(conversion->isEquivalentTo(convRef.get(),
                                           IComparable::Criterion::EQUIVALENT));
}

// ---------------------------------------------------------------------------

TEST(operation, mercator_variant_B_export) {
    auto conv = Conversion::createMercatorVariantB(
        PropertyMap(), Angle(1), Angle(2), Length(3), Length(4));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=merc +lat_ts=1 +lon_0=2 +x_0=3 +y_0=4");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Mercator (variant B)\",\n"
              "    METHOD[\"Mercator (variant B)\",\n"
              "        ID[\"EPSG\",9805]],\n"
              "    PARAMETER[\"Latitude of 1st standard parallel\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8823]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Mercator_2SP\"],\n"
        "PARAMETER[\"standard_parallel_1\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"false_easting\",3],\n"
        "PARAMETER[\"false_northing\",4]");
}

// ---------------------------------------------------------------------------

TEST(operation, webmerc_export) {
    auto conv = Conversion::createPopularVisualisationPseudoMercator(
        PropertyMap(), Angle(0), Angle(2), Length(3), Length(4));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=webmerc +lat_0=0 +lon_0=2 +x_0=3 +y_0=4");

    EXPECT_THROW(
        conv->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        FormattingException);

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Popular Visualisation Pseudo Mercator\",\n"
              "    METHOD[\"Popular Visualisation Pseudo Mercator\",\n"
              "        ID[\"EPSG\",1024]],\n"
              "    PARAMETER[\"Latitude of natural origin\",0,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    auto projCRS = ProjectedCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Pseudo-Mercator"),
        GeographicCRS::EPSG_4326, conv,
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));

    EXPECT_EQ(
        projCRS->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJCS[\"Pseudo-Mercator\",\n"
        "    GEOGCS[\"WGS 84\",\n"
        "        DATUM[\"WGS_1984\",\n"
        "            SPHEROID[\"WGS 84\",6378137,298.257223563,\n"
        "                AUTHORITY[\"EPSG\",\"7030\"]],\n"
        "            AUTHORITY[\"EPSG\",\"6326\"]],\n"
        "        PRIMEM[\"Greenwich\",0,\n"
        "            AUTHORITY[\"EPSG\",\"8901\"]],\n"
        "        UNIT[\"degree\",0.0174532925199433,\n"
        "            AUTHORITY[\"EPSG\",\"9122\"]],\n"
        "        AUTHORITY[\"EPSG\",\"4326\"]],\n"
        "    PROJECTION[\"Mercator_1SP\"],\n"
        "    PARAMETER[\"central_meridian\",2],\n"
        "    PARAMETER[\"scale_factor\",1],\n"
        "    PARAMETER[\"false_easting\",3],\n"
        "    PARAMETER[\"false_northing\",4],\n"
        "    UNIT[\"metre\",1,\n"
        "        AUTHORITY[\"EPSG\",\"9001\"]],\n"
        "    AXIS[\"Easting\",EAST],\n"
        "    AXIS[\"Northing\",NORTH],\n"
        "    EXTENSION[\"PROJ4\",\"+proj=merc "
        "+a=6378137 +b=6378137 +lat_ts=0 +lon_0=2 "
        "+x_0=3 +y_0=4 +k=1 +units=m "
        "+nadgrids=@null +wktext +no_defs\"]]");

    EXPECT_EQ(
        projCRS->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_5)
                .get()),
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=webmerc "
        "+lat_0=0 +lon_0=2 +x_0=3 +y_0=4 +ellps=WGS84");

    EXPECT_EQ(
        projCRS->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        "+proj=merc +a=6378137 +b=6378137 +lat_ts=0 +lon_0=2 +x_0=3 "
        "+y_0=4 +k=1 +units=m +nadgrids=@null +wktext +no_defs");
}

// ---------------------------------------------------------------------------

TEST(operation, webmerc_import_from_GDAL_wkt1) {

    auto projCRS = ProjectedCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Pseudo-Mercator"),
        GeographicCRS::EPSG_4326,
        Conversion::createPopularVisualisationPseudoMercator(
            PropertyMap(), Angle(0), Angle(0), Length(0), Length(0)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));

    auto wkt1 = projCRS->exportToWKT(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get());
    auto obj = WKTParser().createFromWKT(wkt1);
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto convGot = crs->derivingConversion();

    EXPECT_EQ(convGot->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"unnamed\",\n"
              "    METHOD[\"Popular Visualisation Pseudo Mercator\",\n"
              "        ID[\"EPSG\",1024]],\n"
              "    PARAMETER[\"Latitude of natural origin\",0,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",0,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",0,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",0,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");
}

// ---------------------------------------------------------------------------

TEST(operation, webmerc_import_from_GDAL_wkt1_EPSG_3785_deprecated) {

    auto wkt1 =
        "PROJCS[\"Popular Visualisation CRS / Mercator (deprecated)\","
        "    GEOGCS[\"Popular Visualisation CRS\","
        "        DATUM[\"Popular_Visualisation_Datum\","
        "            SPHEROID[\"Popular Visualisation Sphere\",6378137,0,"
        "                AUTHORITY[\"EPSG\",\"7059\"]],"
        "            TOWGS84[0,0,0,0,0,0,0],"
        "            AUTHORITY[\"EPSG\",\"6055\"]],"
        "        PRIMEM[\"Greenwich\",0,"
        "            AUTHORITY[\"EPSG\",\"8901\"]],"
        "        UNIT[\"degree\",0.0174532925199433,"
        "            AUTHORITY[\"EPSG\",\"9122\"]],"
        "        AUTHORITY[\"EPSG\",\"4055\"]],"
        "    PROJECTION[\"Mercator_1SP\"],"
        "    PARAMETER[\"central_meridian\",0],"
        "    PARAMETER[\"scale_factor\",1],"
        "    PARAMETER[\"false_easting\",0],"
        "    PARAMETER[\"false_northing\",0],"
        "    UNIT[\"metre\",1,"
        "        AUTHORITY[\"EPSG\",\"9001\"]],"
        "    AXIS[\"X\",EAST],"
        "    AXIS[\"Y\",NORTH],"
        "    EXTENSION[\"PROJ4\",\"+proj=merc +a=6378137 +b=6378137 "
        "+lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m "
        "+nadgrids=@null +wktext  +no_defs\"]]";

    auto obj = WKTParser().createFromWKT(wkt1);
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    EXPECT_EQ(crs->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=webmerc "
              "+lat_0=0 +lon_0=0 +x_0=0 +y_0=0 "
              "+ellps=WGS84");

    EXPECT_EQ(
        crs->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        "+proj=merc +a=6378137 +b=6378137 +lat_ts=0 +lon_0=0 +x_0=0 "
        "+y_0=0 +k=1 +units=m +nadgrids=@null +wktext +no_defs");

    auto convGot = crs->derivingConversion();

    EXPECT_EQ(convGot->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"unnamed\",\n"
              "    METHOD[\"Popular Visualisation Pseudo Mercator\",\n"
              "        ID[\"EPSG\",1024]],\n"
              "    PARAMETER[\"Latitude of natural origin\",0,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",0,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",0,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",0,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");
}

// ---------------------------------------------------------------------------

TEST(operation, webmerc_import_from_WKT2_EPSG_3785_deprecated) {
    auto wkt2 =
        "PROJCRS[\"Popular Visualisation CRS / Mercator\",\n"
        "    BASEGEODCRS[\"Popular Visualisation CRS\",\n"
        "        DATUM[\"Popular Visualisation Datum\",\n"
        "            ELLIPSOID[\"Popular Visualisation Sphere\",6378137,0,\n"
        "                LENGTHUNIT[\"metre\",1]]],\n"
        "        PRIMEM[\"Greenwich\",0,\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433]]],\n"
        "    CONVERSION[\"Popular Visualisation Mercator\",\n"
        "        METHOD[\"Mercator (1SP) (Spherical)\",\n"
        "            ID[\"EPSG\",9841]],\n"
        "        PARAMETER[\"Latitude of natural origin\",0,\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433],\n"
        "            ID[\"EPSG\",8801]],\n"
        "        PARAMETER[\"Longitude of natural origin\",0,\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433],\n"
        "            ID[\"EPSG\",8802]],\n"
        "        PARAMETER[\"Scale factor at natural origin\",1,\n"
        "            SCALEUNIT[\"unity\",1],\n"
        "            ID[\"EPSG\",8805]],\n"
        "        PARAMETER[\"False easting\",0,\n"
        "            LENGTHUNIT[\"metre\",1],\n"
        "            ID[\"EPSG\",8806]],\n"
        "        PARAMETER[\"False northing\",0,\n"
        "            LENGTHUNIT[\"metre\",1],\n"
        "            ID[\"EPSG\",8807]]],\n"
        "    CS[Cartesian,2],\n"
        "        AXIS[\"easting (X)\",east,\n"
        "            ORDER[1],\n"
        "            LENGTHUNIT[\"metre\",1]],\n"
        "        AXIS[\"northing (Y)\",north,\n"
        "            ORDER[2],\n"
        "            LENGTHUNIT[\"metre\",1]]]";

    auto obj = WKTParser().createFromWKT(wkt2);
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    EXPECT_EQ(crs->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=webmerc "
              "+ellps=WGS84");

    EXPECT_EQ(
        crs->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        "+proj=merc +a=6378137 +b=6378137 +lat_ts=0 +lon_0=0 +x_0=0 "
        "+y_0=0 +k=1 +units=m +nadgrids=@null +wktext +no_defs");

    EXPECT_EQ(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2015).get()),
        wkt2);

    EXPECT_EQ(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJCS[\"Popular Visualisation CRS / Mercator\",\n"
        "    GEOGCS[\"Popular Visualisation CRS\",\n"
        "        DATUM[\"Popular_Visualisation_Datum\",\n"
        "            SPHEROID[\"Popular Visualisation Sphere\",6378137,0],\n"
        "            TOWGS84[0,0,0,0,0,0,0]],\n"
        "        PRIMEM[\"Greenwich\",0],\n"
        "        UNIT[\"degree\",0.0174532925199433]],\n"
        "    PROJECTION[\"Mercator_1SP\"],\n"
        "    PARAMETER[\"central_meridian\",0],\n"
        "    PARAMETER[\"scale_factor\",1],\n"
        "    PARAMETER[\"false_easting\",0],\n"
        "    PARAMETER[\"false_northing\",0],\n"
        "    UNIT[\"metre\",1],\n"
        "    AXIS[\"Easting\",EAST],\n"
        "    AXIS[\"Northing\",NORTH],\n"
        "    EXTENSION[\"PROJ4\",\"+proj=merc +a=6378137 +b=6378137 +lat_ts=0 "
        "+lon_0=0 +x_0=0 +y_0=0 +k=1 +units=m +nadgrids=@null +wktext "
        "+no_defs\"]]");
}

// ---------------------------------------------------------------------------

TEST(operation, mollweide_export) {

    auto conv = Conversion::createMollweide(PropertyMap(), Angle(1), Length(2),
                                            Length(3));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=moll +lon_0=1 +x_0=2 +y_0=3");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Mollweide\",\n"
              "    METHOD[\"Mollweide\"],\n"
              "    PARAMETER[\"Longitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",2,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Mollweide\"],\n"
        "PARAMETER[\"central_meridian\",1],\n"
        "PARAMETER[\"false_easting\",2],\n"
        "PARAMETER[\"false_northing\",3]");
}
// ---------------------------------------------------------------------------

TEST(operation, nzmg_export) {
    auto conv = Conversion::createNewZealandMappingGrid(
        PropertyMap(), Angle(1), Angle(2), Length(4), Length(5));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=nzmg +lat_0=1 +lon_0=2 +x_0=4 +y_0=5");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"New Zealand Map Grid\",\n"
              "    METHOD[\"New Zealand Map Grid\",\n"
              "        ID[\"EPSG\",9811]],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"New_Zealand_Map_Grid\"],\n"
        "PARAMETER[\"latitude_of_origin\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"false_easting\",4],\n"
        "PARAMETER[\"false_northing\",5]");
}

// ---------------------------------------------------------------------------

TEST(operation, oblique_stereographic_export) {
    auto conv = Conversion::createObliqueStereographic(
        PropertyMap(), Angle(1), Angle(2), Scale(3), Length(4), Length(5));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=sterea +lat_0=1 +lon_0=2 +k=3 +x_0=4 +y_0=5");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Oblique Stereographic\",\n"
              "    METHOD[\"Oblique Stereographic\",\n"
              "        ID[\"EPSG\",9809]],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"Scale factor at natural origin\",3,\n"
              "        SCALEUNIT[\"unity\",1],\n"
              "        ID[\"EPSG\",8805]],\n"
              "    PARAMETER[\"False easting\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Oblique_Stereographic\"],\n"
        "PARAMETER[\"latitude_of_origin\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"scale_factor\",3],\n"
        "PARAMETER[\"false_easting\",4],\n"
        "PARAMETER[\"false_northing\",5]");
}

// ---------------------------------------------------------------------------

TEST(operation, orthographic_export) {
    auto conv = Conversion::createOrthographic(PropertyMap(), Angle(1),
                                               Angle(2), Length(4), Length(5));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=ortho +lat_0=1 +lon_0=2 +x_0=4 +y_0=5");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Orthographic\",\n"
              "    METHOD[\"Orthographic\",\n"
              "        ID[\"EPSG\",9840]],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Orthographic\"],\n"
        "PARAMETER[\"latitude_of_origin\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"false_easting\",4],\n"
        "PARAMETER[\"false_northing\",5]");
}

// ---------------------------------------------------------------------------

TEST(operation, american_polyconic_export) {
    auto conv = Conversion::createAmericanPolyconic(
        PropertyMap(), Angle(1), Angle(2), Length(4), Length(5));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=poly +lat_0=1 +lon_0=2 +x_0=4 +y_0=5");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"American Polyconic\",\n"
              "    METHOD[\"American Polyconic\",\n"
              "        ID[\"EPSG\",9818]],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Polyconic\"],\n"
        "PARAMETER[\"latitude_of_origin\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"false_easting\",4],\n"
        "PARAMETER[\"false_northing\",5]");
}

// ---------------------------------------------------------------------------

TEST(operation, polar_stereographic_variant_A_export) {
    auto conv = Conversion::createPolarStereographicVariantA(
        PropertyMap(), Angle(90), Angle(2), Scale(3), Length(4), Length(5));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=stere +lat_0=90 +lon_0=2 +k=3 +x_0=4 +y_0=5");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Polar Stereographic (variant A)\",\n"
              "    METHOD[\"Polar Stereographic (variant A)\",\n"
              "        ID[\"EPSG\",9810]],\n"
              "    PARAMETER[\"Latitude of natural origin\",90,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"Scale factor at natural origin\",3,\n"
              "        SCALEUNIT[\"unity\",1],\n"
              "        ID[\"EPSG\",8805]],\n"
              "    PARAMETER[\"False easting\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Polar_Stereographic\"],\n"
        "PARAMETER[\"latitude_of_origin\",90],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"scale_factor\",3],\n"
        "PARAMETER[\"false_easting\",4],\n"
        "PARAMETER[\"false_northing\",5]");
}

// ---------------------------------------------------------------------------

TEST(operation, polar_stereographic_variant_B_export_positive_lat) {
    auto conv = Conversion::createPolarStereographicVariantB(
        PropertyMap(), Angle(70), Angle(2), Length(4), Length(5));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=stere +lat_0=90 +lat_ts=70 +lon_0=2 +x_0=4 +y_0=5");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Polar Stereographic (variant B)\",\n"
              "    METHOD[\"Polar Stereographic (variant B)\",\n"
              "        ID[\"EPSG\",9829]],\n"
              "    PARAMETER[\"Latitude of standard parallel\",70,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8832]],\n"
              "    PARAMETER[\"Longitude of origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8833]],\n"
              "    PARAMETER[\"False easting\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Polar_Stereographic\"],\n"
        "PARAMETER[\"latitude_of_origin\",70],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"false_easting\",4],\n"
        "PARAMETER[\"false_northing\",5]");
}

// ---------------------------------------------------------------------------

TEST(operation, polar_stereographic_variant_B_export_negative_lat) {
    auto conv = Conversion::createPolarStereographicVariantB(
        PropertyMap(), Angle(-70), Angle(2), Length(4), Length(5));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=stere +lat_0=-90 +lat_ts=-70 +lon_0=2 +x_0=4 +y_0=5");
}

// ---------------------------------------------------------------------------

TEST(operation, wkt1_import_polar_stereographic_variantA) {
    auto wkt = "PROJCS[\"test\",\n"
               "    GEOGCS[\"WGS 84\",\n"
               "        DATUM[\"WGS 1984\",\n"
               "            SPHEROID[\"WGS 84\",6378137,298.257223563]],\n"
               "        PRIMEM[\"Greenwich\",0],\n"
               "        UNIT[\"degree\",0.0174532925199433]],\n"
               "    PROJECTION[\"Polar_Stereographic\"],\n"
               "    PARAMETER[\"latitude_of_origin\",-90],\n"
               "    PARAMETER[\"central_meridian\",2],\n"
               "    PARAMETER[\"scale_factor\",3],\n"
               "    PARAMETER[\"false_easting\",4],\n"
               "    PARAMETER[\"false_northing\",5]"
               "    UNIT[\"metre\",1]]";
    auto obj = WKTParser().createFromWKT(wkt);
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto conversion = crs->derivingConversion();
    auto convRef = Conversion::createPolarStereographicVariantA(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "unnamed"), Angle(-90),
        Angle(2), Scale(3), Length(4), Length(5));

    EXPECT_EQ(conversion->exportToWKT(WKTFormatter::create().get()),
              convRef->exportToWKT(WKTFormatter::create().get()));
}

// ---------------------------------------------------------------------------

TEST(operation, wkt1_import_polar_stereographic_variantB) {
    auto wkt = "PROJCS[\"test\",\n"
               "    GEOGCS[\"WGS 84\",\n"
               "        DATUM[\"WGS 1984\",\n"
               "            SPHEROID[\"WGS 84\",6378137,298.257223563]],\n"
               "        PRIMEM[\"Greenwich\",0],\n"
               "        UNIT[\"degree\",0.0174532925199433]],\n"
               "    PROJECTION[\"Polar_Stereographic\"],\n"
               "    PARAMETER[\"latitude_of_origin\",-70],\n"
               "    PARAMETER[\"central_meridian\",2],\n"
               "    PARAMETER[\"scale_factor\",1],\n"
               "    PARAMETER[\"false_easting\",4],\n"
               "    PARAMETER[\"false_northing\",5]"
               "    UNIT[\"metre\",1]]";
    auto obj = WKTParser().createFromWKT(wkt);
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto conversion = crs->derivingConversion();
    auto convRef = Conversion::createPolarStereographicVariantB(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "unnamed"), Angle(-70),
        Angle(2), Length(4), Length(5));

    EXPECT_EQ(conversion->exportToWKT(WKTFormatter::create().get()),
              convRef->exportToWKT(WKTFormatter::create().get()));
}

// ---------------------------------------------------------------------------

TEST(operation, wkt1_import_polar_stereographic_ambiguous) {
    auto wkt = "PROJCS[\"test\",\n"
               "    GEOGCS[\"WGS 84\",\n"
               "        DATUM[\"WGS 1984\",\n"
               "            SPHEROID[\"WGS 84\",6378137,298.257223563]],\n"
               "        PRIMEM[\"Greenwich\",0],\n"
               "        UNIT[\"degree\",0.0174532925199433]],\n"
               "    PROJECTION[\"Polar_Stereographic\"],\n"
               "    PARAMETER[\"latitude_of_origin\",-70],\n"
               "    PARAMETER[\"central_meridian\",2],\n"
               "    PARAMETER[\"scale_factor\",3],\n"
               "    PARAMETER[\"false_easting\",4],\n"
               "    PARAMETER[\"false_northing\",5]"
               "    UNIT[\"metre\",1]]";
    auto obj = WKTParser().createFromWKT(wkt);
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto conversion = crs->derivingConversion();
    EXPECT_EQ(conversion->method()->nameStr(), "Polar_Stereographic");
}

// ---------------------------------------------------------------------------

TEST(operation, wkt1_import_equivalent_parameters) {
    auto wkt = "PROJCS[\"test\",\n"
               "    GEOGCS[\"WGS 84\",\n"
               "        DATUM[\"WGS 1984\",\n"
               "            SPHEROID[\"WGS 84\",6378137,298.257223563]],\n"
               "        PRIMEM[\"Greenwich\",0],\n"
               "        UNIT[\"degree\",0.0174532925199433]],\n"
               "    PROJECTION[\"Hotine Oblique Mercator Two Point Natural "
               "Origin\"],\n"
               "    PARAMETER[\"latitude_of_origin\",1],\n"
               "    PARAMETER[\"Latitude_Of_1st_Point\",2],\n"
               "    PARAMETER[\"Longitude_Of_1st_Point\",3],\n"
               "    PARAMETER[\"Latitude_Of_2nd_Point\",4],\n"
               "    PARAMETER[\"Longitude_Of 2nd_Point\",5],\n"
               "    PARAMETER[\"scale_factor\",6],\n"
               "    PARAMETER[\"false_easting\",7],\n"
               "    PARAMETER[\"false_northing\",8],\n"
               "    UNIT[\"metre\",1]]";
    auto obj = WKTParser().createFromWKT(wkt);
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto conversion = crs->derivingConversion();
    auto convRef = Conversion::createHotineObliqueMercatorTwoPointNaturalOrigin(
        PropertyMap(), Angle(1), Angle(2), Angle(3), Angle(4), Angle(5),
        Scale(6), Length(7), Length(8));

    EXPECT_EQ(
        conversion->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        convRef->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()));
}

// ---------------------------------------------------------------------------

TEST(operation, robinson_export) {

    auto conv = Conversion::createRobinson(PropertyMap(), Angle(1), Length(2),
                                           Length(3));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=robin +lon_0=1 +x_0=2 +y_0=3");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Robinson\",\n"
              "    METHOD[\"Robinson\"],\n"
              "    PARAMETER[\"Longitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",2,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Robinson\"],\n"
        "PARAMETER[\"longitude_of_center\",1],\n"
        "PARAMETER[\"false_easting\",2],\n"
        "PARAMETER[\"false_northing\",3]");
}

// ---------------------------------------------------------------------------

TEST(operation, sinusoidal_export) {

    auto conv = Conversion::createSinusoidal(PropertyMap(), Angle(1), Length(2),
                                             Length(3));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=sinu +lon_0=1 +x_0=2 +y_0=3");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Sinusoidal\",\n"
              "    METHOD[\"Sinusoidal\"],\n"
              "    PARAMETER[\"Longitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",2,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Sinusoidal\"],\n"
        "PARAMETER[\"longitude_of_center\",1],\n"
        "PARAMETER[\"false_easting\",2],\n"
        "PARAMETER[\"false_northing\",3]");
}

// ---------------------------------------------------------------------------

TEST(operation, stereographic_export) {
    auto conv = Conversion::createStereographic(
        PropertyMap(), Angle(1), Angle(2), Scale(3), Length(4), Length(5));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=stere +lat_0=1 +lon_0=2 +k=3 +x_0=4 +y_0=5");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Stereographic\",\n"
              "    METHOD[\"Stereographic\"],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"Scale factor at natural origin\",3,\n"
              "        SCALEUNIT[\"unity\",1],\n"
              "        ID[\"EPSG\",8805]],\n"
              "    PARAMETER[\"False easting\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Stereographic\"],\n"
        "PARAMETER[\"latitude_of_origin\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"scale_factor\",3],\n"
        "PARAMETER[\"false_easting\",4],\n"
        "PARAMETER[\"false_northing\",5]");
}

// ---------------------------------------------------------------------------

TEST(operation, vandergrinten_export) {

    auto conv = Conversion::createVanDerGrinten(PropertyMap(), Angle(1),
                                                Length(2), Length(3));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=vandg +R_A +lon_0=1 +x_0=2 +y_0=3");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Van Der Grinten\",\n"
              "    METHOD[\"Van Der Grinten\"],\n"
              "    PARAMETER[\"Longitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",2,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"VanDerGrinten\"],\n"
        "PARAMETER[\"central_meridian\",1],\n"
        "PARAMETER[\"false_easting\",2],\n"
        "PARAMETER[\"false_northing\",3]");
}

// ---------------------------------------------------------------------------

TEST(operation, wagner_export) {

    std::vector<std::string> numbers{"", "1", "2", "3", "4", "5", "6", "7"};
    std::vector<std::string> latinNumbers{"",   "I", "II", "III",
                                          "IV", "V", "VI", "VII"};

    for (int i = 1; i <= 7; i++) {
        if (i == 3)
            continue;
        auto conv =
            (i == 1)
                ? Conversion::createWagnerI(PropertyMap(), Angle(1), Length(2),
                                            Length(3))
                : (i == 2)
                      ? Conversion::createWagnerII(PropertyMap(), Angle(1),
                                                   Length(2), Length(3))
                      : (i == 4)
                            ? Conversion::createWagnerIV(
                                  PropertyMap(), Angle(1), Length(2), Length(3))
                            : (i == 5) ? Conversion::createWagnerV(
                                             PropertyMap(), Angle(1), Length(2),
                                             Length(3))
                                       : (i == 6) ?

                                                  Conversion::createWagnerVI(
                                                      PropertyMap(), Angle(1),
                                                      Length(2), Length(3))
                                                  :

                                                  Conversion::createWagnerVII(
                                                      PropertyMap(), Angle(1),
                                                      Length(2), Length(3));

        EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
                  "+proj=wag" + numbers[i] + " +lon_0=1 +x_0=2 +y_0=3");

        auto formatter = WKTFormatter::create();
        formatter->simulCurNodeHasId();
        EXPECT_EQ(conv->exportToWKT(formatter.get()),
                  "CONVERSION[\"Wagner " + latinNumbers[i] +
                      "\",\n"
                      "    METHOD[\"Wagner " +
                      latinNumbers[i] +
                      "\"],\n"
                      "    PARAMETER[\"Longitude of natural origin\",1,\n"
                      "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
                      "        ID[\"EPSG\",8802]],\n"
                      "    PARAMETER[\"False easting\",2,\n"
                      "        LENGTHUNIT[\"metre\",1],\n"
                      "        ID[\"EPSG\",8806]],\n"
                      "    PARAMETER[\"False northing\",3,\n"
                      "        LENGTHUNIT[\"metre\",1],\n"
                      "        ID[\"EPSG\",8807]]]");

        EXPECT_EQ(conv->exportToWKT(
                      WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL)
                          .get()),
                  "PROJECTION[\"Wagner_" + latinNumbers[i] +
                      "\"],\n"
                      "PARAMETER[\"central_meridian\",1],\n"
                      "PARAMETER[\"false_easting\",2],\n"
                      "PARAMETER[\"false_northing\",3]");
    }
}

// ---------------------------------------------------------------------------

TEST(operation, wagnerIII_export) {

    auto conv = Conversion::createWagnerIII(PropertyMap(), Angle(1), Angle(2),
                                            Length(3), Length(4));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=wag3 +lat_ts=1 +lon_0=2 +x_0=3 +y_0=4");

    auto formatter = WKTFormatter::create();
    formatter->simulCurNodeHasId();
    EXPECT_EQ(conv->exportToWKT(formatter.get()),
              "CONVERSION[\"Wagner III\",\n"
              "    METHOD[\"Wagner III\"],\n"
              "    PARAMETER[\"Latitude of true scale\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Wagner_III\"],\n"
        "PARAMETER[\"latitude_of_origin\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"false_easting\",3],\n"
        "PARAMETER[\"false_northing\",4]");
}

// ---------------------------------------------------------------------------

TEST(operation, qsc_export) {

    auto conv = Conversion::createQuadrilateralizedSphericalCube(
        PropertyMap(), Angle(1), Angle(2), Length(3), Length(4));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=qsc +lat_0=1 +lon_0=2 +x_0=3 +y_0=4");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Quadrilateralized Spherical Cube\",\n"
              "    METHOD[\"Quadrilateralized Spherical Cube\"],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Quadrilateralized_Spherical_Cube\"],\n"
        "PARAMETER[\"latitude_of_origin\",1],\n"
        "PARAMETER[\"central_meridian\",2],\n"
        "PARAMETER[\"false_easting\",3],\n"
        "PARAMETER[\"false_northing\",4]");
}

// ---------------------------------------------------------------------------

TEST(operation, sch_export) {

    auto conv = Conversion::createSphericalCrossTrackHeight(
        PropertyMap(), Angle(1), Angle(2), Angle(3), Length(4));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=sch +plat_0=1 +plon_0=2 +phdg_0=3 +h_0=4");

    auto formatter = WKTFormatter::create();
    formatter->simulCurNodeHasId();
    EXPECT_EQ(conv->exportToWKT(formatter.get()),
              "CONVERSION[\"Spherical Cross-Track Height\",\n"
              "    METHOD[\"Spherical Cross-Track Height\"],\n"
              "    PARAMETER[\"Peg point latitude\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "    PARAMETER[\"Peg point longitude\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "    PARAMETER[\"Peg point heading\",3,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "    PARAMETER[\"Peg point height\",4,\n"
              "        LENGTHUNIT[\"metre\",1]]]");

    EXPECT_EQ(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        "PROJECTION[\"Spherical_Cross_Track_Height\"],\n"
        "PARAMETER[\"peg_point_latitude\",1],\n"
        "PARAMETER[\"peg_point_longitude\",2],\n"
        "PARAMETER[\"peg_point_heading\",3],\n"
        "PARAMETER[\"peg_point_height\",4]");
}

// ---------------------------------------------------------------------------

TEST(operation, conversion_inverse) {
    auto conv = Conversion::createTransverseMercator(
        PropertyMap(), Angle(1), Angle(2), Scale(3), Length(4), Length(5));
    auto inv = conv->inverse();
    EXPECT_EQ(inv->inverse(), conv);
    EXPECT_EQ(inv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Inverse of Transverse Mercator\",\n"
              "    METHOD[\"Inverse of Transverse Mercator\",\n"
              "        ID[\"INVERSE(EPSG)\",9807]],\n"
              "    PARAMETER[\"Latitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8801]],\n"
              "    PARAMETER[\"Longitude of natural origin\",2,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"Scale factor at natural origin\",3,\n"
              "        SCALEUNIT[\"unity\",1],\n"
              "        ID[\"EPSG\",8805]],\n"
              "    PARAMETER[\"False easting\",4,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",5,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");

    EXPECT_EQ(inv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +inv +proj=tmerc +lat_0=1 +lon_0=2 +k_0=3 "
              "+x_0=4 +y_0=5");

    EXPECT_TRUE(inv->isEquivalentTo(inv.get()));
    EXPECT_FALSE(inv->isEquivalentTo(createUnrelatedObject().get()));
}

// ---------------------------------------------------------------------------

TEST(operation, eqearth_export) {

    auto conv = Conversion::createEqualEarth(PropertyMap(), Angle(1), Length(2),
                                             Length(3));

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=eqearth +lon_0=1 +x_0=2 +y_0=3");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"Equal Earth\",\n"
              "    METHOD[\"Equal Earth\",\n"
              "        ID[\"EPSG\",1078]],\n"
              "    PARAMETER[\"Longitude of natural origin\",1,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
              "        ID[\"EPSG\",8802]],\n"
              "    PARAMETER[\"False easting\",2,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8806]],\n"
              "    PARAMETER[\"False northing\",3,\n"
              "        LENGTHUNIT[\"metre\",1],\n"
              "        ID[\"EPSG\",8807]]]");
}

// ---------------------------------------------------------------------------

TEST(operation, laborde_oblique_mercator) {

    // Content of EPSG:29701 "Tananarive (Paris) / Laborde Grid"
    auto projString = "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
                      "+proj=unitconvert +xy_in=grad +xy_out=rad +step +inv "
                      "+proj=longlat +ellps=intl +pm=paris +step +proj=labrd "
                      "+lat_0=-18.9 +lon_0=44.1 +azi=18.9 +k=0.9995 "
                      "+x_0=400000 +y_0=800000 +ellps=intl +pm=paris +step "
                      "+proj=axisswap +order=2,1";
    auto obj = PROJStringParser().createFromPROJString(projString);
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);
    EXPECT_EQ(crs->exportToPROJString(PROJStringFormatter::create().get()),
              projString);
}

// ---------------------------------------------------------------------------

TEST(operation, PROJ_based) {
    auto conv = SingleOperation::createPROJBased(PropertyMap(), "+proj=merc",
                                                 nullptr, nullptr);

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=merc");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"PROJ-based coordinate operation\",\n"
              "    METHOD[\"PROJ-based operation method\"],\n"
              "    PARAMETER[\"PROJ string\",\"+proj=merc\"]]");

    EXPECT_EQ(conv->inverse()->exportToPROJString(
                  PROJStringFormatter::create().get()),
              "+proj=pipeline +step +inv +proj=merc");

    auto str = "+proj=pipeline +step +proj=unitconvert +xy_in=grad +xy_out=rad "
               "+step +proj=axisswap +order=2,1 +step +proj=longlat "
               "+ellps=clrk80ign +pm=paris +step +proj=axisswap +order=2,1";
    EXPECT_EQ(
        SingleOperation::createPROJBased(PropertyMap(), str, nullptr, nullptr)
            ->exportToPROJString(PROJStringFormatter::create().get()),
        str);

    EXPECT_THROW(SingleOperation::createPROJBased(PropertyMap(), "+inv",
                                                  nullptr, nullptr)
                     ->exportToPROJString(PROJStringFormatter::create().get()),
                 FormattingException);
    EXPECT_THROW(
        SingleOperation::createPROJBased(PropertyMap(), "foo", nullptr, nullptr)
            ->exportToPROJString(PROJStringFormatter::create().get()),
        FormattingException);
}

// ---------------------------------------------------------------------------

TEST(operation, PROJ_based_empty) {
    auto conv =
        SingleOperation::createPROJBased(PropertyMap(), "", nullptr, nullptr);

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "");

    EXPECT_EQ(conv->exportToWKT(WKTFormatter::create().get()),
              "CONVERSION[\"PROJ-based coordinate operation\",\n"
              "    METHOD[\"PROJ-based operation method\"],\n"
              "    PARAMETER[\"PROJ string\",\"\"]]");

    EXPECT_THROW(
        conv->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        FormattingException);

    EXPECT_EQ(conv->inverse()->exportToPROJString(
                  PROJStringFormatter::create().get()),
              "");
}

// ---------------------------------------------------------------------------

TEST(operation, PROJ_based_with_global_parameters) {
    auto conv = SingleOperation::createPROJBased(
        PropertyMap(), "+proj=pipeline +ellps=WGS84 +step +proj=longlat",
        nullptr, nullptr);

    EXPECT_EQ(conv->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +ellps=WGS84 +step +proj=longlat");
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_geogCRS) {

    auto op = CoordinateOperationFactory::create()->createOperation(
        GeographicCRS::EPSG_4807, GeographicCRS::EPSG_4326);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(
        op->exportToPROJString(PROJStringFormatter::create().get()),
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=grad +xy_out=rad +step +inv +proj=longlat "
        "+ellps=clrk80ign +pm=paris +step +proj=unitconvert +xy_in=rad "
        "+xy_out=deg +step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_geogCRS_context_default) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0);
    ctxt->setSpatialCriterion(
        CoordinateOperationContext::SpatialCriterion::PARTIAL_INTERSECTION);
    ctxt->setAllowUseIntermediateCRS(false);

    // Directly found in database
    {
        auto list = CoordinateOperationFactory::create()->createOperations(
            authFactory->createCoordinateReferenceSystem("4179"), // Pulkovo 42
            authFactory->createCoordinateReferenceSystem("4258"), // ETRS89
            ctxt);
        ASSERT_EQ(list.size(), 2);
        // Romania has a larger area than Poland (given our approx formula)
        EXPECT_EQ(list[0]->getEPSGCode(), 15994); // Romania - 3m
        EXPECT_EQ(list[1]->getEPSGCode(), 1644);  // Poland - 1m

        EXPECT_EQ(
            list[0]->exportToPROJString(PROJStringFormatter::create().get()),
            "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
            "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=cart "
            "+ellps=krass +step +proj=helmert +x=2.3287 +y=-147.0425 "
            "+z=-92.0802 +rx=0.3092483 +ry=-0.32482185 +rz=-0.49729934 "
            "+s=5.68906266 +convention=coordinate_frame +step +inv "
            "+proj=cart +ellps=GRS80 +step +proj=unitconvert +xy_in=rad "
            "+xy_out=deg +step +proj=axisswap +order=2,1");
    }

    // Reverse case
    {
        auto list = CoordinateOperationFactory::create()->createOperations(
            authFactory->createCoordinateReferenceSystem("4258"),
            authFactory->createCoordinateReferenceSystem("4179"), ctxt);
        ASSERT_EQ(list.size(), 2);
        // Romania has a larger area than Poland (given our approx formula)
        EXPECT_EQ(list[0]->nameStr(),
                  "Inverse of Pulkovo 1942(58) to ETRS89 (4)"); // Romania - 3m

        EXPECT_EQ(
            list[0]->exportToPROJString(PROJStringFormatter::create().get()),
            "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
            "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=cart "
            "+ellps=GRS80 +step +inv +proj=helmert +x=2.3287 "
            "+y=-147.0425 +z=-92.0802 +rx=0.3092483 +ry=-0.32482185 "
            "+rz=-0.49729934 +s=5.68906266 +convention=coordinate_frame "
            "+step +inv +proj=cart +ellps=krass +step +proj=unitconvert "
            "+xy_in=rad +xy_out=deg +step +proj=axisswap +order=2,1");
    }
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_geogCRS_context_filter_accuracy) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    {
        auto ctxt =
            CoordinateOperationContext::create(authFactory, nullptr, 1.0);
        ctxt->setSpatialCriterion(
            CoordinateOperationContext::SpatialCriterion::PARTIAL_INTERSECTION);

        auto list = CoordinateOperationFactory::create()->createOperations(
            authFactory->createCoordinateReferenceSystem("4179"),
            authFactory->createCoordinateReferenceSystem("4258"), ctxt);
        ASSERT_EQ(list.size(), 1);
        EXPECT_EQ(list[0]->getEPSGCode(), 1644); // Poland - 1m
    }
    {
        auto ctxt =
            CoordinateOperationContext::create(authFactory, nullptr, 0.9);
        ctxt->setSpatialCriterion(
            CoordinateOperationContext::SpatialCriterion::PARTIAL_INTERSECTION);

        auto list = CoordinateOperationFactory::create()->createOperations(
            authFactory->createCoordinateReferenceSystem("4179"),
            authFactory->createCoordinateReferenceSystem("4258"), ctxt);
        ASSERT_EQ(list.size(), 0);
    }
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_geogCRS_context_filter_bbox) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    // INSERT INTO "area" VALUES('EPSG','1197','Romania','Romania - onshore and
    // offshore.',43.44,48.27,20.26,31.41,0);
    {
        auto ctxt = CoordinateOperationContext::create(
            authFactory, Extent::createFromBBOX(20.26, 43.44, 31.41, 48.27),
            0.0);
        auto list = CoordinateOperationFactory::create()->createOperations(
            authFactory->createCoordinateReferenceSystem("4179"),
            authFactory->createCoordinateReferenceSystem("4258"), ctxt);
        ASSERT_EQ(list.size(), 1);
        EXPECT_EQ(list[0]->getEPSGCode(), 15994); // Romania - 3m
    }
    {
        auto ctxt = CoordinateOperationContext::create(
            authFactory, Extent::createFromBBOX(20.26 + .1, 43.44 + .1,
                                                31.41 - .1, 48.27 - .1),
            0.0);
        auto list = CoordinateOperationFactory::create()->createOperations(
            authFactory->createCoordinateReferenceSystem("4179"),
            authFactory->createCoordinateReferenceSystem("4258"), ctxt);
        ASSERT_EQ(list.size(), 1);
        EXPECT_EQ(list[0]->getEPSGCode(), 15994); // Romania - 3m
    }
    {
        auto ctxt = CoordinateOperationContext::create(
            authFactory, Extent::createFromBBOX(20.26 - .1, 43.44 - .1,
                                                31.41 + .1, 48.27 + .1),
            0.0);
        auto list = CoordinateOperationFactory::create()->createOperations(
            authFactory->createCoordinateReferenceSystem("4179"),
            authFactory->createCoordinateReferenceSystem("4258"), ctxt);
        ASSERT_EQ(list.size(), 1);
        EXPECT_EQ(
            list[0]->exportToPROJString(PROJStringFormatter::create().get()),
            "");
    }
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_geogCRS_context_incompatible_area) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        authFactory->createCoordinateReferenceSystem("4267"), // NAD27
        authFactory->createCoordinateReferenceSystem("4258"), // ETRS 89
        ctxt);
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "");
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_geogCRS_context_inverse_needed) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    {
        auto ctxt =
            CoordinateOperationContext::create(authFactory, nullptr, 0.0);
        ctxt->setUsePROJAlternativeGridNames(false);
        auto list = CoordinateOperationFactory::create()->createOperations(
            authFactory->createCoordinateReferenceSystem("4275"), // NTF
            authFactory->createCoordinateReferenceSystem("4258"), // ETRS89
            ctxt);
        ASSERT_EQ(list.size(), 3);
        EXPECT_EQ(
            list[0]->exportToPROJString(PROJStringFormatter::create().get()),
            "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
            "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=cart "
            "+ellps=clrk80ign +step +proj=helmert +x=-168 +y=-60 +z=320 "
            "+step +inv +proj=cart +ellps=GRS80 +step +proj=unitconvert "
            "+xy_in=rad +xy_out=deg +step +proj=axisswap +order=2,1");
        EXPECT_EQ(list[1]->exportToPROJString(
                      PROJStringFormatter::create(
                          PROJStringFormatter::Convention::PROJ_5,
                          authFactory->databaseContext())
                          .get()),
                  "");
        EXPECT_EQ(list[2]->exportToPROJString(
                      PROJStringFormatter::create(
                          PROJStringFormatter::Convention::PROJ_5,
                          authFactory->databaseContext())
                          .get()),
                  "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
                  "+proj=unitconvert +xy_in=deg +xy_out=rad +step "
                  "+proj=hgridshift +grids=ntf_r93.gsb +step +proj=unitconvert "
                  "+xy_in=rad +xy_out=deg +step +proj=axisswap +order=2,1");
    }
    {
        auto ctxt =
            CoordinateOperationContext::create(authFactory, nullptr, 0.0);
        auto list = CoordinateOperationFactory::create()->createOperations(
            authFactory->createCoordinateReferenceSystem("4275"), // NTF
            authFactory->createCoordinateReferenceSystem("4258"), // ETRS89
            ctxt);
        ASSERT_EQ(list.size(), 2);
        EXPECT_EQ(
            list[0]->exportToPROJString(PROJStringFormatter::create().get()),
            "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
            "+proj=unitconvert +xy_in=deg +xy_out=rad +step "
            "+proj=hgridshift +grids=ntf_r93.gsb +step +proj=unitconvert "
            "+xy_in=rad +xy_out=deg +step +proj=axisswap +order=2,1");
    }
    {
        auto ctxt =
            CoordinateOperationContext::create(authFactory, nullptr, 0.0);
        auto list = CoordinateOperationFactory::create()->createOperations(
            authFactory->createCoordinateReferenceSystem("4258"), // ETRS89
            authFactory->createCoordinateReferenceSystem("4275"), // NTF
            ctxt);
        ASSERT_EQ(list.size(), 2);
        EXPECT_EQ(
            list[0]->exportToPROJString(PROJStringFormatter::create().get()),
            "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
            "+proj=unitconvert +xy_in=deg +xy_out=rad +step +inv "
            "+proj=hgridshift +grids=ntf_r93.gsb +step +proj=unitconvert "
            "+xy_in=rad +xy_out=deg +step +proj=axisswap +order=2,1");
    }
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_geogCRS_context_ntv1_ntv2_ctable2) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    ctxt->setSpatialCriterion(
        CoordinateOperationContext::SpatialCriterion::PARTIAL_INTERSECTION);
    ctxt->setGridAvailabilityUse(
        CoordinateOperationContext::GridAvailabilityUse::
            IGNORE_GRID_AVAILABILITY);

    auto list = CoordinateOperationFactory::create()->createOperations(
        authFactory->createCoordinateReferenceSystem("4267"), // NAD27
        authFactory->createCoordinateReferenceSystem("4269"), // NAD83
        ctxt);
    ASSERT_EQ(list.size(), 6);
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=hgridshift "
              "+grids=ntv1_can.dat +step +proj=unitconvert +xy_in=rad "
              "+xy_out=deg +step +proj=axisswap +order=2,1");
    EXPECT_EQ(list[1]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=hgridshift "
              "+grids=ntv2_0.gsb +step +proj=unitconvert +xy_in=rad "
              "+xy_out=deg +step +proj=axisswap +order=2,1");
    EXPECT_EQ(list[2]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=hgridshift "
              "+grids=conus +step +proj=unitconvert +xy_in=rad +xy_out=deg "
              "+step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, vertCRS_to_geogCRS_context) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    {
        auto ctxt =
            CoordinateOperationContext::create(authFactory, nullptr, 0.0);
        ctxt->setUsePROJAlternativeGridNames(false);
        auto list = CoordinateOperationFactory::create()->createOperations(
            authFactory->createCoordinateReferenceSystem(
                "3855"), // EGM2008 height
            authFactory->createCoordinateReferenceSystem("4979"), // WGS 84
            ctxt);
        ASSERT_EQ(list.size(), 2);
        EXPECT_EQ(list[1]->exportToPROJString(
                      PROJStringFormatter::create(
                          PROJStringFormatter::Convention::PROJ_5,
                          authFactory->databaseContext())
                          .get()),
                  "+proj=vgridshift +grids=egm08_25.gtx");
    }
    {
        auto ctxt =
            CoordinateOperationContext::create(authFactory, nullptr, 0.0);
        auto list = CoordinateOperationFactory::create()->createOperations(
            authFactory->createCoordinateReferenceSystem(
                "3855"), // EGM2008 height
            authFactory->createCoordinateReferenceSystem("4979"), // WGS 84
            ctxt);
        ASSERT_EQ(list.size(), 2);
        EXPECT_EQ(
            list[0]->exportToPROJString(PROJStringFormatter::create().get()),
            "+proj=vgridshift +grids=egm08_25.gtx");
    }
    {
        auto ctxt =
            CoordinateOperationContext::create(authFactory, nullptr, 0.0);
        auto list = CoordinateOperationFactory::create()->createOperations(
            authFactory->createCoordinateReferenceSystem("4979"), // WGS 84
            authFactory->createCoordinateReferenceSystem(
                "3855"), // EGM2008 height
            ctxt);
        ASSERT_EQ(list.size(), 2);
        EXPECT_EQ(
            list[0]->exportToPROJString(PROJStringFormatter::create().get()),
            "+proj=pipeline +step +inv +proj=vgridshift +grids=egm08_25.gtx");
    }
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_geogCRS_noop) {

    auto op = CoordinateOperationFactory::create()->createOperation(
        GeographicCRS::EPSG_4326, GeographicCRS::EPSG_4326);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->nameStr(), "Null geographic offset from WGS 84 to WGS 84");
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()), "");
    EXPECT_EQ(op->inverse()->nameStr(), op->nameStr());
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_geogCRS_longitude_rotation) {

    auto src = GeographicCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "A"),
        GeodeticReferenceFrame::create(PropertyMap(), Ellipsoid::WGS84,
                                       optional<std::string>(),
                                       PrimeMeridian::GREENWICH),
        EllipsoidalCS::createLatitudeLongitude(UnitOfMeasure::DEGREE));
    auto dest = GeographicCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "B"),
        GeodeticReferenceFrame::create(PropertyMap(), Ellipsoid::WGS84,
                                       optional<std::string>(),
                                       PrimeMeridian::PARIS),
        EllipsoidalCS::createLatitudeLongitude(UnitOfMeasure::DEGREE));

    auto op = CoordinateOperationFactory::create()->createOperation(src, dest);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=longlat "
              "+ellps=WGS84 +pm=paris +step +proj=unitconvert +xy_in=rad "
              "+xy_out=deg +step +proj=axisswap +order=2,1");
    EXPECT_EQ(op->inverse()->exportToWKT(WKTFormatter::create().get()),
              CoordinateOperationFactory::create()
                  ->createOperation(dest, src)
                  ->exportToWKT(WKTFormatter::create().get()));
    EXPECT_TRUE(
        op->inverse()->isEquivalentTo(CoordinateOperationFactory::create()
                                          ->createOperation(dest, src)
                                          .get()));
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_geogCRS_longitude_rotation_context) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        authFactory->createCoordinateReferenceSystem("4807"), // NTF(Paris)
        authFactory->createCoordinateReferenceSystem("4275"), // NTF
        ctxt);
    ASSERT_EQ(list.size(), 2);
    EXPECT_EQ(list[0]->nameStr(), "NTF (Paris) to NTF (1)");
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=grad +xy_out=rad +step +inv "
              "+proj=longlat +ellps=clrk80ign +pm=paris +step "
              "+proj=unitconvert +xy_in=rad +xy_out=deg +step +proj=axisswap "
              "+order=2,1");
    EXPECT_EQ(list[1]->nameStr(), "NTF (Paris) to NTF (2)");
    EXPECT_EQ(list[1]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=grad +xy_out=rad +step +inv "
              "+proj=longlat +ellps=clrk80ign +pm=2.33720833333333 +step "
              "+proj=unitconvert +xy_in=rad +xy_out=deg +step +proj=axisswap "
              "+order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_geogCRS_context_concatenated_operation) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        authFactory->createCoordinateReferenceSystem("4807"), // NTF(Paris)
        authFactory->createCoordinateReferenceSystem("4171"), // RGF93
        ctxt);
    ASSERT_EQ(list.size(), 3);
    EXPECT_EQ(list[0]->nameStr(), "NTF (Paris) to RGF93 (2)");
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=grad +xy_out=rad +step +inv "
              "+proj=longlat +ellps=clrk80ign +pm=paris +step +proj=hgridshift "
              "+grids=ntf_r93.gsb +step +proj=unitconvert +xy_in=rad "
              "+xy_out=deg +step +proj=axisswap +order=2,1");

    EXPECT_TRUE(nn_dynamic_pointer_cast<ConcatenatedOperation>(list[0]) !=
                nullptr);
    auto grids = list[0]->gridsNeeded(DatabaseContext::create());
    EXPECT_EQ(grids.size(), 1);
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_geogCRS_context_same_grid_name) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        authFactory->createCoordinateReferenceSystem("4314"), // DHDN
        authFactory->createCoordinateReferenceSystem("4258"), // ETRS89
        ctxt);
    ASSERT_TRUE(!list.empty());
    EXPECT_EQ(list[0]->nameStr(), "DHDN to ETRS89 (8)");
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=hgridshift "
              "+grids=BETA2007.gsb +step +proj=unitconvert +xy_in=rad "
              "+xy_out=deg +step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_geogCRS_geographic_offset_context) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        authFactory->createCoordinateReferenceSystem("4120"), // NTF(Paris)
        authFactory->createCoordinateReferenceSystem("4121"), // NTF
        ctxt);
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list[0]->nameStr(), "Greek to GGRS87 (1)");
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=geogoffset "
              "+dlat=-5.86 +dlon=0.28 +step +proj=unitconvert +xy_in=rad "
              "+xy_out=deg +step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_geogCRS_CH1903_to_CH1903plus_context) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        authFactory->createCoordinateReferenceSystem("4149"), // CH1903
        authFactory->createCoordinateReferenceSystem("4150"), // CH1903+
        ctxt);
    ASSERT_EQ(list.size(), 2);

    EXPECT_EQ(list[0]->nameStr(),
              "CH1903 to ETRS89 (1) + Inverse of CH1903+ to ETRS89 (1)");
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "");

    EXPECT_EQ(list[1]->nameStr(), "CH1903 to CH1903+ (1)");
    EXPECT_EQ(list[1]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 "
              "+step +proj=unitconvert +xy_in=deg +xy_out=rad "
              "+step +proj=hgridshift +grids=CHENyx06a.gsb "
              "+step +proj=unitconvert +xy_in=rad +xy_out=deg "
              "+step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_geogCRS_3D) {

    auto geogcrs_m_obj =
        PROJStringParser().createFromPROJString("+proj=longlat +vunits=m");
    auto geogcrs_m = nn_dynamic_pointer_cast<CRS>(geogcrs_m_obj);
    ASSERT_TRUE(geogcrs_m != nullptr);

    auto geogcrs_ft_obj =
        PROJStringParser().createFromPROJString("+proj=longlat +vunits=ft");
    auto geogcrs_ft = nn_dynamic_pointer_cast<CRS>(geogcrs_ft_obj);
    ASSERT_TRUE(geogcrs_ft != nullptr);

    {
        auto op = CoordinateOperationFactory::create()->createOperation(
            NN_CHECK_ASSERT(geogcrs_m), NN_CHECK_ASSERT(geogcrs_ft));
        ASSERT_TRUE(op != nullptr);
        EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
                  "+proj=unitconvert +z_in=m +z_out=ft");
    }

    {
        auto op = CoordinateOperationFactory::create()->createOperation(
            NN_CHECK_ASSERT(geogcrs_ft), NN_CHECK_ASSERT(geogcrs_m));
        ASSERT_TRUE(op != nullptr);
        EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
                  "+proj=unitconvert +z_in=ft +z_out=m");
    }

    auto geogcrs_m_with_pm_obj = PROJStringParser().createFromPROJString(
        "+proj=longlat +pm=paris +vunits=m");
    auto geogcrs_m_with_pm =
        nn_dynamic_pointer_cast<CRS>(geogcrs_m_with_pm_obj);
    ASSERT_TRUE(geogcrs_m_with_pm != nullptr);

    {
        auto op = CoordinateOperationFactory::create()->createOperation(
            NN_CHECK_ASSERT(geogcrs_m_with_pm), NN_CHECK_ASSERT(geogcrs_ft));
        ASSERT_TRUE(op != nullptr);
        EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
                  "+proj=pipeline +step +proj=unitconvert +xy_in=deg +z_in=m "
                  "+xy_out=rad +z_out=m +step +inv +proj=longlat +ellps=WGS84 "
                  "+pm=paris +step +proj=unitconvert +xy_in=rad +z_in=m "
                  "+xy_out=deg +z_out=ft");
    }
}

// ---------------------------------------------------------------------------

TEST(operation, geocentricCRS_to_geogCRS_same_datum) {

    auto op = CoordinateOperationFactory::create()->createOperation(
        createGeocentricDatumWGS84(), GeographicCRS::EPSG_4326);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +inv +proj=cart +ellps=WGS84 +step "
              "+proj=unitconvert +xy_in=rad +xy_out=deg +step +proj=axisswap "
              "+order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, geocentricCRS_to_geogCRS_different_datum) {

    auto op = CoordinateOperationFactory::create()->createOperation(
        createGeocentricDatumWGS84(), GeographicCRS::EPSG_4269);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->nameStr(), "Null geocentric translation from WGS 84 to NAD83 "
                             "(geocentric) + Conversion from NAD83 "
                             "(geocentric) to NAD83");
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +inv +proj=cart +ellps=GRS80 +step "
              "+proj=unitconvert +xy_in=rad +xy_out=deg +step +proj=axisswap "
              "+order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_geocentricCRS_different_datum) {

    auto op = CoordinateOperationFactory::create()->createOperation(
        GeographicCRS::EPSG_4269, createGeocentricDatumWGS84());
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->nameStr(), "Conversion from NAD83 to NAD83 (geocentric) + "
                             "Null geocentric translation from NAD83 "
                             "(geocentric) to WGS 84");
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=cart "
              "+ellps=GRS80");
}

// ---------------------------------------------------------------------------

TEST(operation, geocentricCRS_to_geocentricCRS_noop) {

    auto op = CoordinateOperationFactory::create()->createOperation(
        createGeocentricDatumWGS84(), createGeocentricDatumWGS84());
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->nameStr(),
              "Null geocentric translation from WGS 84 to WGS 84");
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()), "");
    EXPECT_EQ(op->inverse()->nameStr(), op->nameStr());
}

// ---------------------------------------------------------------------------

TEST(operation, geocentricCRS_to_geogCRS_same_datum_context) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        authFactory->createCoordinateReferenceSystem("4326"),
        // WGS84 geocentric
        authFactory->createCoordinateReferenceSystem("4978"), ctxt);
    ASSERT_EQ(list.size(), 1);

    EXPECT_EQ(list[0]->nameStr(),
              "Conversion from WGS 84 (geog2D) to WGS 84 (geocentric)");
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=cart "
              "+ellps=WGS84");

    EXPECT_EQ(list[0]->inverse()->nameStr(),
              "Conversion from WGS 84 (geocentric) to WGS 84 (geog2D)");
    EXPECT_EQ(list[0]->inverse()->exportToPROJString(
                  PROJStringFormatter::create().get()),
              "+proj=pipeline +step +inv +proj=cart +ellps=WGS84 +step "
              "+proj=unitconvert +xy_in=rad +xy_out=deg +step +proj=axisswap "
              "+order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, geocentricCRS_to_geogCRS_same_datum_context_all_auth) {
    // This is to check we don't use OGC:CRS84 as a pivot
    auto authFactoryEPSG =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto authFactoryAll =
        AuthorityFactory::create(DatabaseContext::create(), std::string());
    auto ctxt =
        CoordinateOperationContext::create(authFactoryAll, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        authFactoryEPSG->createCoordinateReferenceSystem("4326"),
        // WGS84 geocentric
        authFactoryEPSG->createCoordinateReferenceSystem("4978"), ctxt);
    ASSERT_EQ(list.size(), 1);

    EXPECT_EQ(list[0]->nameStr(),
              "Conversion from WGS 84 (geog2D) to WGS 84 (geocentric)");
}

// ---------------------------------------------------------------------------

TEST(operation, geocentricCRS_to_geocentricCRS_different_datum_context) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        // ITRF2000 (geocentric)
        authFactory->createCoordinateReferenceSystem("4919"),
        // ITRF2005 (geocentric)
        authFactory->createCoordinateReferenceSystem("4896"), ctxt);
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list[0]->nameStr(), "ITRF2000 to ITRF2005 (1)");
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=helmert +x=-0.0001 "
              "+y=0.0008 +z=0.0058 +rx=0 +ry=0 +rz=0 +s=-0.0004 +dx=0.0002 "
              "+dy=-0.0001 +dz=0.0018 +drx=0 +dry=0 +drz=0 +ds=-8e-05 "
              "+t_epoch=2000 +convention=position_vector");
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_geocentricCRS_same_datum_to_context) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        // WGS84 geocentric
        authFactory->createCoordinateReferenceSystem("4978"),
        authFactory->createCoordinateReferenceSystem("4326"), ctxt);
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list[0]->nameStr(),
              "Conversion from WGS 84 (geocentric) to WGS 84 (geog2D)");
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +inv +proj=cart +ellps=WGS84 +step "
              "+proj=unitconvert +xy_in=rad +xy_out=deg +step +proj=axisswap "
              "+order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation,
     geogCRS_to_geogCRS_different_datum_though_geocentric_transform_context) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        // ITRF2000 (geog3D)
        authFactory->createCoordinateReferenceSystem("7909"),
        // ITRF2005 (geog3D)
        authFactory->createCoordinateReferenceSystem("7910"), ctxt);
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list[0]->nameStr(),
              "Conversion from ITRF2000 (geog3D) to ITRF2000 (geocentric) + "
              "ITRF2000 to ITRF2005 (1) + "
              "Conversion from ITRF2005 (geocentric) to ITRF2005 (geog3D)");
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +z_in=m +xy_out=rad +z_out=m "
              "+step +proj=cart +ellps=GRS80 +step +proj=helmert +x=-0.0001 "
              "+y=0.0008 +z=0.0058 +rx=0 +ry=0 +rz=0 +s=-0.0004 +dx=0.0002 "
              "+dy=-0.0001 +dz=0.0018 +drx=0 +dry=0 +drz=0 +ds=-8e-05 "
              "+t_epoch=2000 +convention=position_vector +step +inv "
              "+proj=cart +ellps=GRS80 +step +proj=unitconvert +xy_in=rad "
              "+z_in=m +xy_out=deg +z_out=m +step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_geocentricCRS_different_datum_context) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        // ITRF2000 (geog3D)
        authFactory->createCoordinateReferenceSystem("7909"),
        // ITRF2005 (geocentric)
        authFactory->createCoordinateReferenceSystem("4896"), ctxt);
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list[0]->nameStr(),
              "Conversion from ITRF2000 (geog3D) to ITRF2000 (geocentric) + "
              "ITRF2000 to ITRF2005 (1)");
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +z_in=m +xy_out=rad +z_out=m "
              "+step +proj=cart +ellps=GRS80 +step +proj=helmert +x=-0.0001 "
              "+y=0.0008 +z=0.0058 +rx=0 +ry=0 +rz=0 +s=-0.0004 +dx=0.0002 "
              "+dy=-0.0001 +dz=0.0018 +drx=0 +dry=0 +drz=0 +ds=-8e-05 "
              "+t_epoch=2000 +convention=position_vector");
}

// ---------------------------------------------------------------------------

TEST(operation, geocentricCRS_to_geogCRS_different_datum_context) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        // ITRF2000 (geocentric)
        authFactory->createCoordinateReferenceSystem("4919"),
        // ITRF2005 (geog3D)
        authFactory->createCoordinateReferenceSystem("7910"), ctxt);
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list[0]->nameStr(),
              "ITRF2000 to ITRF2005 (1) + "
              "Conversion from ITRF2005 (geocentric) to ITRF2005 (geog3D)");
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=helmert +x=-0.0001 "
              "+y=0.0008 +z=0.0058 +rx=0 +ry=0 +rz=0 +s=-0.0004 +dx=0.0002 "
              "+dy=-0.0001 +dz=0.0018 +drx=0 +dry=0 +drz=0 +ds=-8e-05 "
              "+t_epoch=2000 +convention=position_vector +step +inv "
              "+proj=cart +ellps=GRS80 +step +proj=unitconvert +xy_in=rad "
              "+z_in=m +xy_out=deg +z_out=m +step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, esri_projectedCRS_to_geogCRS_with_ITRF_intermediate_context) {
    auto dbContext = DatabaseContext::create();
    auto authFactoryEPSG = AuthorityFactory::create(dbContext, "EPSG");
    auto authFactoryESRI = AuthorityFactory::create(dbContext, "ESRI");
    auto ctxt =
        CoordinateOperationContext::create(authFactoryEPSG, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        // NAD_1983_CORS96_StatePlane_North_Carolina_FIPS_3200_Ft_US (projected)
        authFactoryESRI->createCoordinateReferenceSystem("103501"),
        // ITRF2005 (geog3D)
        authFactoryEPSG->createCoordinateReferenceSystem("7910"), ctxt);
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list[0]->nameStr(),
              "Inverse of NAD_1983_CORS96_StatePlane_North_Carolina_"
              "FIPS_3200_Ft_US + "
              "Conversion from NAD83(CORS96) (geog2D) to NAD83(CORS96) "
              "(geocentric) + Inverse of ITRF2000 to NAD83(CORS96) (1) + "
              "ITRF2000 to ITRF2005 (1) + "
              "Conversion from ITRF2005 (geocentric) to ITRF2005 (geog3D)");
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=unitconvert +xy_in=us-ft +z_in=us-ft "
              "+xy_out=m +z_out=m +step +inv +proj=lcc +lat_0=33.75 +lon_0=-79 "
              "+lat_1=34.3333333333333 +lat_2=36.1666666666667 "
              "+x_0=609601.219202438 +y_0=0 +ellps=GRS80 +step +proj=cart "
              "+ellps=GRS80 +step +inv +proj=helmert +x=0.9956 +y=-1.9013 "
              "+z=-0.5215 +rx=0.025915 +ry=0.009426 +rz=0.011599 +s=0.00062 "
              "+dx=0.0007 +dy=-0.0007 +dz=0.0005 +drx=6.7e-05 +dry=-0.000757 "
              "+drz=-5.1e-05 +ds=-0.00018 +t_epoch=1997 "
              "+convention=coordinate_frame +step +proj=helmert +x=-0.0001 "
              "+y=0.0008 +z=0.0058 +rx=0 +ry=0 +rz=0 +s=-0.0004 +dx=0.0002 "
              "+dy=-0.0001 +dz=0.0018 +drx=0 +dry=0 +drz=0 +ds=-8e-05 "
              "+t_epoch=2000 +convention=position_vector +step +inv +proj=cart "
              "+ellps=GRS80 +step +proj=unitconvert +xy_in=rad +z_in=m "
              "+xy_out=deg +z_out=m +step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

static ProjectedCRSNNPtr createUTM31_WGS84() {
    return ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4326,
        Conversion::createUTM(PropertyMap(), 31, true),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
}

// ---------------------------------------------------------------------------

static ProjectedCRSNNPtr createUTM32_WGS84() {
    return ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4326,
        Conversion::createUTM(PropertyMap(), 32, true),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_projCRS) {

    auto op = CoordinateOperationFactory::create()->createOperation(
        GeographicCRS::EPSG_4326, createUTM31_WGS84());
    ASSERT_TRUE(op != nullptr);
    EXPECT_TRUE(std::dynamic_pointer_cast<Conversion>(op) != nullptr);
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=utm "
              "+zone=31 +ellps=WGS84");
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_longlat_to_geogCS_latlong) {

    auto sourceCRS = GeographicCRS::OGC_CRS84;
    auto targetCRS = GeographicCRS::EPSG_4326;
    auto op = CoordinateOperationFactory::create()->createOperation(sourceCRS,
                                                                    targetCRS);
    ASSERT_TRUE(op != nullptr);
    auto conv = std::dynamic_pointer_cast<Conversion>(op);
    ASSERT_TRUE(conv != nullptr);
    EXPECT_TRUE(op->sourceCRS() &&
                op->sourceCRS()->isEquivalentTo(sourceCRS.get()));
    EXPECT_TRUE(op->targetCRS() &&
                op->targetCRS()->isEquivalentTo(targetCRS.get()));
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=axisswap +order=2,1");
    auto convInverse = nn_dynamic_pointer_cast<Conversion>(conv->inverse());
    ASSERT_TRUE(convInverse != nullptr);
    EXPECT_TRUE(convInverse->sourceCRS() &&
                convInverse->sourceCRS()->isEquivalentTo(targetCRS.get()));
    EXPECT_TRUE(convInverse->targetCRS() &&
                convInverse->targetCRS()->isEquivalentTo(sourceCRS.get()));
    EXPECT_EQ(conv->method()->exportToWKT(WKTFormatter::create().get()),
              convInverse->method()->exportToWKT(WKTFormatter::create().get()));
    EXPECT_TRUE(conv->method()->isEquivalentTo(convInverse->method().get()));
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_longlat_to_geogCS_latlong_database) {

    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), std::string());
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        AuthorityFactory::create(DatabaseContext::create(), "OGC")
            ->createCoordinateReferenceSystem("CRS84"),
        AuthorityFactory::create(DatabaseContext::create(), "EPSG")
            ->createCoordinateReferenceSystem("4326"),
        ctxt);
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_longlat_to_projCRS) {

    auto op = CoordinateOperationFactory::create()->createOperation(
        GeographicCRS::OGC_CRS84, createUTM31_WGS84());
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=unitconvert +xy_in=deg +xy_out=rad "
              "+step +proj=utm +zone=31 +ellps=WGS84");
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_different_from_baseCRS_to_projCRS) {

    auto op = CoordinateOperationFactory::create()->createOperation(
        GeographicCRS::EPSG_4807, createUTM31_WGS84());
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(
        op->exportToPROJString(PROJStringFormatter::create().get()),
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=grad +xy_out=rad +step +inv +proj=longlat "
        "+ellps=clrk80ign +pm=paris +step +proj=utm +zone=31 "
        "+ellps=WGS84");
}

// ---------------------------------------------------------------------------

TEST(operation,
     geogCRS_different_from_baseCRS_to_projCRS_context_compatible_area) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        authFactory->createCoordinateReferenceSystem("4807"),  // NTF(Paris)
        authFactory->createCoordinateReferenceSystem("32631"), // UTM31 WGS84
        ctxt);
    ASSERT_EQ(list.size(), 4);
    EXPECT_EQ(
        list[0]->nameStr(),
        "NTF (Paris) to NTF (1) + Inverse of WGS 84 to NTF (3) + UTM zone 31N");
    ASSERT_EQ(list[0]->coordinateOperationAccuracies().size(), 1);
    EXPECT_EQ(list[0]->coordinateOperationAccuracies()[0]->value(), "1");
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=grad +xy_out=rad +step +inv "
              "+proj=longlat +ellps=clrk80ign +pm=paris +step +proj=hgridshift "
              "+grids=ntf_r93.gsb +step +proj=utm +zone=31 +ellps=WGS84");
}

// ---------------------------------------------------------------------------

TEST(operation, geocentricCRS_to_projCRS) {

    auto op = CoordinateOperationFactory::create()->createOperation(
        createGeocentricDatumWGS84(), createUTM31_WGS84());
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +inv +proj=cart +ellps=WGS84 +step "
              "+proj=utm +zone=31 +ellps=WGS84");
}

// ---------------------------------------------------------------------------

TEST(operation, projCRS_to_geogCRS) {

    auto op = CoordinateOperationFactory::create()->createOperation(
        createUTM31_WGS84(), GeographicCRS::EPSG_4326);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +inv +proj=utm +zone=31 +ellps=WGS84 +step "
              "+proj=unitconvert +xy_in=rad +xy_out=deg +step +proj=axisswap "
              "+order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, projCRS_to_projCRS) {

    auto op = CoordinateOperationFactory::create()->createOperation(
        createUTM31_WGS84(), createUTM32_WGS84());
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +inv +proj=utm +zone=31 +ellps=WGS84 +step "
              "+proj=utm +zone=32 +ellps=WGS84");
}

// ---------------------------------------------------------------------------

TEST(operation, projCRS_to_projCRS_different_baseCRS) {

    auto utm32 = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4807,
        Conversion::createUTM(PropertyMap(), 32, true),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));

    auto op = CoordinateOperationFactory::create()->createOperation(
        createUTM31_WGS84(), utm32);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +inv +proj=utm +zone=31 +ellps=WGS84 +step "
              "+proj=utm +zone=32 +ellps=clrk80ign +pm=paris");
}

// ---------------------------------------------------------------------------

TEST(operation, projCRS_to_projCRS_context_compatible_area) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        authFactory->createCoordinateReferenceSystem("32634"), // UTM 34
        authFactory->createCoordinateReferenceSystem(
            "2171"), // Pulkovo 42 Poland I
        ctxt);
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list[0]->nameStr(),
              "Inverse of UTM zone 34N + Inverse of Pulkovo 1942(58) to WGS 84 "
              "(1) + Poland zone I");
    ASSERT_EQ(list[0]->coordinateOperationAccuracies().size(), 1);
    EXPECT_EQ(list[0]->coordinateOperationAccuracies()[0]->value(), "1");
}

// ---------------------------------------------------------------------------

TEST(operation, projCRS_to_projCRS_context_compatible_area_bis) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        authFactory->createCoordinateReferenceSystem(
            "3844"), // Pulkovo 42 Stereo 70 (Romania)
        authFactory->createCoordinateReferenceSystem("32634"), // UTM 34
        ctxt);
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list[0]->nameStr(), "Inverse of Stereo 70 + "
                                  "Pulkovo 1942(58) to WGS 84 "
                                  "(19) + UTM zone 34N");
    ASSERT_EQ(list[0]->coordinateOperationAccuracies().size(), 1);
    EXPECT_EQ(list[0]->coordinateOperationAccuracies()[0]->value(), "3");
}

// ---------------------------------------------------------------------------

TEST(operation, projCRS_to_projCRS_context_one_incompatible_area) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        authFactory->createCoordinateReferenceSystem("32631"), // UTM 31
        authFactory->createCoordinateReferenceSystem(
            "2171"), // Pulkovo 42 Poland I
        ctxt);
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list[0]->nameStr(),
              "Inverse of UTM zone 31N + Inverse of Pulkovo 1942(58) to WGS 84 "
              "(1) + Poland zone I");
    ASSERT_EQ(list[0]->coordinateOperationAccuracies().size(), 1);
    EXPECT_EQ(list[0]->coordinateOperationAccuracies()[0]->value(), "1");
}

// ---------------------------------------------------------------------------

TEST(operation, projCRS_to_projCRS_context_incompatible_areas) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), "EPSG");
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        authFactory->createCoordinateReferenceSystem("32631"), // UTM 31
        authFactory->createCoordinateReferenceSystem("32633"), // UTM 33
        ctxt);
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list[0]->nameStr(), "Inverse of UTM zone 31N + UTM zone 33N");
    ASSERT_EQ(list[0]->coordinateOperationAccuracies().size(), 1);
    EXPECT_EQ(list[0]->coordinateOperationAccuracies()[0]->value(), "0");
}

// ---------------------------------------------------------------------------

TEST(operation, projCRS_to_projCRS_north_pole_inverted_axis) {

    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), std::string());
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        AuthorityFactory::create(DatabaseContext::create(), "EPSG")
            ->createCoordinateReferenceSystem("32661"),
        AuthorityFactory::create(DatabaseContext::create(), "EPSG")
            ->createCoordinateReferenceSystem("5041"),
        ctxt);
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, projCRS_to_projCRS_south_pole_inverted_axis) {

    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), std::string());
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        AuthorityFactory::create(DatabaseContext::create(), "EPSG")
            ->createCoordinateReferenceSystem("32761"),
        AuthorityFactory::create(DatabaseContext::create(), "EPSG")
            ->createCoordinateReferenceSystem("5042"),
        ctxt);
    ASSERT_EQ(list.size(), 1);
    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, boundCRS_of_geogCRS_to_geogCRS) {
    auto boundCRS = BoundCRS::createFromTOWGS84(
        GeographicCRS::EPSG_4807, std::vector<double>{1, 2, 3, 4, 5, 6, 7});
    auto op = CoordinateOperationFactory::create()->createOperation(
        boundCRS, GeographicCRS::EPSG_4326);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(
        op->exportToPROJString(PROJStringFormatter::create().get()),
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=grad +xy_out=rad +step +inv +proj=longlat "
        "+ellps=clrk80ign +pm=paris +step +proj=cart +ellps=clrk80ign "
        "+step +proj=helmert +x=1 +y=2 +z=3 +rx=4 +ry=5 +rz=6 +s=7 "
        "+convention=position_vector +step "
        "+inv +proj=cart +ellps=WGS84 +step +proj=unitconvert +xy_in=rad "
        "+xy_out=deg +step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, boundCRS_of_geogCRS_to_unrelated_geogCRS) {
    auto boundCRS = BoundCRS::createFromTOWGS84(
        GeographicCRS::EPSG_4807, std::vector<double>{1, 2, 3, 4, 5, 6, 7});
    auto op = CoordinateOperationFactory::create()->createOperation(
        boundCRS, GeographicCRS::EPSG_4269);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              CoordinateOperationFactory::create()
                  ->createOperation(GeographicCRS::EPSG_4807,
                                    GeographicCRS::EPSG_4269)
                  ->exportToPROJString(PROJStringFormatter::create().get()));
}

// ---------------------------------------------------------------------------

TEST(operation, boundCRS_of_projCRS_to_geogCRS) {
    auto utm31 = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4807,
        Conversion::createUTM(PropertyMap(), 31, true),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    auto boundCRS = BoundCRS::createFromTOWGS84(
        utm31, std::vector<double>{1, 2, 3, 4, 5, 6, 7});
    auto op = CoordinateOperationFactory::create()->createOperation(
        boundCRS, GeographicCRS::EPSG_4326);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +inv +proj=utm +zone=31 +ellps=clrk80ign "
              "+pm=paris "
              "+step +proj=cart +ellps=clrk80ign +step +proj=helmert +x=1 +y=2 "
              "+z=3 +rx=4 +ry=5 +rz=6 +s=7 +convention=position_vector +step "
              "+inv +proj=cart +ellps=WGS84 "
              "+step +proj=unitconvert +xy_in=rad +xy_out=deg +step "
              "+proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, boundCRS_of_geogCRS_to_projCRS) {
    auto boundCRS = BoundCRS::createFromTOWGS84(
        GeographicCRS::EPSG_4807, std::vector<double>{1, 2, 3, 4, 5, 6, 7});
    auto utm31 = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4326,
        Conversion::createUTM(PropertyMap(), 31, true),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    auto op =
        CoordinateOperationFactory::create()->createOperation(boundCRS, utm31);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(
        op->exportToPROJString(PROJStringFormatter::create().get()),
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=grad +xy_out=rad +step +inv +proj=longlat "
        "+ellps=clrk80ign +pm=paris +step +proj=cart +ellps=clrk80ign "
        "+step +proj=helmert +x=1 +y=2 +z=3 +rx=4 +ry=5 +rz=6 +s=7 "
        "+convention=position_vector +step "
        "+inv +proj=cart +ellps=WGS84 +step +proj=utm +zone=31 "
        "+ellps=WGS84");
}

// ---------------------------------------------------------------------------

TEST(operation, geogCRS_to_boundCRS_of_geogCRS) {
    auto boundCRS = BoundCRS::createFromTOWGS84(
        GeographicCRS::EPSG_4807, std::vector<double>{1, 2, 3, 4, 5, 6, 7});
    auto op = CoordinateOperationFactory::create()->createOperation(
        GeographicCRS::EPSG_4326, boundCRS);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=cart "
              "+ellps=WGS84 +step +inv +proj=helmert +x=1 +y=2 +z=3 +rx=4 "
              "+ry=5 +rz=6 +s=7 +convention=position_vector +step +inv "
              "+proj=cart +ellps=clrk80ign +step +proj=longlat "
              "+ellps=clrk80ign +pm=paris +step +proj=unitconvert +xy_in=rad "
              "+xy_out=grad +step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, boundCRS_to_boundCRS) {
    auto utm31 = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4807,
        Conversion::createUTM(PropertyMap(), 31, true),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    auto utm32 = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4269,
        Conversion::createUTM(PropertyMap(), 32, true),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    auto boundCRS1 = BoundCRS::createFromTOWGS84(
        utm31, std::vector<double>{1, 2, 3, 4, 5, 6, 7});
    auto boundCRS2 = BoundCRS::createFromTOWGS84(
        utm32, std::vector<double>{8, 9, 10, 11, 12, 13, 14});
    auto op = CoordinateOperationFactory::create()->createOperation(boundCRS1,
                                                                    boundCRS2);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +inv +proj=utm +zone=31 +ellps=clrk80ign "
              "+pm=paris "
              "+step +proj=cart +ellps=clrk80ign +step +proj=helmert +x=1 +y=2 "
              "+z=3 +rx=4 +ry=5 +rz=6 +s=7 +convention=position_vector +step "
              "+inv +proj=helmert +x=8 +y=9 +z=10 +rx=11 +ry=12 +rz=13 +s=14 "
              "+convention=position_vector +step +inv +proj=cart +ellps=GRS80 "
              "+step +proj=utm +zone=32 +ellps=GRS80");
}

// ---------------------------------------------------------------------------

TEST(operation, boundCRS_to_boundCRS_noop_for_TOWGS84) {
    auto boundCRS1 = BoundCRS::createFromTOWGS84(
        GeographicCRS::EPSG_4807, std::vector<double>{1, 2, 3, 4, 5, 6, 7});
    auto boundCRS2 = BoundCRS::createFromTOWGS84(
        GeographicCRS::EPSG_4269, std::vector<double>{1, 2, 3, 4, 5, 6, 7});
    auto op = CoordinateOperationFactory::create()->createOperation(boundCRS1,
                                                                    boundCRS2);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(
        op->exportToPROJString(PROJStringFormatter::create().get()),
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=grad +xy_out=rad +step +inv +proj=longlat "
        "+ellps=clrk80ign +pm=paris +step +proj=cart +ellps=clrk80ign "
        "+step +inv +proj=cart +ellps=GRS80 +step +proj=unitconvert "
        "+xy_in=rad +xy_out=deg +step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, boundCRS_to_boundCRS_unralated_hub) {
    auto boundCRS1 = BoundCRS::createFromTOWGS84(
        GeographicCRS::EPSG_4807, std::vector<double>{1, 2, 3, 4, 5, 6, 7});
    auto boundCRS2 = BoundCRS::create(
        GeographicCRS::EPSG_4269, GeographicCRS::EPSG_4979,
        Transformation::createGeocentricTranslations(
            PropertyMap(), GeographicCRS::EPSG_4269, GeographicCRS::EPSG_4979,
            1.0, 2.0, 3.0, std::vector<PositionalAccuracyNNPtr>()));
    auto op = CoordinateOperationFactory::create()->createOperation(boundCRS1,
                                                                    boundCRS2);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              CoordinateOperationFactory::create()
                  ->createOperation(boundCRS1->baseCRS(), boundCRS2->baseCRS())
                  ->exportToPROJString(PROJStringFormatter::create().get()));
}

// ---------------------------------------------------------------------------

static VerticalCRSNNPtr createVerticalCRS() {
    PropertyMap propertiesVDatum;
    propertiesVDatum.set(Identifier::CODESPACE_KEY, "EPSG")
        .set(Identifier::CODE_KEY, 5101)
        .set(IdentifiedObject::NAME_KEY, "Ordnance Datum Newlyn");
    auto vdatum = VerticalReferenceFrame::create(propertiesVDatum);
    PropertyMap propertiesCRS;
    propertiesCRS.set(Identifier::CODESPACE_KEY, "EPSG")
        .set(Identifier::CODE_KEY, 5701)
        .set(IdentifiedObject::NAME_KEY, "ODN height");
    return VerticalCRS::create(
        propertiesCRS, vdatum,
        VerticalCS::createGravityRelatedHeight(UnitOfMeasure::METRE));
}

// ---------------------------------------------------------------------------

TEST(operation, compoundCRS_to_geogCRS) {

    auto compound = CompoundCRS::create(
        PropertyMap(),
        std::vector<CRSNNPtr>{GeographicCRS::EPSG_4326, createVerticalCRS()});
    auto op = CoordinateOperationFactory::create()->createOperation(
        compound, GeographicCRS::EPSG_4807);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              CoordinateOperationFactory::create()
                  ->createOperation(GeographicCRS::EPSG_4326,
                                    GeographicCRS::EPSG_4807)
                  ->exportToPROJString(PROJStringFormatter::create().get()));
}

// ---------------------------------------------------------------------------

static BoundCRSNNPtr createBoundVerticalCRS() {
    auto vertCRS = createVerticalCRS();
    auto transformation =
        Transformation::createGravityRelatedHeightToGeographic3D(
            PropertyMap(), vertCRS, GeographicCRS::EPSG_4979, "egm08_25.gtx",
            std::vector<PositionalAccuracyNNPtr>());
    return BoundCRS::create(vertCRS, GeographicCRS::EPSG_4979, transformation);
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_height_to_PROJ_string) {
    auto transf = createBoundVerticalCRS()->transformation();
    EXPECT_EQ(transf->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=vgridshift +grids=egm08_25.gtx");

    auto grids = transf->gridsNeeded(DatabaseContext::create());
    ASSERT_EQ(grids.size(), 1);
    auto gridDesc = *(grids.begin());
    EXPECT_EQ(gridDesc.shortName, "egm08_25.gtx");
    EXPECT_EQ(gridDesc.packageName, "proj-datumgrid-world");
    EXPECT_TRUE(gridDesc.url.find(
                    "https://download.osgeo.org/proj/proj-datumgrid-world-") ==
                0)
        << gridDesc.url;
    if (gridDesc.available) {
        EXPECT_TRUE(!gridDesc.fullName.empty()) << gridDesc.fullName;
        EXPECT_TRUE(gridDesc.fullName.find(gridDesc.shortName) !=
                    std::string::npos)
            << gridDesc.fullName;
    } else {
        EXPECT_TRUE(gridDesc.fullName.empty()) << gridDesc.fullName;
    }
    EXPECT_EQ(gridDesc.directDownload, true);
    EXPECT_EQ(gridDesc.openLicense, true);
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_ntv2_to_PROJ_string) {
    auto transformation = Transformation::createNTv2(
        PropertyMap(), GeographicCRS::EPSG_4807, GeographicCRS::EPSG_4326,
        "foo.gsb", std::vector<PositionalAccuracyNNPtr>());
    EXPECT_EQ(
        transformation->exportToPROJString(PROJStringFormatter::create().get()),
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=grad +xy_out=rad +step "
        "+proj=hgridshift +grids=foo.gsb +step +proj=unitconvert "
        "+xy_in=rad +xy_out=deg +step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_VERTCON_to_PROJ_string) {
    auto verticalCRS1 = createVerticalCRS();

    auto verticalCRS2 = VerticalCRS::create(
        PropertyMap(), VerticalReferenceFrame::create(PropertyMap()),
        VerticalCS::createGravityRelatedHeight(UnitOfMeasure::METRE));

    // Use of this type of transformation is a bit of non-sense here
    // since it should normally be used with NGVD29 and NAVD88 for VerticalCRS,
    // and NAD27/NAD83 as horizontal CRS...
    auto vtransformation = Transformation::createVERTCON(
        PropertyMap(), verticalCRS1, verticalCRS2, "bla.gtx",
        std::vector<PositionalAccuracyNNPtr>());
    EXPECT_EQ(vtransformation->exportToPROJString(
                  PROJStringFormatter::create().get()),
              "+proj=vgridshift +grids=bla.gtx +multiplier=0.001");
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_longitude_rotation_to_PROJ_string) {

    auto src = GeographicCRS::create(
        PropertyMap(), GeodeticReferenceFrame::create(
                           PropertyMap(), Ellipsoid::WGS84,
                           optional<std::string>(), PrimeMeridian::GREENWICH),
        EllipsoidalCS::createLatitudeLongitude(UnitOfMeasure::DEGREE));
    auto dest = GeographicCRS::create(
        PropertyMap(), GeodeticReferenceFrame::create(
                           PropertyMap(), Ellipsoid::WGS84,
                           optional<std::string>(), PrimeMeridian::PARIS),
        EllipsoidalCS::createLatitudeLongitude(UnitOfMeasure::DEGREE));
    auto transformation = Transformation::createLongitudeRotation(
        PropertyMap(), src, dest, Angle(10));
    EXPECT_EQ(
        transformation->exportToPROJString(PROJStringFormatter::create().get()),
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=deg +xy_out=rad +step +inv "
        "+proj=longlat +ellps=WGS84 +pm=10 +step +proj=unitconvert "
        "+xy_in=rad +xy_out=deg +step +proj=axisswap +order=2,1");
    EXPECT_EQ(transformation->inverse()->exportToPROJString(
                  PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +inv "
              "+proj=longlat +ellps=WGS84 +pm=-10 +step +proj=unitconvert "
              "+xy_in=rad +xy_out=deg +step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_Geographic2D_offsets_to_PROJ_string) {

    auto transformation = Transformation::createGeographic2DOffsets(
        PropertyMap(), GeographicCRS::EPSG_4326, GeographicCRS::EPSG_4326,
        Angle(0.5), Angle(-1), {});
    EXPECT_EQ(
        transformation->exportToPROJString(PROJStringFormatter::create().get()),
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=geogoffset "
        "+dlat=1800 +dlon=-3600 +step +proj=unitconvert +xy_in=rad "
        "+xy_out=deg +step +proj=axisswap +order=2,1");
    EXPECT_EQ(transformation->inverse()->exportToPROJString(
                  PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=geogoffset "
              "+dlat=-1800 +dlon=3600 +step +proj=unitconvert +xy_in=rad "
              "+xy_out=deg +step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_Geographic3D_offsets_to_PROJ_string) {

    auto transformation = Transformation::createGeographic3DOffsets(
        PropertyMap(), GeographicCRS::EPSG_4326, GeographicCRS::EPSG_4326,
        Angle(0.5), Angle(-1), Length(2), {});
    EXPECT_EQ(
        transformation->exportToPROJString(PROJStringFormatter::create().get()),
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=geogoffset "
        "+dlat=1800 +dlon=-3600 +dh=2 +step +proj=unitconvert +xy_in=rad "
        "+xy_out=deg +step +proj=axisswap +order=2,1");
    EXPECT_EQ(transformation->inverse()->exportToPROJString(
                  PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=geogoffset "
              "+dlat=-1800 +dlon=3600 +dh=-2 +step +proj=unitconvert "
              "+xy_in=rad +xy_out=deg +step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation,
     transformation_Geographic2D_with_height_offsets_to_PROJ_string) {

    auto transformation = Transformation::createGeographic2DWithHeightOffsets(
        PropertyMap(),
        CompoundCRS::create(PropertyMap(),
                            {GeographicCRS::EPSG_4326, createVerticalCRS()}),
        GeographicCRS::EPSG_4326, Angle(0.5), Angle(-1), Length(2), {});
    EXPECT_EQ(
        transformation->exportToPROJString(PROJStringFormatter::create().get()),
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=geogoffset "
        "+dlat=1800 +dlon=-3600 +dh=2 +step +proj=unitconvert +xy_in=rad "
        "+xy_out=deg +step +proj=axisswap +order=2,1");
    EXPECT_EQ(transformation->inverse()->exportToPROJString(
                  PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=geogoffset "
              "+dlat=-1800 +dlon=3600 +dh=-2 +step +proj=unitconvert "
              "+xy_in=rad +xy_out=deg +step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, transformation_vertical_offset_to_PROJ_string) {

    auto transformation = Transformation::createVerticalOffset(
        PropertyMap(), createVerticalCRS(), createVerticalCRS(), Length(1), {});
    EXPECT_EQ(
        transformation->exportToPROJString(PROJStringFormatter::create().get()),
        "+proj=geogoffset +dh=1");
    EXPECT_EQ(transformation->inverse()->exportToPROJString(
                  PROJStringFormatter::create().get()),
              "+proj=geogoffset +dh=-1");
}

// ---------------------------------------------------------------------------

TEST(operation, compoundCRS_with_boundVerticalCRS_to_geogCRS) {

    auto compound = CompoundCRS::create(
        PropertyMap(), std::vector<CRSNNPtr>{GeographicCRS::EPSG_4326,
                                             createBoundVerticalCRS()});
    auto op = CoordinateOperationFactory::create()->createOperation(
        compound, GeographicCRS::EPSG_4979);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=vgridshift "
              "+grids=egm08_25.gtx +step +proj=unitconvert +xy_in=rad "
              "+xy_out=deg +step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, compoundCRS_with_boundGeogCRS_to_geogCRS) {

    auto geogCRS = GeographicCRS::create(
        PropertyMap(), GeodeticReferenceFrame::create(
                           PropertyMap(), Ellipsoid::WGS84,
                           optional<std::string>(), PrimeMeridian::GREENWICH),
        EllipsoidalCS::createLatitudeLongitude(UnitOfMeasure::DEGREE));
    auto horizBoundCRS = BoundCRS::createFromTOWGS84(
        geogCRS, std::vector<double>{1, 2, 3, 4, 5, 6, 7});
    auto compound = CompoundCRS::create(
        PropertyMap(),
        std::vector<CRSNNPtr>{horizBoundCRS, createVerticalCRS()});
    auto op = CoordinateOperationFactory::create()->createOperation(
        compound, GeographicCRS::EPSG_4979);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=cart "
              "+ellps=WGS84 +step +proj=helmert +x=1 +y=2 +z=3 +rx=4 +ry=5 "
              "+rz=6 +s=7 +convention=position_vector +step +inv +proj=cart "
              "+ellps=WGS84 +step +proj=unitconvert +xy_in=rad +xy_out=deg "
              "+step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(operation, compoundCRS_with_boundGeogCRS_and_boundVerticalCRS_to_geogCRS) {

    auto horizBoundCRS = BoundCRS::createFromTOWGS84(
        GeographicCRS::EPSG_4807, std::vector<double>{1, 2, 3, 4, 5, 6, 7});
    auto compound = CompoundCRS::create(
        PropertyMap(),
        std::vector<CRSNNPtr>{horizBoundCRS, createBoundVerticalCRS()});
    auto op = CoordinateOperationFactory::create()->createOperation(
        compound, GeographicCRS::EPSG_4979);
    ASSERT_TRUE(op != nullptr);
    // Not completely sure the order of horizontal and vertical operations
    // makes sense
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=grad +xy_out=rad +step +inv "
              "+proj=longlat +ellps=clrk80ign +pm=paris +step +proj=cart "
              "+ellps=clrk80ign +step +proj=helmert +x=1 +y=2 +z=3 +rx=4 +ry=5 "
              "+rz=6 +s=7 +convention=position_vector +step +inv +proj=cart "
              "+ellps=WGS84 +step +proj=vgridshift +grids=egm08_25.gtx +step "
              "+proj=unitconvert +xy_in=rad +xy_out=deg +step "
              "+proj=axisswap +order=2,1");

    auto grids = op->gridsNeeded(DatabaseContext::create());
    EXPECT_EQ(grids.size(), 1);

    auto opInverse = CoordinateOperationFactory::create()->createOperation(
        GeographicCRS::EPSG_4979, compound);
    ASSERT_TRUE(opInverse != nullptr);
    EXPECT_TRUE(opInverse->inverse()->isEquivalentTo(op.get()));
}

// ---------------------------------------------------------------------------

TEST(operation, compoundCRS_with_boundProjCRS_and_boundVerticalCRS_to_geogCRS) {

    auto horizBoundCRS = BoundCRS::createFromTOWGS84(
        ProjectedCRS::create(
            PropertyMap(), GeographicCRS::EPSG_4807,
            Conversion::createUTM(PropertyMap(), 31, true),
            CartesianCS::createEastingNorthing(UnitOfMeasure::METRE)),
        std::vector<double>{1, 2, 3, 4, 5, 6, 7});
    auto compound = CompoundCRS::create(
        PropertyMap(),
        std::vector<CRSNNPtr>{horizBoundCRS, createBoundVerticalCRS()});
    auto op = CoordinateOperationFactory::create()->createOperation(
        compound, GeographicCRS::EPSG_4979);
    ASSERT_TRUE(op != nullptr);
    // Not completely sure the order of horizontal and vertical operations
    // makes sense
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +inv +proj=utm +zone=31 +ellps=clrk80ign "
              "+pm=paris +step +proj=cart +ellps=clrk80ign +step +proj=helmert "
              "+x=1 +y=2 +z=3 +rx=4 +ry=5 +rz=6 +s=7 "
              "+convention=position_vector +step +inv +proj=cart +ellps=WGS84 "
              "+step +proj=vgridshift +grids=egm08_25.gtx +step "
              "+proj=unitconvert +xy_in=rad +xy_out=deg +step "
              "+proj=axisswap +order=2,1");

    auto opInverse = CoordinateOperationFactory::create()->createOperation(
        GeographicCRS::EPSG_4979, compound);
    ASSERT_TRUE(opInverse != nullptr);
    EXPECT_TRUE(opInverse->inverse()->isEquivalentTo(op.get()));
}

// ---------------------------------------------------------------------------

TEST(operation, compoundCRS_to_compoundCRS) {
    auto compound1 = CompoundCRS::create(
        PropertyMap(),
        std::vector<CRSNNPtr>{createUTM31_WGS84(), createVerticalCRS()});
    auto compound2 = CompoundCRS::create(
        PropertyMap(),
        std::vector<CRSNNPtr>{createUTM32_WGS84(), createVerticalCRS()});
    auto op = CoordinateOperationFactory::create()->createOperation(compound1,
                                                                    compound2);
    ASSERT_TRUE(op != nullptr);
    auto opRef = CoordinateOperationFactory::create()->createOperation(
        createUTM31_WGS84(), createUTM32_WGS84());
    ASSERT_TRUE(opRef != nullptr);
    EXPECT_TRUE(op->isEquivalentTo(opRef.get()));
}

// ---------------------------------------------------------------------------

TEST(operation, compoundCRS_to_compoundCRS_with_vertical_transform) {
    auto verticalCRS1 = createVerticalCRS();

    auto verticalCRS2 = VerticalCRS::create(
        PropertyMap(), VerticalReferenceFrame::create(PropertyMap()),
        VerticalCS::createGravityRelatedHeight(UnitOfMeasure::METRE));

    // Use of this type of transformation is a bit of non-sense here
    // since it should normally be used with NGVD29 and NAVD88 for VerticalCRS,
    // and NAD27/NAD83 as horizontal CRS...
    auto vtransformation = Transformation::createVERTCON(
        PropertyMap(), verticalCRS1, verticalCRS2, "bla.gtx",
        std::vector<PositionalAccuracyNNPtr>());

    auto compound1 = CompoundCRS::create(
        PropertyMap(),
        std::vector<CRSNNPtr>{
            ProjectedCRS::create(
                PropertyMap(), GeographicCRS::EPSG_4326,
                Conversion::createTransverseMercator(PropertyMap(), Angle(1),
                                                     Angle(2), Scale(3),
                                                     Length(4), Length(5)),
                CartesianCS::createEastingNorthing(UnitOfMeasure::METRE)),
            BoundCRS::create(verticalCRS1, verticalCRS2, vtransformation)});
    auto compound2 = CompoundCRS::create(
        PropertyMap(),
        std::vector<CRSNNPtr>{createUTM32_WGS84(), verticalCRS2});

    auto op = CoordinateOperationFactory::create()->createOperation(compound1,
                                                                    compound2);
    ASSERT_TRUE(op != nullptr);
    EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +inv +proj=tmerc +lat_0=1 +lon_0=2 +k_0=3 "
              "+x_0=4 +y_0=5 +ellps=WGS84 +step "
              "+proj=vgridshift +grids=bla.gtx +multiplier=0.001 +step "
              "+proj=utm +zone=32 "
              "+ellps=WGS84");
    {
        auto formatter = PROJStringFormatter::create();
        formatter->setUseETMercForTMerc(true);
        EXPECT_EQ(op->exportToPROJString(formatter.get()),
                  "+proj=pipeline +step +inv +proj=etmerc +lat_0=1 +lon_0=2 "
                  "+k_0=3 +x_0=4 +y_0=5 +ellps=WGS84 +step "
                  "+proj=vgridshift +grids=bla.gtx +multiplier=0.001 +step "
                  "+proj=utm +zone=32 "
                  "+ellps=WGS84");
    }
    {
        auto formatter = PROJStringFormatter::create();
        formatter->setUseETMercForTMerc(true);
        EXPECT_EQ(op->inverse()->exportToPROJString(formatter.get()),
                  "+proj=pipeline +step +inv +proj=utm +zone=32 +ellps=WGS84 "
                  "+step +inv +proj=vgridshift +grids=bla.gtx "
                  "+multiplier=0.001 +step +proj=etmerc +lat_0=1 +lon_0=2 "
                  "+k_0=3 +x_0=4 +y_0=5 +ellps=WGS84");
    }

    auto opInverse = CoordinateOperationFactory::create()->createOperation(
        compound2, compound1);
    ASSERT_TRUE(opInverse != nullptr);
    {
        auto formatter = PROJStringFormatter::create();
        auto formatter2 = PROJStringFormatter::create();
        EXPECT_EQ(opInverse->inverse()->exportToPROJString(formatter.get()),
                  op->exportToPROJString(formatter2.get()));
    }
}

// ---------------------------------------------------------------------------

TEST(operation, vertCRS_to_vertCRS) {

    auto vertcrs_m_obj = PROJStringParser().createFromPROJString("+vunits=m");
    auto vertcrs_m = nn_dynamic_pointer_cast<VerticalCRS>(vertcrs_m_obj);
    ASSERT_TRUE(vertcrs_m != nullptr);

    auto vertcrs_ft_obj = PROJStringParser().createFromPROJString("+vunits=ft");
    auto vertcrs_ft = nn_dynamic_pointer_cast<VerticalCRS>(vertcrs_ft_obj);
    ASSERT_TRUE(vertcrs_ft != nullptr);

    auto vertcrs_us_ft_obj =
        PROJStringParser().createFromPROJString("+vunits=us-ft");
    auto vertcrs_us_ft =
        nn_dynamic_pointer_cast<VerticalCRS>(vertcrs_us_ft_obj);
    ASSERT_TRUE(vertcrs_us_ft != nullptr);

    {
        auto op = CoordinateOperationFactory::create()->createOperation(
            NN_CHECK_ASSERT(vertcrs_m), NN_CHECK_ASSERT(vertcrs_ft));
        ASSERT_TRUE(op != nullptr);
        EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
                  "+proj=unitconvert +z_in=m +z_out=ft");
    }
    {
        auto op = CoordinateOperationFactory::create()->createOperation(
            NN_CHECK_ASSERT(vertcrs_m), NN_CHECK_ASSERT(vertcrs_ft));
        ASSERT_TRUE(op != nullptr);
        EXPECT_EQ(op->inverse()->exportToPROJString(
                      PROJStringFormatter::create().get()),
                  "+proj=unitconvert +z_in=ft +z_out=m");
    }
    {
        auto op = CoordinateOperationFactory::create()->createOperation(
            NN_CHECK_ASSERT(vertcrs_ft), NN_CHECK_ASSERT(vertcrs_m));
        ASSERT_TRUE(op != nullptr);
        EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
                  "+proj=unitconvert +z_in=ft +z_out=m");
    }
    {
        auto op = CoordinateOperationFactory::create()->createOperation(
            NN_CHECK_ASSERT(vertcrs_ft), NN_CHECK_ASSERT(vertcrs_us_ft));
        ASSERT_TRUE(op != nullptr);
        EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
                  "+proj=affine +s33=0.999998");
    }
}

// ---------------------------------------------------------------------------

TEST(operation, compoundCRS_to_geogCRS_3D) {

    auto compoundcrs_ft_obj =
        PROJStringParser().createFromPROJString("+proj=merc +vunits=ft");
    auto compoundcrs_ft = nn_dynamic_pointer_cast<CRS>(compoundcrs_ft_obj);
    ASSERT_TRUE(compoundcrs_ft != nullptr);

    auto geogcrs_m_obj =
        PROJStringParser().createFromPROJString("+proj=longlat +vunits=m");
    auto geogcrs_m = nn_dynamic_pointer_cast<CRS>(geogcrs_m_obj);
    ASSERT_TRUE(geogcrs_m != nullptr);

    {
        auto op = CoordinateOperationFactory::create()->createOperation(
            NN_CHECK_ASSERT(compoundcrs_ft), NN_CHECK_ASSERT(geogcrs_m));
        ASSERT_TRUE(op != nullptr);
        EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
                  "+proj=pipeline +step +inv +proj=merc +lon_0=0 +k=1 +x_0=0 "
                  "+y_0=0 +ellps=WGS84 +step +proj=unitconvert +xy_in=rad "
                  "+z_in=ft +xy_out=deg +z_out=m");
    }

    {
        auto op = CoordinateOperationFactory::create()->createOperation(
            NN_CHECK_ASSERT(geogcrs_m), NN_CHECK_ASSERT(compoundcrs_ft));
        ASSERT_TRUE(op != nullptr);
        EXPECT_EQ(op->exportToPROJString(PROJStringFormatter::create().get()),
                  "+proj=pipeline +step +proj=unitconvert +xy_in=deg +z_in=m "
                  "+xy_out=rad +z_out=ft +step +proj=merc +lon_0=0 +k=1 +x_0=0 "
                  "+y_0=0 +ellps=WGS84");
    }
}

// ---------------------------------------------------------------------------

TEST(operation, IGNF_LAMB1_TO_EPSG_4326) {
    auto authFactory =
        AuthorityFactory::create(DatabaseContext::create(), std::string());
    auto ctxt = CoordinateOperationContext::create(authFactory, nullptr, 0.0);
    auto list = CoordinateOperationFactory::create()->createOperations(
        AuthorityFactory::create(DatabaseContext::create(), "IGNF")
            ->createCoordinateReferenceSystem("LAMB1"),
        AuthorityFactory::create(DatabaseContext::create(), "EPSG")
            ->createCoordinateReferenceSystem("4326"),
        ctxt);
    ASSERT_EQ(list.size(), 2);

    EXPECT_EQ(list[0]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +inv +proj=lcc +lat_1=49.5 +lat_0=49.5 "
              "+lon_0=0 +k_0=0.99987734 +x_0=600000 +y_0=200000 "
              "+ellps=clrk80ign +pm=paris +step +proj=hgridshift "
              "+grids=ntf_r93.gsb +step +proj=unitconvert +xy_in=rad "
              "+xy_out=deg +step +proj=axisswap +order=2,1");

    EXPECT_EQ(list[1]->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +inv +proj=lcc +lat_1=49.5 +lat_0=49.5 "
              "+lon_0=0 +k_0=0.99987734 +x_0=600000 +y_0=200000 "
              "+ellps=clrk80ign +pm=paris +step +proj=cart +ellps=clrk80ign "
              "+step +proj=helmert +x=-168 +y=-60 +z=320 +step +inv +proj=cart "
              "+ellps=WGS84 +step +proj=unitconvert +xy_in=rad +xy_out=deg "
              "+step +proj=axisswap +order=2,1");

    auto list2 = CoordinateOperationFactory::create()->createOperations(
        AuthorityFactory::create(DatabaseContext::create(), "EPSG")
            // NTF (Paris) / Lambert Nord France equivalent to IGNF:LAMB1
            ->createCoordinateReferenceSystem("27561"),
        AuthorityFactory::create(DatabaseContext::create(), "EPSG")
            ->createCoordinateReferenceSystem("4326"),
        ctxt);
    ASSERT_GE(list2.size(), 4U);

    EXPECT_EQ(replaceAll(list2[0]->exportToPROJString(
                             PROJStringFormatter::create().get()),
                         "0.999877341", "0.99987734"),
              list[0]->exportToPROJString(PROJStringFormatter::create().get()));

    // The second entry in list2 (list2[1]) uses the
    // weird +pm=2.33720833333333 from "NTF (Paris) to NTF (2)"
    // and the third one uses the ESRI geographic offset method with another
    // value
    // so skip to the 4th method
    EXPECT_EQ(replaceAll(list2[3]->exportToPROJString(
                             PROJStringFormatter::create().get()),
                         "0.999877341", "0.99987734"),
              list[1]->exportToPROJString(PROJStringFormatter::create().get()));
}

// ---------------------------------------------------------------------------

TEST(operation, isPROJInstanciable) {

    {
        auto transformation = Transformation::createGeocentricTranslations(
            PropertyMap(), GeographicCRS::EPSG_4269, GeographicCRS::EPSG_4326,
            1.0, 2.0, 3.0, {});
        EXPECT_TRUE(
            transformation->isPROJInstanciable(DatabaseContext::create()));
    }

    // Missing grid
    {
        auto transformation = Transformation::createNTv2(
            PropertyMap(), GeographicCRS::EPSG_4807, GeographicCRS::EPSG_4326,
            "foo.gsb", std::vector<PositionalAccuracyNNPtr>());
        EXPECT_FALSE(
            transformation->isPROJInstanciable(DatabaseContext::create()));
    }

    // Unsupported method
    {
        auto transformation = Transformation::create(
            PropertyMap(), GeographicCRS::EPSG_4269, GeographicCRS::EPSG_4326,
            nullptr, OperationMethod::create(
                         PropertyMap(), std::vector<OperationParameterNNPtr>{}),
            std::vector<GeneralParameterValueNNPtr>{},
            std::vector<PositionalAccuracyNNPtr>{});
        EXPECT_FALSE(
            transformation->isPROJInstanciable(DatabaseContext::create()));
    }
}

// ---------------------------------------------------------------------------

TEST(operation, createOperation_on_crs_with_canonical_bound_crs) {
    auto boundCRS = BoundCRS::createFromTOWGS84(
        GeographicCRS::EPSG_4267, std::vector<double>{1, 2, 3, 4, 5, 6, 7});
    auto crs = boundCRS->baseCRSWithCanonicalBoundCRS();
    {
        auto op = CoordinateOperationFactory::create()->createOperation(
            crs, GeographicCRS::EPSG_4326);
        ASSERT_TRUE(op != nullptr);
        EXPECT_TRUE(op->isEquivalentTo(boundCRS->transformation().get()));
        {
            auto wkt1 = op->exportToWKT(
                WKTFormatter::create(WKTFormatter::Convention::WKT2_2018)
                    .get());
            auto wkt2 = boundCRS->transformation()->exportToWKT(
                WKTFormatter::create(WKTFormatter::Convention::WKT2_2018)
                    .get());
            EXPECT_EQ(wkt1, wkt2);
        }
    }
    {
        auto op = CoordinateOperationFactory::create()->createOperation(
            GeographicCRS::EPSG_4326, crs);
        ASSERT_TRUE(op != nullptr);
        EXPECT_TRUE(
            op->isEquivalentTo(boundCRS->transformation()->inverse().get()));
        {
            auto wkt1 = op->exportToWKT(
                WKTFormatter::create(WKTFormatter::Convention::WKT2_2018)
                    .get());
            auto wkt2 = boundCRS->transformation()->inverse()->exportToWKT(
                WKTFormatter::create(WKTFormatter::Convention::WKT2_2018)
                    .get());
            EXPECT_EQ(wkt1, wkt2);
        }
    }
}

// ---------------------------------------------------------------------------

TEST(operation, mercator_variant_A_to_variant_B) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4326,
        Conversion::createMercatorVariantA(PropertyMap(), Angle(0), Angle(1),
                                           Scale(0.9), Length(3), Length(4)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));

    auto conv = projCRS->derivingConversion();
    auto sameConv =
        conv->convertToOtherMethod(EPSG_CODE_METHOD_MERCATOR_VARIANT_A);
    ASSERT_TRUE(sameConv);
    EXPECT_TRUE(sameConv->isEquivalentTo(conv.get()));

    auto targetConv =
        conv->convertToOtherMethod(EPSG_CODE_METHOD_MERCATOR_VARIANT_B);
    ASSERT_TRUE(targetConv);

    auto lat_1 = targetConv->parameterValueNumeric(
        EPSG_CODE_PARAMETER_LATITUDE_1ST_STD_PARALLEL, UnitOfMeasure::DEGREE);
    EXPECT_EQ(lat_1, 25.917499691810534) << lat_1;

    EXPECT_EQ(targetConv->parameterValueNumeric(
                  EPSG_CODE_PARAMETER_LONGITUDE_OF_NATURAL_ORIGIN,
                  UnitOfMeasure::DEGREE),
              1);

    EXPECT_EQ(targetConv->parameterValueNumeric(
                  EPSG_CODE_PARAMETER_FALSE_EASTING, UnitOfMeasure::METRE),
              3);

    EXPECT_EQ(targetConv->parameterValueNumeric(
                  EPSG_CODE_PARAMETER_FALSE_NORTHING, UnitOfMeasure::METRE),
              4);

    EXPECT_FALSE(
        conv->isEquivalentTo(targetConv.get(), IComparable::Criterion::STRICT));
    EXPECT_TRUE(conv->isEquivalentTo(targetConv.get(),
                                     IComparable::Criterion::EQUIVALENT));
    EXPECT_TRUE(targetConv->isEquivalentTo(conv.get(),
                                           IComparable::Criterion::EQUIVALENT));
}

// ---------------------------------------------------------------------------

TEST(operation, mercator_variant_A_to_variant_B_scale_1) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4326,
        Conversion::createMercatorVariantA(PropertyMap(), Angle(0), Angle(1),
                                           Scale(1.0), Length(3), Length(4)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));

    auto targetConv = projCRS->derivingConversion()->convertToOtherMethod(
        EPSG_CODE_METHOD_MERCATOR_VARIANT_B);
    ASSERT_TRUE(targetConv);

    auto lat_1 = targetConv->parameterValueNumeric(
        EPSG_CODE_PARAMETER_LATITUDE_1ST_STD_PARALLEL, UnitOfMeasure::DEGREE);
    EXPECT_EQ(lat_1, 0.0) << lat_1;
}

// ---------------------------------------------------------------------------

TEST(operation, mercator_variant_A_to_variant_B_no_crs) {
    auto targetConv =
        Conversion::createMercatorVariantA(PropertyMap(), Angle(0), Angle(1),
                                           Scale(1.0), Length(3), Length(4))
            ->convertToOtherMethod(EPSG_CODE_METHOD_MERCATOR_VARIANT_B);
    EXPECT_FALSE(targetConv != nullptr);
}

// ---------------------------------------------------------------------------

TEST(operation, mercator_variant_A_to_variant_B_invalid_scale) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4326,
        Conversion::createMercatorVariantA(PropertyMap(), Angle(0), Angle(1),
                                           Scale(0.0), Length(3), Length(4)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));

    auto targetConv = projCRS->derivingConversion()->convertToOtherMethod(
        EPSG_CODE_METHOD_MERCATOR_VARIANT_B);
    EXPECT_FALSE(targetConv != nullptr);
}

// ---------------------------------------------------------------------------

static GeographicCRSNNPtr geographicCRSInvalidEccentricity() {
    return GeographicCRS::create(
        PropertyMap(),
        GeodeticReferenceFrame::create(
            PropertyMap(), Ellipsoid::createFlattenedSphere(
                               PropertyMap(), Length(6378137), Scale(0.1)),
            optional<std::string>(), PrimeMeridian::GREENWICH),
        EllipsoidalCS::createLatitudeLongitude(UnitOfMeasure::DEGREE));
}

// ---------------------------------------------------------------------------

TEST(operation, mercator_variant_A_to_variant_B_invalid_eccentricity) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), geographicCRSInvalidEccentricity(),
        Conversion::createMercatorVariantA(PropertyMap(), Angle(0), Angle(1),
                                           Scale(1.0), Length(3), Length(4)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));

    auto targetConv = projCRS->derivingConversion()->convertToOtherMethod(
        EPSG_CODE_METHOD_MERCATOR_VARIANT_B);
    EXPECT_FALSE(targetConv != nullptr);
}

// ---------------------------------------------------------------------------

TEST(operation, mercator_variant_B_to_variant_A) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4326,
        Conversion::createMercatorVariantB(PropertyMap(),
                                           Angle(25.917499691810534), Angle(1),
                                           Length(3), Length(4)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    auto targetConv = projCRS->derivingConversion()->convertToOtherMethod(
        EPSG_CODE_METHOD_MERCATOR_VARIANT_A);
    ASSERT_TRUE(targetConv);

    EXPECT_EQ(targetConv->parameterValueNumeric(
                  EPSG_CODE_PARAMETER_LATITUDE_OF_NATURAL_ORIGIN,
                  UnitOfMeasure::DEGREE),
              0);

    EXPECT_EQ(targetConv->parameterValueNumeric(
                  EPSG_CODE_PARAMETER_LONGITUDE_OF_NATURAL_ORIGIN,
                  UnitOfMeasure::DEGREE),
              1);

    auto k_0 = targetConv->parameterValueNumeric(
        EPSG_CODE_PARAMETER_SCALE_FACTOR_AT_NATURAL_ORIGIN,
        UnitOfMeasure::SCALE_UNITY);
    EXPECT_EQ(k_0, 0.9) << k_0;

    EXPECT_EQ(targetConv->parameterValueNumeric(
                  EPSG_CODE_PARAMETER_FALSE_EASTING, UnitOfMeasure::METRE),
              3);

    EXPECT_EQ(targetConv->parameterValueNumeric(
                  EPSG_CODE_PARAMETER_FALSE_NORTHING, UnitOfMeasure::METRE),
              4);
}

// ---------------------------------------------------------------------------

TEST(operation, mercator_variant_B_to_variant_A_invalid_std1) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4326,
        Conversion::createMercatorVariantB(PropertyMap(), Angle(100), Angle(1),
                                           Length(3), Length(4)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    auto targetConv = projCRS->derivingConversion()->convertToOtherMethod(
        EPSG_CODE_METHOD_MERCATOR_VARIANT_A);
    EXPECT_FALSE(targetConv != nullptr);
}

// ---------------------------------------------------------------------------

TEST(operation, mercator_variant_B_to_variant_A_invalid_eccentricity) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), geographicCRSInvalidEccentricity(),
        Conversion::createMercatorVariantB(PropertyMap(), Angle(0), Angle(1),
                                           Length(3), Length(4)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    auto targetConv = projCRS->derivingConversion()->convertToOtherMethod(
        EPSG_CODE_METHOD_MERCATOR_VARIANT_A);
    EXPECT_FALSE(targetConv != nullptr);
}

// ---------------------------------------------------------------------------

TEST(operation, lcc2sp_to_lcc1sp) {
    // equivalent to EPSG:2154
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4269, // something using GRS80
        Conversion::createLambertConicConformal_2SP(
            PropertyMap(), Angle(46.5), Angle(3), Angle(49), Angle(44),
            Length(700000), Length(6600000)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));

    auto conv = projCRS->derivingConversion();
    auto targetConv = conv->convertToOtherMethod(
        EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_1SP);
    ASSERT_TRUE(targetConv);

    {
        auto lat_0 = targetConv->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LATITUDE_OF_NATURAL_ORIGIN,
            UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lat_0, 46.519430223986866, 1e-12) << lat_0;

        auto lon_0 = targetConv->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LONGITUDE_OF_NATURAL_ORIGIN,
            UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lon_0, 3.0, 1e-15) << lon_0;

        auto k_0 = targetConv->parameterValueNumeric(
            EPSG_CODE_PARAMETER_SCALE_FACTOR_AT_NATURAL_ORIGIN,
            UnitOfMeasure::SCALE_UNITY);
        EXPECT_NEAR(k_0, 0.9990510286374692, 1e-15) << k_0;

        auto x_0 = targetConv->parameterValueNumeric(
            EPSG_CODE_PARAMETER_FALSE_EASTING, UnitOfMeasure::METRE);
        EXPECT_NEAR(x_0, 700000, 1e-15) << x_0;

        auto y_0 = targetConv->parameterValueNumeric(
            EPSG_CODE_PARAMETER_FALSE_NORTHING, UnitOfMeasure::METRE);
        EXPECT_NEAR(y_0, 6602157.8388103368, 1e-7) << y_0;
    }

    auto _2sp_from_1sp = targetConv->convertToOtherMethod(
        EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_2SP);
    ASSERT_TRUE(_2sp_from_1sp);

    {
        auto lat_0 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LATITUDE_FALSE_ORIGIN, UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lat_0, 46.5, 1e-15) << lat_0;

        auto lon_0 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LONGITUDE_FALSE_ORIGIN, UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lon_0, 3, 1e-15) << lon_0;

        auto lat_1 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LATITUDE_1ST_STD_PARALLEL,
            UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lat_1, 49, 1e-15) << lat_1;

        auto lat_2 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LATITUDE_2ND_STD_PARALLEL,
            UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lat_2, 44, 1e-15) << lat_2;

        auto x_0 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_EASTING_FALSE_ORIGIN, UnitOfMeasure::METRE);
        EXPECT_NEAR(x_0, 700000, 1e-15) << x_0;

        auto y_0 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_NORTHING_FALSE_ORIGIN, UnitOfMeasure::METRE);
        EXPECT_NEAR(y_0, 6600000, 1e-15) << y_0;
    }

    EXPECT_FALSE(
        conv->isEquivalentTo(targetConv.get(), IComparable::Criterion::STRICT));
    EXPECT_TRUE(conv->isEquivalentTo(targetConv.get(),
                                     IComparable::Criterion::EQUIVALENT));
    EXPECT_TRUE(targetConv->isEquivalentTo(conv.get(),
                                           IComparable::Criterion::EQUIVALENT));
}

// ---------------------------------------------------------------------------

TEST(operation, lcc2sp_to_lcc1sp_phi0_eq_phi1_eq_phi2) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4269, // something using GRS80
        Conversion::createLambertConicConformal_2SP(
            PropertyMap(), Angle(46.5), Angle(3), Angle(46.5), Angle(46.5),
            Length(700000), Length(6600000)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));

    auto conv = projCRS->derivingConversion();
    auto targetConv = conv->convertToOtherMethod(
        EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_1SP);
    ASSERT_TRUE(targetConv);

    {
        auto lat_0 = targetConv->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LATITUDE_OF_NATURAL_ORIGIN,
            UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lat_0, 46.5, 1e-15) << lat_0;

        auto lon_0 = targetConv->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LONGITUDE_OF_NATURAL_ORIGIN,
            UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lon_0, 3.0, 1e-15) << lon_0;

        auto k_0 = targetConv->parameterValueNumeric(
            EPSG_CODE_PARAMETER_SCALE_FACTOR_AT_NATURAL_ORIGIN,
            UnitOfMeasure::SCALE_UNITY);
        EXPECT_NEAR(k_0, 1.0, 1e-15) << k_0;

        auto x_0 = targetConv->parameterValueNumeric(
            EPSG_CODE_PARAMETER_FALSE_EASTING, UnitOfMeasure::METRE);
        EXPECT_NEAR(x_0, 700000, 1e-15) << x_0;

        auto y_0 = targetConv->parameterValueNumeric(
            EPSG_CODE_PARAMETER_FALSE_NORTHING, UnitOfMeasure::METRE);
        EXPECT_NEAR(y_0, 6600000, 1e-15) << y_0;
    }

    auto _2sp_from_1sp = targetConv->convertToOtherMethod(
        EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_2SP);
    ASSERT_TRUE(_2sp_from_1sp);

    {
        auto lat_0 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LATITUDE_FALSE_ORIGIN, UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lat_0, 46.5, 1e-15) << lat_0;

        auto lon_0 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LONGITUDE_FALSE_ORIGIN, UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lon_0, 3, 1e-15) << lon_0;

        auto lat_1 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LATITUDE_1ST_STD_PARALLEL,
            UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lat_1, 46.5, 1e-15) << lat_1;

        auto lat_2 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LATITUDE_2ND_STD_PARALLEL,
            UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lat_2, 46.5, 1e-15) << lat_2;

        auto x_0 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_EASTING_FALSE_ORIGIN, UnitOfMeasure::METRE);
        EXPECT_NEAR(x_0, 700000, 1e-15) << x_0;

        auto y_0 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_NORTHING_FALSE_ORIGIN, UnitOfMeasure::METRE);
        EXPECT_NEAR(y_0, 6600000, 1e-15) << y_0;
    }

    EXPECT_TRUE(conv->isEquivalentTo(targetConv.get(),
                                     IComparable::Criterion::EQUIVALENT));
    EXPECT_TRUE(targetConv->isEquivalentTo(conv.get(),
                                           IComparable::Criterion::EQUIVALENT));
}

// ---------------------------------------------------------------------------

TEST(operation, lcc2sp_to_lcc1sp_phi0_diff_phi1_and_phi1_eq_phi2) {

    auto projCRS = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4269, // something using GRS80
        Conversion::createLambertConicConformal_2SP(
            PropertyMap(), Angle(46.123), Angle(3), Angle(46.4567),
            Angle(46.4567), Length(700000), Length(6600000)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));

    auto conv = projCRS->derivingConversion();
    auto targetConv = conv->convertToOtherMethod(
        EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_1SP);
    ASSERT_TRUE(targetConv);

    {
        auto lat_0 = targetConv->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LATITUDE_OF_NATURAL_ORIGIN,
            UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lat_0, 46.4567, 1e-14) << lat_0;

        auto lon_0 = targetConv->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LONGITUDE_OF_NATURAL_ORIGIN,
            UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lon_0, 3.0, 1e-15) << lon_0;

        auto k_0 = targetConv->parameterValueNumeric(
            EPSG_CODE_PARAMETER_SCALE_FACTOR_AT_NATURAL_ORIGIN,
            UnitOfMeasure::SCALE_UNITY);
        EXPECT_NEAR(k_0, 1.0, 1e-15) << k_0;

        auto x_0 = targetConv->parameterValueNumeric(
            EPSG_CODE_PARAMETER_FALSE_EASTING, UnitOfMeasure::METRE);
        EXPECT_NEAR(x_0, 700000, 1e-15) << x_0;

        auto y_0 = targetConv->parameterValueNumeric(
            EPSG_CODE_PARAMETER_FALSE_NORTHING, UnitOfMeasure::METRE);
        EXPECT_NEAR(y_0, 6637093.292952879, 1e-8) << y_0;
    }

    auto _2sp_from_1sp = targetConv->convertToOtherMethod(
        EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_2SP);
    ASSERT_TRUE(_2sp_from_1sp);

    {
        auto lat_0 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LATITUDE_FALSE_ORIGIN, UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lat_0, 46.4567, 1e-14) << lat_0;

        auto lon_0 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LONGITUDE_FALSE_ORIGIN, UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lon_0, 3, 1e-15) << lon_0;

        auto lat_1 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LATITUDE_1ST_STD_PARALLEL,
            UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lat_1, 46.4567, 1e-14) << lat_1;

        auto lat_2 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_LATITUDE_2ND_STD_PARALLEL,
            UnitOfMeasure::DEGREE);
        EXPECT_NEAR(lat_2, 46.4567, 1e-14) << lat_2;

        auto x_0 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_EASTING_FALSE_ORIGIN, UnitOfMeasure::METRE);
        EXPECT_NEAR(x_0, 700000, 1e-15) << x_0;

        auto y_0 = _2sp_from_1sp->parameterValueNumeric(
            EPSG_CODE_PARAMETER_NORTHING_FALSE_ORIGIN, UnitOfMeasure::METRE);
        EXPECT_NEAR(y_0, 6637093.292952879, 1e-8) << y_0;
    }

    EXPECT_TRUE(conv->isEquivalentTo(targetConv.get(),
                                     IComparable::Criterion::EQUIVALENT));
    EXPECT_TRUE(targetConv->isEquivalentTo(conv.get(),
                                           IComparable::Criterion::EQUIVALENT));

    EXPECT_TRUE(_2sp_from_1sp->isEquivalentTo(
        targetConv.get(), IComparable::Criterion::EQUIVALENT));
    EXPECT_TRUE(targetConv->isEquivalentTo(_2sp_from_1sp.get(),
                                           IComparable::Criterion::EQUIVALENT));

    EXPECT_TRUE(conv->isEquivalentTo(_2sp_from_1sp.get(),
                                     IComparable::Criterion::EQUIVALENT));
}

// ---------------------------------------------------------------------------

TEST(operation, lcc1sp_to_lcc2sp_invalid_eccentricity) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), geographicCRSInvalidEccentricity(),
        Conversion::createLambertConicConformal_1SP(PropertyMap(), Angle(40),
                                                    Angle(1), Scale(0.99),
                                                    Length(3), Length(4)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    auto targetConv = projCRS->derivingConversion()->convertToOtherMethod(
        EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_2SP);
    EXPECT_FALSE(targetConv != nullptr);
}

// ---------------------------------------------------------------------------

TEST(operation, lcc1sp_to_lcc2sp_invalid_scale) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4326,
        Conversion::createLambertConicConformal_1SP(
            PropertyMap(), Angle(40), Angle(1), Scale(0), Length(3), Length(4)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    auto targetConv = projCRS->derivingConversion()->convertToOtherMethod(
        EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_2SP);
    EXPECT_FALSE(targetConv != nullptr);
}

// ---------------------------------------------------------------------------

TEST(operation, lcc1sp_to_lcc2sp_invalid_lat0) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4326,
        Conversion::createLambertConicConformal_1SP(PropertyMap(), Angle(100),
                                                    Angle(1), Scale(0.99),
                                                    Length(3), Length(4)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    auto targetConv = projCRS->derivingConversion()->convertToOtherMethod(
        EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_2SP);
    EXPECT_FALSE(targetConv != nullptr);
}

// ---------------------------------------------------------------------------

TEST(operation, lcc1sp_to_lcc2sp_null_lat0) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4326,
        Conversion::createLambertConicConformal_1SP(PropertyMap(), Angle(0),
                                                    Angle(1), Scale(0.99),
                                                    Length(3), Length(4)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    auto targetConv = projCRS->derivingConversion()->convertToOtherMethod(
        EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_2SP);
    EXPECT_FALSE(targetConv != nullptr);
}

// ---------------------------------------------------------------------------

TEST(operation, lcc2sp_to_lcc1sp_invalid_lat0) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4326,
        Conversion::createLambertConicConformal_2SP(
            PropertyMap(), Angle(100), Angle(3), Angle(44), Angle(49),
            Length(700000), Length(6600000)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    auto targetConv = projCRS->derivingConversion()->convertToOtherMethod(
        EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_1SP);
    EXPECT_FALSE(targetConv != nullptr);
}

// ---------------------------------------------------------------------------

TEST(operation, lcc2sp_to_lcc1sp_invalid_lat1) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4326,
        Conversion::createLambertConicConformal_2SP(
            PropertyMap(), Angle(46.5), Angle(3), Angle(100), Angle(49),
            Length(700000), Length(6600000)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    auto targetConv = projCRS->derivingConversion()->convertToOtherMethod(
        EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_1SP);
    EXPECT_FALSE(targetConv != nullptr);
}

// ---------------------------------------------------------------------------

TEST(operation, lcc2sp_to_lcc1sp_invalid_lat2) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4326,
        Conversion::createLambertConicConformal_2SP(
            PropertyMap(), Angle(46.5), Angle(3), Angle(44), Angle(100),
            Length(700000), Length(6600000)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    auto targetConv = projCRS->derivingConversion()->convertToOtherMethod(
        EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_1SP);
    EXPECT_FALSE(targetConv != nullptr);
}

// ---------------------------------------------------------------------------

TEST(operation, lcc2sp_to_lcc1sp_invalid_lat1_opposite_lat2) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4326,
        Conversion::createLambertConicConformal_2SP(
            PropertyMap(), Angle(46.5), Angle(3), Angle(-49), Angle(49),
            Length(700000), Length(6600000)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    auto targetConv = projCRS->derivingConversion()->convertToOtherMethod(
        EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_1SP);
    EXPECT_FALSE(targetConv != nullptr);
}

// ---------------------------------------------------------------------------

TEST(operation, lcc2sp_to_lcc1sp_invalid_lat1_and_lat2_close_to_zero) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), GeographicCRS::EPSG_4326,
        Conversion::createLambertConicConformal_2SP(
            PropertyMap(), Angle(46.5), Angle(3), Angle(.0000000000000001),
            Angle(.0000000000000002), Length(700000), Length(6600000)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    auto targetConv = projCRS->derivingConversion()->convertToOtherMethod(
        EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_1SP);
    EXPECT_FALSE(targetConv != nullptr);
}

// ---------------------------------------------------------------------------

TEST(operation, lcc2sp_to_lcc1sp_invalid_eccentricity) {
    auto projCRS = ProjectedCRS::create(
        PropertyMap(), geographicCRSInvalidEccentricity(),
        Conversion::createLambertConicConformal_2SP(
            PropertyMap(), Angle(46.5), Angle(3), Angle(44), Angle(49),
            Length(700000), Length(6600000)),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
    auto targetConv = projCRS->derivingConversion()->convertToOtherMethod(
        EPSG_CODE_METHOD_LAMBERT_CONIC_CONFORMAL_1SP);
    EXPECT_FALSE(targetConv != nullptr);
}
