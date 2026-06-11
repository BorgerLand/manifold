// Copyright 2021 The Manifold Authors.
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
#include <cstdint>  // uint32_t, uint64_t
#include <functional>
#include <memory>  // needed for shared_ptr
#include <mutex>

#include "manifold/common.h"
#include "manifold/mesh.h"
#include "manifold/vec_view.h"
#include <iostream>
#include "../../../generated/test.h"

namespace manifold {

/**
 * @ingroup Debug
 *
 * Allows modification of the assertions checked in MANIFOLD_DEBUG mode.
 *
 * @return ExecutionParams&
 */
inline ExecutionParams& ManifoldParams() {
  static ExecutionParams p;
  return p;
}

class CsgNode;
class CsgLeafNode;

/** @addtogroup Core
 *  @brief The central classes of the library
 *  @{
 */

/**
 * @brief This library's internal representation of an oriented, 2-manifold,
 * triangle mesh - a simple boundary-representation of a solid object. Use this
 * class to store and operate on solids, and use MeshGL for input and output.
 *
 * In addition to storing geometric data, a Manifold can also store an arbitrary
 * number of vertex properties. These could be anything, e.g. normals, UV
 * coordinates, colors, etc, but this library is completely agnostic. All
 * properties are merely float values indexed by channel number. It is up to the
 * user to associate channel numbers with meaning.
 *
 * Manifold allows vertex properties to be shared for efficient storage, or to
 * have multiple property verts associated with a single geometric vertex,
 * allowing sudden property changes, e.g. at Boolean intersections, without
 * sacrificing manifoldness.
 *
 * Manifolds also keep track of their relationships to their inputs, via
 * OriginalIDs and the faceIDs and transforms accessible through MeshGL. This
 * allows object-level properties to be re-associated with the output after many
 * operations, particularly useful for materials. Since separate object's
 * properties are not mixed, there is no requirement that channels have
 * consistent meaning between different inputs.
 */
class Manifold {
 public:
  /** @name Basics
   *  Copy / move / assignment
   */
  ///@{
  Manifold() : internal(rust::meshbool::MeshBool::default_()) {}
  ~Manifold() = default;
  Manifold(const Manifold& other) { this->internal = other.internal.clone(); }
  Manifold& operator=(const Manifold& other) { this->internal = other.internal.clone(); return *this; }
  Manifold(Manifold&& other) noexcept : internal(std::move(other.internal)) {}
  Manifold& operator=(Manifold&& other) noexcept { this->internal = std::move(other.internal); return *this; }
  ///@}

  /** @name Input & Output
   *  Create and retrieve arbitrary manifolds
   */
  ///@{
  Manifold(const MeshGL& mesh) {
    auto gl = CPPMeshToRSMesh(mesh);
    internal = rust::meshbool::MeshBool::from_meshgl(gl);
  }
  Manifold(const MeshGL64& mesh) {
    auto gl = CPPMeshToRSMesh(mesh);
    internal = rust::meshbool::MeshBool::from_meshgl(gl);
  }
  MeshGL GetMeshGL(int normalIdx = -1) const {
    return RSMeshToCPPMesh<float, uint32_t>(internal.get_mesh_gl_32(normalIdx));
  }
  MeshGL64 GetMeshGL64(int normalIdx = -1) const {
    return RSMeshToCPPMesh<double, uint64_t>(internal.get_mesh_gl_64(normalIdx));
  }
  ///@}

