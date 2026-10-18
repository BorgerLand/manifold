// Copyright 2026 The Manifold Authors.
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

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "manifold/polygon.h"
#include "shared.h"

#if MANIFOLD_PAR == 1
#include <tbb/combinable.h>
#endif

namespace manifold {

class HalfedgeTriangulation {
 public:
  explicit HalfedgeTriangulation(
      rust::meshbool::triangulation::HalfedgeTriangulation&& internal)
      : internal_(std::move(internal)) {}

  std::vector<ivec3> Triangles() const {
    return TrianglesRS2CPP(internal_.triangles());
  }

 private:
  rust::meshbool::triangulation::HalfedgeTriangulation internal_;
};

// Reuses ear-clipping scratch allocations across sequential calls.
using PolygonTriangulator = rust::meshbool::test::PolygonTriangulator;

// Supplies one reusable triangulator per worker in parallel builds and one per
// operation in sequential builds.
class PolygonTriangulatorStore {
 public:
  PolygonTriangulatorStore()
      : store_(PolygonTriangulator::default_())
  {
  }
  PolygonTriangulator& local() {
#if MANIFOLD_PAR == 1
    return store_.local();
#else
    return store_;
#endif
  }

 private:
#if MANIFOLD_PAR == 1
  tbb::combinable<PolygonTriangulator> store_;
#else
  PolygonTriangulator store_;
#endif
};

HalfedgeTriangulation TriangulateIdxHalfedges(const PolygonsIdx& polys,
                                              double epsilon = -1,
                                              bool allowConvex = true);

inline HalfedgeTriangulation TriangulateIdxHalfedges(
    const PolygonsIdx& polys, double epsilon, bool allowConvex,
    PolygonTriangulator& triangulator) {
  using RustPolyVert = rust::meshbool::triangulation::PolyVert;
  using RustPoint2 = rust::nalgebra::Point2<double>;

  auto rs = ::rust::std::vec::Vec<::rust::std::vec::Vec<RustPolyVert>>::new_();
  for (const auto& poly : polys) {
    auto rs_sub = ::rust::std::vec::Vec<RustPolyVert>::new_();
    for (const auto& v : poly) {
      rs_sub.push(RustPolyVert::new_(
          RustPoint2::new_(v.pos.x, v.pos.y), (uint32_t)v.idx));
    }
    rs.push(std::move(rs_sub));
  }

  return HalfedgeTriangulation(
      rust::meshbool::test::triangulate_idx_halfedges_reuse(
          rs, epsilon, rust::Bool(allowConvex), triangulator));
}

}  // namespace manifold
