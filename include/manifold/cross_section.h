// Copyright 2023 The Manifold Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

#include "manifold/common.h"
#include "manifold/vec_view.h"

namespace manifold {

/** @addtogroup Optional
 * @brief Optional features that can be enabled through build flags and may
 * require extra dependencies.
 *  @{
 */

struct PathImpl;

/**
 * @brief Two-dimensional cross sections guaranteed to be without
 * self-intersections, or overlaps between polygons (from construction onwards).
 * Polygon clipping (boolean) and offsetting use Manifold's own robust
 * floating-point predicates.
 */
class CrossSection {
 public:
  /** @name Basics
   *  Copy / move / assignment
   */
  ///@{
  CrossSection() {}
  ~CrossSection() {}

  CrossSection(const CrossSection& other) {
    std::cout << "CrossSection::CrossSection(const CrossSection&): STUB, not implemented in Rust\n";
  }
  CrossSection& operator=(const CrossSection& other) {
    std::cout << "CrossSection::operator=(const CrossSection&): STUB, not implemented in Rust\n";
    return *this;
  }
  CrossSection(CrossSection&&) noexcept {
    std::cout << "CrossSection::CrossSection(CrossSection&&): STUB, not implemented in Rust\n";
  }
  CrossSection& operator=(CrossSection&&) noexcept {
    std::cout << "CrossSection::operator=(CrossSection&&): STUB, not implemented in Rust\n";
    return *this;
  }
  ///@}

  // Adapted from Clipper2 docs:
  // http://www.angusj.com/clipper2/Docs/Units/Clipper/Types/FillRule.htm
  // (Copyright © 2010-2023 Angus Johnson)
  /**
   * Filling rules defining which polygon sub-regions are considered to be
   * inside a given polygon, and which sub-regions will not (based on winding
   * numbers). See the [Clipper2
   * docs](http://www.angusj.com/clipper2/Docs/Units/Clipper/Types/FillRule.htm)
   * for a detailed explaination with illusrations.
   */
  // TODO(#1707): EvenOdd/NonZero/Negative are preserved for backend/API
  // compatibility; remove them when the public construction API can break. The
  // Boolean2 backend supports only Positive construction.
  enum class FillRule {
    EvenOdd,   ///< Only odd numbered sub-regions are filled.
    NonZero,   ///< Only non-zero sub-regions are filled.
    Positive,  ///< Only sub-regions with winding counts > 0 are filled.
    Negative   ///< Only sub-regions with winding counts < 0 are filled.
  };

  /**
   * Specifies the treatment of path/contour joins (corners) when offseting
   * CrossSections; alias of manifold::JoinType (see common.h), shared with the
   * polygon offset implementation.
   */
  using JoinType = ::manifold::JoinType;

  /** @name Input & Output
   */
  ///@{
  CrossSection(const SimplePolygon& contour,
               FillRule fillrule = FillRule::Positive) {
    std::cout << "CrossSection::CrossSection(const SimplePolygon&, FillRule): STUB, not implemented in Rust\n";
  }
  CrossSection(const Polygons& contours,
               FillRule fillrule = FillRule::Positive) {
    std::cout << "CrossSection::CrossSection(const Polygons&, FillRule): STUB, not implemented in Rust\n";
  }
  CrossSection(const Rect& rect) {
    std::cout << "CrossSection::CrossSection(const Rect&): STUB, not implemented in Rust\n";
  }
  Polygons ToPolygons() const {
    std::cout << "CrossSection::ToPolygons(): STUB, not implemented in Rust\n";
    return {};
  }
  ///@}

  /** @name Constructors
   * Topological ops and primitives
   */
  ///@{
  std::vector<CrossSection> Decompose() const {
    std::cout << "CrossSection::Decompose(): STUB, not implemented in Rust\n";
    return {};
  }
  static CrossSection Compose(const std::vector<CrossSection>&) {
    std::cout << "CrossSection::Compose(): STUB, not implemented in Rust\n";
    return {};
  }
  static CrossSection Square(const vec2 dims, bool center = false) {
    std::cout << "CrossSection::Square(): STUB, not implemented in Rust\n";
    return {};
  }
  static CrossSection Circle(double radius, int circularSegments = 0) {
    std::cout << "CrossSection::Circle(): STUB, not implemented in Rust\n";
    return {};
  }
  ///@}

  /** @name Information
   *  Details of the cross-section
   */
  ///@{
  bool IsEmpty() const {
    std::cout << "CrossSection::IsEmpty(): STUB, not implemented in Rust\n";
    return {};
  }
  size_t NumVert() const {
    std::cout << "CrossSection::NumVert(): STUB, not implemented in Rust\n";
    return {};
  }
  size_t NumContour() const {
    std::cout << "CrossSection::NumContour(): STUB, not implemented in Rust\n";
    return {};
  }
  Rect Bounds() const {
    std::cout << "CrossSection::Bounds(): STUB, not implemented in Rust\n";
    return {};
  }
  double Area() const {
    std::cout << "CrossSection::Area(): STUB, not implemented in Rust\n";
    return {};
  }
  ///@}