  /** @name Constructors
   *  Topological ops, primitives, and SDF
   */
  ///@{
  std::vector<Manifold> Decompose() const {
    auto rs = internal.decompose();
    std::vector<Manifold> out;
    out.reserve(rs.len());
    for (size_t i = 0; i < rs.len(); i++) {
      Manifold m;
      m.internal = rs.get(i).unwrap().clone();
      out.push_back(std::move(m));
    }
    return out;
  }
  [[deprecated(
      "Compose is deprecated, use BatchBoolean with OpType::Add instead.")]]
  static Manifold Compose(const std::vector<Manifold>&) {
    std::cout << "Manifold::Compose(): STUB, not implemented in Rust\n";
    return {};
  }
  static Manifold Tetrahedron() {
    Manifold r;
    r.internal = rust::meshbool::MeshBool::tetrahedron();
    return r;
  }
  static Manifold Cube(vec3 size = vec3(1.0), bool center = false) {
    Manifold r;
    r.internal = rust::meshbool::MeshBool::cube(
        rust::nalgebra::Vector3<double>::new_(size.x, size.y, size.z),
        rust::Bool(center));
    return r;
  }
  static Manifold Cylinder(double height, double radiusLow,
                            double radiusHigh = -1.0,
                            int circularSegments = 0, bool center = false) {
    Manifold r;
    r.internal = rust::meshbool::MeshBool::cylinder(
        height, radiusLow,
        radiusHigh < 0.0 ? radiusLow : radiusHigh,
        (uint32_t)circularSegments, rust::Bool(center));
    return r;
  }
  static Manifold Sphere(double radius, int circularSegments = 0) {
    Manifold r;
    r.internal = rust::meshbool::MeshBool::sphere(radius, circularSegments);
    return r;
  }
  static Manifold LevelSet(std::function<double(vec3)> sdf, Box bounds,
                           double edgeLength, double level = 0,
                           double tolerance = -1, bool canParallel = true) {
    std::cout << "Manifold::LevelSet(): STUB, not implemented in Rust\n";
    return {};
  }
  ///@}

  /** @name Polygons
   * 3D to 2D and 2D to 3D
   */
  ///@{
  Polygons Slice(double height = 0) const {
    auto rs = internal.slice(height);
    return RSPolygonsToCPPPolygons(rs);
  }
  Polygons Project() const {
    auto rs = internal.project();
    return RSPolygonsToCPPPolygons(rs);
  }
  static Manifold Extrude(const Polygons& crossSection, double height,
                          int nDivisions = 0, double twistDegrees = 0.0,
                          vec2 scaleTop = vec2(1.0)) {
    auto rs = CPPPolygonsToRSPolygons(crossSection);
    Manifold r;
    r.internal = rust::meshbool::MeshBool::extrude(
        rs, height, (uint32_t)nDivisions,
        twistDegrees,
        rust::nalgebra::Vector2<double>::new_(scaleTop.x, scaleTop.y));
    return r;
  }
  static Manifold Revolve(const Polygons& crossSection,
                          int circularSegments = 0,
                          double revolveDegrees = 360.0f) {
    auto rs = CPPPolygonsToRSPolygons(crossSection);
    Manifold r;
    r.internal = rust::meshbool::MeshBool::revolve(rs, circularSegments,
                                                   revolveDegrees);
    return r;
  }
  ///@}

  enum class Error {
    NoError,
    NonFiniteVertex,
    NotManifold,
    VertexOutOfBounds,
    PropertiesWrongLength,
    MissingPositionProperties,
    MergeVectorsDifferentLengths,
    MergeIndexOutOfBounds,
    TransformWrongLength,
    RunIndexWrongLength,
    FaceIDWrongLength,
    InvalidConstruction,
    ResultTooLarge,
    InvalidTangents,
    Cancelled,
  };

  /** @name Information
   *  Details of the manifold
   */
  ///@{
  Error Status() const {
    auto e = internal.status();
    if (e.is_non_finite_vertex()) return Error::NonFiniteVertex;
    if (e.is_not_manifold()) return Error::NotManifold;
    if (e.is_vertex_out_of_bounds()) return Error::VertexOutOfBounds;
    if (e.is_missing_position_properties()) return Error::MissingPositionProperties;
    if (e.is_merge_vectors_different_lengths()) return Error::MergeVectorsDifferentLengths;
    if (e.is_merge_index_out_of_bounds()) return Error::MergeIndexOutOfBounds;
    if (e.is_transform_wrong_length()) return Error::TransformWrongLength;
    if (e.is_run_index_wrong_length()) return Error::RunIndexWrongLength;
    if (e.is_face_id_wrong_length()) return Error::FaceIDWrongLength;
    if (e.is_invalid_construction()) return Error::InvalidConstruction;
    if (e.is_result_too_large()) return Error::ResultTooLarge;
    return Error::NoError;
  }

  /// Returns a copy of this Manifold with the given ExecutionContext attached.
  /// The attachment is consumed only by `Status()` (for deferred CSG trees)
  /// and the eager ops (`Refine` / `RefineToLength` / `RefineToTolerance`,
  /// `Hull`, `MinkowskiSum` / `MinkowskiDifference`); those snapshot the ctx
  /// and report progress / observe cancellation through it. Other queries
  /// that force evaluation (`Volume`, `GetMeshGL`, `BoundingBox`, etc.) do
  /// not currently observe attached ctx.
  ///
  /// Deferred ops (Boolean operators, Translate / Rotate / Scale / Transform
  /// / Mirror / Warp / SetTolerance / Simplify, BatchBoolean, the
  /// vector-of-Manifold Hull) ignore any attached ctx and produce a result
  /// with no attached ctx. Inputs are not mutated. The idiom for observing a
  /// deferred tree is therefore:
  ///
  ///   (a + b - c).WithContext(ctx).Status();
  ///
  /// while the idiom for observing an eager op is:
  ///
  ///   m.WithContext(ctx).Refine(n);
  ///   m.WithContext(ctx).MinkowskiSum(other);
  ///
  /// Raw copy / assignment preserves the attachment (it's the same logical
  /// Manifold). Only ops that derive a *new* Manifold drop the attachment.
  Manifold WithContext(const ExecutionContext& ctx) const {
    std::cout << "Manifold::WithContext(): STUB, not implemented in Rust\n";
    return *this;
  }

  bool IsEmpty() const { return internal.is_empty(); }
  size_t NumVert() const { return internal.num_vert(); }
  size_t NumEdge() const { return internal.num_edge(); }
  size_t NumTri() const { return internal.num_tri(); }
  size_t NumProp() const { return internal.num_prop(); }
  size_t NumPropVert() const { return internal.num_prop_vert(); }
  Box BoundingBox() const {
    auto aabb = internal.bounding_box();
    return Box(vec3(aabb.min.get_x(), aabb.min.get_y(), aabb.min.get_z()),
               vec3(aabb.max.get_x(), aabb.max.get_y(), aabb.max.get_z()));
  }
  int Genus() const { return internal.genus(); }
  double GetTolerance() const { return internal.get_tolerance(); }
  ///@}

  /** @name Measurement
   */
  ///@{
  double SurfaceArea() const {
    return internal.surface_area();
  }
  double Volume() const {
    return internal.volume();
  }
  double MinGap(const Manifold& other, double searchLength) const {
    return internal.min_gap(other.internal, searchLength);
  }
  std::vector<RayHit> RayCast(vec3 origin, vec3 endpoint) const {
    std::cout << "Manifold::RayCast(): STUB, not implemented in Rust\n";
    return {};
  }
  std::vector<int> WindingNumber(const std::vector<vec3>& points) const {
    std::cout << "Manifold::WindingNumber(): STUB, not implemented in Rust\n";
    return std::vector<int>(points.size(), 0);
  }
  ///@}

  /** @name Mesh ID
   *  Details of the manifold's relation to its input meshes, for the purposes
   * of reapplying mesh properties.
   */
  ///@{
  int OriginalID() const { return internal.original_id(); }
  Manifold AsOriginal() const {
    Manifold r;
    r.internal = internal.as_original();
    return r;
  }
  static uint32_t ReserveIDs(uint32_t n) {
    return rust::meshbool::MeshBool::reserve_ids(n);
  }
  ///@}