  /** @name Transformation
   */
  ///@{
  CrossSection Translate(const vec2 v) const {
    std::cout << "CrossSection::Translate(): STUB, not implemented in Rust\n";
    return {};
  }
  CrossSection Rotate(double degrees) const {
    std::cout << "CrossSection::Rotate(): STUB, not implemented in Rust\n";
    return {};
  }
  CrossSection Scale(const vec2 s) const {
    std::cout << "CrossSection::Scale(): STUB, not implemented in Rust\n";
    return {};
  }
  CrossSection Mirror(const vec2 ax) const {
    std::cout << "CrossSection::Mirror(): STUB, not implemented in Rust\n";
    return {};
  }
  CrossSection Transform(const mat2x3& m) const {
    std::cout << "CrossSection::Transform(): STUB, not implemented in Rust\n";
    return {};
  }
  CrossSection Warp(std::function<void(vec2&)> warpFunc) const {
    std::cout << "CrossSection::Warp(): STUB, not implemented in Rust\n";
    return {};
  }
  CrossSection WarpBatch(std::function<void(VecView<vec2>)> warpFunc) const {
    std::cout << "CrossSection::WarpBatch(): STUB, not implemented in Rust\n";
    return {};
  }
  CrossSection Simplify(double epsilon = 1e-6) const {
    std::cout << "CrossSection::Simplify(): STUB, not implemented in Rust\n";
    return {};
  }
  CrossSection Offset(double delta, JoinType jt = JoinType::Round,
                      double miter_limit = 2.0, int circularSegments = 0) const {
    std::cout << "CrossSection::Offset(): STUB, not implemented in Rust\n";
    return {};
  }
  ///@}

  /** @name Boolean
   *  Combine two manifolds
   */
  ///@{
  CrossSection Boolean(const CrossSection& second, OpType op) const {
    std::cout << "CrossSection::Boolean(): STUB, not implemented in Rust\n";
    return {};
  }
  static CrossSection BatchBoolean(
      const std::vector<CrossSection>& crossSections, OpType op) {
    std::cout << "CrossSection::BatchBoolean(): STUB, not implemented in Rust\n";
    return {};
  }
  CrossSection operator+(const CrossSection&) const {
    std::cout << "CrossSection::operator+(): STUB, not implemented in Rust\n";
    return {};
  }
  CrossSection& operator+=(const CrossSection&) {
    std::cout << "CrossSection::operator+=(): STUB, not implemented in Rust\n";
    return *this;
  }
  CrossSection operator-(const CrossSection&) const {
    std::cout << "CrossSection::operator-(): STUB, not implemented in Rust\n";
    return {};
  }
  CrossSection& operator-=(const CrossSection&) {
    std::cout << "CrossSection::operator-=(): STUB, not implemented in Rust\n";
    return *this;
  }
  CrossSection operator^(const CrossSection&) const {
    std::cout << "CrossSection::operator^(): STUB, not implemented in Rust\n";
    return {};
  }
  CrossSection& operator^=(const CrossSection&) {
    std::cout << "CrossSection::operator^=(): STUB, not implemented in Rust\n";
    return *this;
  }
  ///@}

  /** @name Convex Hull
   */
  ///@{
  CrossSection Hull() const {
    std::cout << "CrossSection::Hull(): STUB, not implemented in Rust\n";
    return {};
  }
  static CrossSection Hull(const std::vector<CrossSection>& crossSections) {
    std::cout << "CrossSection::Hull(std::vector<CrossSection>&): STUB, not implemented in Rust\n";
    return {};
  }
  static CrossSection Hull(const SimplePolygon pts) {
    std::cout << "CrossSection::Hull(SimplePolygon): STUB, not implemented in Rust\n";
    return {};
  }
  static CrossSection Hull(const Polygons polys) {
    std::cout << "CrossSection::Hull(Polygons): STUB, not implemented in Rust\n";
    return {};
  }
  ///@}

 private:
  mutable std::mutex pathsMutex_;
  mutable std::shared_ptr<const PathImpl> paths_;
  mutable mat2x3 transform_ = la::identity;
  // Propagated drift budget, analogous to Manifold::Impl::tolerance_.
  mutable double tolerance_ = 0.0;
  CrossSection(std::shared_ptr<const PathImpl> paths);
  std::shared_ptr<const PathImpl> GetPaths() const;
};
/** @} */
}  // namespace manifold