  /** @name Transformations
   */
  ///@{
  Manifold Translate(vec3 v) const {
    Manifold r;
    r.internal = internal.translate(rust::nalgebra::Vector3<double>::new_(v.x, v.y, v.z));
    return r;
  }
  Manifold Scale(vec3 v) const {
    Manifold r;
    r.internal = internal.scale(rust::nalgebra::Vector3<double>::new_(v.x, v.y, v.z));
    return r;
  }
  Manifold Rotate(double xDegrees, double yDegrees = 0.0,
                  double zDegrees = 0.0) const {
    Manifold r;
    r.internal = internal.rotate(xDegrees, yDegrees, zDegrees);
    return r;
  }
  Manifold Mirror(vec3 v) const {
    Manifold r;
    r.internal = internal.mirror(rust::nalgebra::Vector3<double>::new_(v.x, v.y, v.z));
    return r;
  }
  Manifold Transform(const mat3x4& m) const {
    // la::mat is column-major: 4 columns (x,y,z,w), each a 3-vec (rows).
    double cols[12] = {
        m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2],
        m[2][0], m[2][1], m[2][2], m[3][0], m[3][1], m[3][2],
    };
    auto slice = rust::std::slice::from_raw_parts(cols, 12);
    auto mat = rust::nalgebra::Matrix3x4<double>::from_column_slice(slice);
    Manifold r;
    r.internal =
        internal.transform(mat);
    return r;
  }
  Manifold Warp(std::function<void(vec3&)> f) const {
    using RustPoint3 = rust::nalgebra::Point3<double>;
    using FT = rust::Box<rust::Dyn<rust::FnMut<rust::RefMut<RustPoint3>, rust::Unit>>>;
    auto f_new = FT::make_box([f](rust::RefMut<RustPoint3> v) -> rust::Unit {
      vec3 nv(v.get_x(), v.get_y(), v.get_z());
      f(nv);
      rust::RawMut<RustPoint3> raw(v);
      raw.write(RustPoint3::new_(nv.x, nv.y, nv.z));
      return {};
    });
    Manifold r;
    r.internal = internal.warp(std::move(f_new));
    return r;
  }
  Manifold WarpBatch(std::function<void(VecView<vec3>)> f) const {
    using RustPoint3 = rust::nalgebra::Point3<double>;
    using FT = rust::Box<
        rust::Dyn<rust::FnMut<rust::RefMut<rust::Slice<RustPoint3>>, rust::Unit>>>;
    auto f_new = FT::make_box(
        [f](rust::RefMut<rust::Slice<RustPoint3>> vec) -> rust::Unit {
          size_t n = vec.len();
          auto raw = vec.as_mut_ptr();
          std::vector<vec3> tmp;
          tmp.reserve(n);
          for (size_t i = 0; i < n; i++) {
            RustPoint3 p = raw.offset(i).read();
            tmp.push_back({p.get_x(), p.get_y(), p.get_z()});
          }
          f({tmp.data(), tmp.size()});
          for (size_t i = 0; i < n; i++) {
            auto& v = tmp[i];
            raw.offset(i).write(RustPoint3::new_(v.x, v.y, v.z));
          }
          return {};
        });
    Manifold r;
    r.internal = internal.warp_batch(std::move(f_new));
    return r;
  }
  Manifold SetTolerance(double t) const {
    Manifold r;
    r.internal = internal.set_tolerance(t);
    return r;
  }
  Manifold Simplify(double tolerance = 0) const {
    using OptD = rust::core::option::Option<double>;
    Manifold r;
    r.internal = internal.simplify(tolerance == 0 ? OptD::None()
                                                  : OptD::Some(tolerance));
    return r;
  }
  ///@}

  /** @name Boolean
   *  Combine two manifolds
   */
  ///@{
  Manifold Boolean(const Manifold& second, OpType op) const {
    Manifold r;
    if (op == OpType::Add)
      r.internal = internal.boolean(second.internal, rust::meshbool::OpType::Add());
    else if (op == OpType::Subtract)
      r.internal = internal.boolean(second.internal, rust::meshbool::OpType::Subtract());
    else
      r.internal = internal.boolean(second.internal, rust::meshbool::OpType::Intersect());
    return r;
  }
  static Manifold BatchBoolean(const std::vector<Manifold>& manifolds,
                               OpType op) {
    if (manifolds.empty()) return Manifold();
    Manifold r = manifolds[0];
    for (size_t i = 1; i < manifolds.size(); i++)
      r = r.Boolean(manifolds[i], op);
    return r;
  }
  // Boolean operation shorthand
  Manifold operator+(const Manifold& o) const { return Boolean(o, OpType::Add); }
  Manifold& operator+=(const Manifold& o) { *this = Boolean(o, OpType::Add); return *this; }
  Manifold operator-(const Manifold& o) const { return Boolean(o, OpType::Subtract); }
  Manifold& operator-=(const Manifold& o) { *this = Boolean(o, OpType::Subtract); return *this; }
  Manifold operator^(const Manifold& o) const { return Boolean(o, OpType::Intersect); }
  Manifold& operator^=(const Manifold& o) { *this = Boolean(o, OpType::Intersect); return *this; }
  std::pair<Manifold, Manifold> Split(const Manifold& cutter) const {
    auto t = internal.split(cutter.internal);
    Manifold a, b;
    a.internal = t.f0.clone();
    b.internal = t.f1.clone();
    return {a, b};
  }
  std::pair<Manifold, Manifold> SplitByPlane(vec3 normal,
                                             double originOffset) const {
    auto t = internal.split_by_plane(
        rust::nalgebra::Vector3<double>::new_(normal.x, normal.y, normal.z),
        originOffset);
    Manifold a, b;
    a.internal = t.f0.clone();
    b.internal = t.f1.clone();
    return {a, b};
  }
  Manifold TrimByPlane(vec3 normal, double originOffset) const {
    Manifold r;
    r.internal = internal.trim_by_plane(
        rust::nalgebra::Vector3<double>::new_(normal.x, normal.y, normal.z),
        originOffset);
    return r;
  }
  Manifold MinkowskiSum(const Manifold&) const {
    std::cout << "Manifold::MinkowskiSum(): STUB, not implemented in Rust\n";
    return *this;
  }
  Manifold MinkowskiDifference(const Manifold&) const {
    std::cout << "Manifold::MinkowskiDifference(): STUB, not implemented in Rust\n";
    return *this;
  }
  ///@}

  /** @name Properties
   * Create and modify vertex properties.
   */
  ///@{
  Manifold SetProperties(
      int numProp,
      std::function<void(double*, vec3, const double*)> propFunc) const {
    using FT = rust::Box<rust::Dyn<rust::FnMut<
        rust::RefMut<rust::Slice<double>>, rust::nalgebra::Point3<double>,
        rust::Ref<rust::Slice<double>>, rust::Unit>>>;
    using OptFT = rust::core::option::Option<FT>;
    auto maybe = (propFunc == nullptr)
                     ? OptFT::None()
                     : OptFT::Some(FT::make_box(
                           [propFunc](rust::RefMut<rust::Slice<double>> a,
                                      rust::nalgebra::Point3<double> b,
                                      rust::Ref<rust::Slice<double>> c)
                               -> rust::Unit {
                             vec3 nb(b.get_x(), b.get_y(), b.get_z());
                             propFunc(a.as_mut_ptr(), nb, c.as_ptr());
                             return {};
                           }));
    Manifold r;
    r.internal = internal.set_properties(numProp, std::move(maybe));
    return r;
  }
  Manifold CalculateCurvature(int gaussianIdx, int meanIdx) const {
    Manifold r;
    r.internal = internal.calculate_curvature(gaussianIdx, meanIdx);
    return r;
  }
  Manifold CalculateNormals(int normalIdx = 0,
                            double minSharpAngle = 52.5) const {
    Manifold r;
    r.internal = internal.calculate_normals(normalIdx, minSharpAngle);
    return r;
  }
  ///@}

  /** @name Smoothing
   * Smooth meshes by calculating tangent vectors and refining to a higher
   * triangle count.
   */
  ///@{
  Manifold Refine(int) const {
    std::cout << "Manifold::Refine(): STUB, not implemented in Rust\n";
    return *this;
  }
  Manifold RefineToLength(double) const {
    std::cout << "Manifold::RefineToLength(): STUB, not implemented in Rust\n";
    return *this;
  }
  Manifold RefineToTolerance(double) const {
    std::cout << "Manifold::RefineToTolerance(): STUB, not implemented in Rust\n";
    return *this;
  }
  Manifold SmoothByNormals(int normalIdx = 0) const {
    std::cout << "Manifold::SmoothByNormals(): STUB, not implemented in Rust\n";
    return *this;
  }
  Manifold SmoothOut(double minSharpAngle = 52.5,
                     double minSmoothness = 0) const {
    std::cout << "Manifold::SmoothOut(): STUB, not implemented in Rust\n";
    return *this;
  }
  static Manifold Smooth(const MeshGL&,
                         const std::vector<Smoothness>& sharpenedEdges = {}) {
    std::cout << "Manifold::Smooth(MeshGL): STUB, not implemented in Rust\n";
    return {};
  }
  static Manifold Smooth(const MeshGL64&,
                         const std::vector<Smoothness>& sharpenedEdges = {}) {
    std::cout << "Manifold::Smooth(MeshGL64): STUB, not implemented in Rust\n";
    return {};
  }
  ///@}

  /** @name Convex Hull
   */
  ///@{
  Manifold Hull() const {
    std::cout << "Manifold::Hull(): STUB, not implemented in Rust\n";
    return *this;
  }
  static Manifold Hull(const std::vector<Manifold>& manifolds) {
    std::cout << "Manifold::Hull(std::vector<Manifold>&): STUB, not implemented in Rust\n";
    return {};
  }
  static Manifold Hull(const std::vector<vec3>& pts) {
    std::cout << "Manifold::Hull(std::vector<vec3>&): STUB, not implemented in Rust\n";
    return {};
  }
  ///@}

  /** @name I/O
   * Self-contained mechanism for reading and writing high precision Manifold
   * data.  Write function creates special-purpose OBJ files, and Read function
   * reads them in.
   *
   * To work with a file, the caller should prepare the ifstream/ostream
   * themselves, as follows:
   *
   * Reading:
   * @code
   * std::ifstream ifile;
   * ifile.open(filename);
   * if (ifile.is_open()) {
   *   Manifold obj_m = Manifold::ReadOBJ(ifile);
   *   ifile.close();
   *   if (obj_m.Status() != Manifold::Error::NoError) {
   *      std::cerr << "Failed reading " << filename << ":\n";
   *      std::cerr << Manifold::ToString(obj_m.Status()) << "\n";
   *   }
   *   ifile.close();
   * }
   * @endcode
   *
   * Writing:
   * @code
   * std::ofstream ofile;
   * ofile.open(filename);
   * if (ofile.is_open()) {
   *    if (!m.WriteOBJ(ofile)) {
   *       std::cerr << "Failed writing to " << filename << "\n";
   *    }
   * }
   * ofile.close();
   * @endcode
   */
#ifndef MANIFOLD_NO_IOSTREAM
  static Manifold ReadOBJ(std::istream& stream) {
    std::cout << "Manifold::ReadOBJ(): STUB, not implemented in Rust\n";
    return {};
  }
  bool WriteOBJ(std::ostream& stream) const {
    std::cout << "Manifold::WriteOBJ(): STUB, not implemented in Rust\n";
    return false;
  }
#endif

  /** @name Testing Hooks
   *  These are just for internal testing.
   */
  ///@{
  bool MatchesTriNormals() const { return internal.matches_tri_normals(); }
  size_t NumDegenerateTris() const { return internal.num_degenerate_tris(); }
  double GetEpsilon() const { return internal.get_epsilon(); }
  ///@}

  struct Impl;

  /// @internal Wrap a fully-built Impl into a leaf-node Manifold.
  /// Caller is responsible for the invariants the public ctors enforce
  /// (in particular, calling `MakeEmpty(status)` on error). Used by
  /// ctx-aware static factories on `ExecutionContext`.
  static Manifold FromImpl(std::shared_ptr<Impl> pImpl);

 private:
  Manifold(std::shared_ptr<CsgNode> pNode_);
  Manifold(std::shared_ptr<Impl> pImpl_);
  static Manifold Invalid();
  static Manifold PropagateStatus(Error status);
  mutable std::shared_ptr<std::mutex> pNodeMutex_ =
      std::make_shared<std::mutex>();
  mutable std::shared_ptr<CsgNode> pNode_;
  // Optional attached ExecutionContext. shared_ptr so the Impl outlives
  // the user's ExecutionContext if a ctx-attached Manifold survives it.
  // Propagates through copy ctor / op= (raw copy preserves the attachment).
  // Manifold-returning ops do *not* propagate it: derived Manifolds get a
  // null ctx_. Eager ops (Status, Refine family) snapshot ctx_ to observe
  // their in-call work; the snapshot uses std::atomic_load, which pins the
  // Impl across long-running evaluations even if a concurrent op= reseats
  // ctx_ mid-eval.
  //
  // Accessed only via std::atomic_load / std::atomic_store: no const method
  // mutates ctx_, but op= and the copy ctor write it on a Manifold that
  // may be concurrently observed by const methods on other threads. The
  // atomic-shared-ptr free functions give a torn-read-free snapshot
  // without taking a lock. (pNode_ uses a mutex instead because lazy CSG
  // eval mutates it through const methods, which atomic_load can't model.)
  std::shared_ptr<ExecutionContext::Impl> ctx_;

  std::shared_ptr<CsgNode> LoadPNode() const;
  rust::meshbool::MeshBool internal;
  CsgLeafNode& GetCsgLeafNode(ExecutionContext::Impl* ctx = nullptr) const;
};
/** @} */

/** @addtogroup Debug
 *  @ingroup Optional
 *  @brief Debugging features
 *
 * The features require compiler flags to be enabled. Assertions are enabled
 * with the MANIFOLD_DEBUG flag and then controlled with ExecutionParams.
 *  @{
 */
#ifdef MANIFOLD_DEBUG
inline std::string ToString(const Manifold::Error& error) {
  switch (error) {
    case Manifold::Error::NoError:
      return "No Error";
    case Manifold::Error::NonFiniteVertex:
      return "Non Finite Vertex";
    case Manifold::Error::NotManifold:
      return "Not Manifold";
    case Manifold::Error::VertexOutOfBounds:
      return "Vertex Out Of Bounds";
    case Manifold::Error::PropertiesWrongLength:
      return "Properties Wrong Length";
    case Manifold::Error::MissingPositionProperties:
      return "Missing Position Properties";
    case Manifold::Error::MergeVectorsDifferentLengths:
      return "Merge Vectors Different Lengths";
    case Manifold::Error::MergeIndexOutOfBounds:
      return "Merge Index Out Of Bounds";
    case Manifold::Error::TransformWrongLength:
      return "Transform Wrong Length";
    case Manifold::Error::RunIndexWrongLength:
      return "Run Index Wrong Length";
    case Manifold::Error::FaceIDWrongLength:
      return "Face ID Wrong Length";
    case Manifold::Error::InvalidConstruction:
      return "Invalid Construction";
    case Manifold::Error::ResultTooLarge:
      return "Result Too Large";
    case Manifold::Error::InvalidTangents:
      return "Invalid Tangents";
    case Manifold::Error::Cancelled:
      return "Cancelled";
    default:
      return "Unknown Error";
  };
}

inline std::ostream& operator<<(std::ostream& stream,
                                const Manifold::Error& error) {
  return stream << ToString(error);
}
#endif
/** @} */
}  // namespace manifold
