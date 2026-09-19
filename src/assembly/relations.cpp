#include "duomec/assembly/relations.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace duomec::assembly {
namespace {
double dot(Vector3 a, Vector3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
double norm(Vector3 v) { return std::sqrt(dot(v, v)); }

class Writer {
public:
  template <class T> void pod(const T &v) {
    static_assert(std::is_trivially_copyable_v<T>);
    const auto *p = reinterpret_cast<const char *>(&v);
    out_.append(p, sizeof(T));
  }
  void string(std::string_view v) {
    const auto n = static_cast<std::uint32_t>(v.size());
    pod(n);
    out_.append(v);
  }
  template <class Id> void id(const Id &v) { string(v.value()); }
  void vec(Vector3 v) {
    pod(v.x);
    pod(v.y);
    pod(v.z);
  }
  std::string take() { return std::move(out_); }

private:
  std::string out_;
};
class Reader {
public:
  explicit Reader(std::string_view in) : in_(in) {}
  template <class T> T pod() {
    static_assert(std::is_trivially_copyable_v<T>);
    if (in_.size() < sizeof(T))
      throw std::runtime_error("truncated assembly relation data");
    T v;
    std::memcpy(&v, in_.data(), sizeof(T));
    in_.remove_prefix(sizeof(T));
    return v;
  }
  std::string string() {
    const auto n = pod<std::uint32_t>();
    if (in_.size() < n)
      throw std::runtime_error("truncated assembly relation string");
    std::string v(in_.substr(0, n));
    in_.remove_prefix(n);
    return v;
  }
  template <class Id> Id id() {
    auto parsed = Id::parse(string());
    if (!parsed)
      throw std::runtime_error("invalid persistent id");
    return parsed.value();
  }
  Vector3 vec() { return {pod<double>(), pod<double>(), pod<double>()}; }
  bool empty() const { return in_.empty(); }

private:
  std::string_view in_;
};
template <class E> void enumeration(Writer &w, E e) {
  w.pod(static_cast<std::uint8_t>(e));
}
template <class E> E enumeration(Reader &r) {
  return static_cast<E>(r.pod<std::uint8_t>());
}
template <class C, class F> void sequence(Writer &w, const C &c, F f) {
  w.pod(static_cast<std::uint32_t>(c.size()));
  for (const auto &v : c)
    f(v);
}
template <class F> void count(Reader &r, F f) {
  const auto n = r.pod<std::uint32_t>();
  if (n > 1'000'000)
    throw std::runtime_error("unreasonable collection size");
  for (std::uint32_t i = 0; i < n; ++i)
    f();
}

void writeRefs(Writer &w, const LightweightReferenceMetadata &m) {
  for (const auto &v : m.references) {
    enumeration(w, v.kind);
    w.id(v.referenceId);
  }
}
LightweightReferenceMetadata readRefs(Reader &r) {
  LightweightReferenceMetadata m;
  for (auto &v : m.references) {
    v.kind = enumeration<AbsoluteReferenceKind>(r);
    v.referenceId = r.id<cad::TopologyReferenceId>();
  }
  return m;
}
void writeDof(Writer &w, const DofState &d) {
  for (bool v : d.free)
    w.pod(static_cast<std::uint8_t>(v));
  enumeration(w, d.state);
}
DofState readDof(Reader &r) {
  DofState d;
  for (auto &v : d.free)
    v = r.pod<std::uint8_t>() != 0;
  d.state = enumeration<ConstraintState>(r);
  return d;
}
void writeFrame(Writer &w, const LocalKinematicFrame &f) {
  w.id(f.id);
  w.vec(f.origin);
  w.vec(f.xAxis);
  w.vec(f.yAxis);
  w.vec(f.zAxis);
}
LocalKinematicFrame readFrame(Reader &r) {
  LocalKinematicFrame f;
  f.id = r.id<cad::KinematicFrameId>();
  f.origin = r.vec();
  f.xAxis = r.vec();
  f.yAxis = r.vec();
  f.zAxis = r.vec();
  return f;
}
void writeGeometry(Writer &w, const GeometryDescriptor &g) {
  enumeration(w, g.kind);
  w.vec(g.origin);
  w.vec(g.direction);
  w.vec(g.secondaryDirection);
  w.pod(g.radius);
  w.pod(g.secondaryRadius);
  w.pod(g.angleRadians);
  sequence(w, g.samples, [&](auto v) { w.vec(v); });
}
GeometryDescriptor readGeometry(Reader &r) {
  GeometryDescriptor g;
  g.kind = enumeration<GeometryKind>(r);
  g.origin = r.vec();
  g.direction = r.vec();
  g.secondaryDirection = r.vec();
  g.radius = r.pod<double>();
  g.secondaryRadius = r.pod<double>();
  g.angleRadians = r.pod<double>();
  count(r, [&] { g.samples.push_back(r.vec()); });
  return g;
}
void writeMotion(Writer &w, const MotionSemanticDescriptor &m) {
  enumeration(w, m.semantic);
  sequence(w, m.parameters, [&](const auto &v) {
    w.string(v.first);
    w.pod(v.second);
  });
}
MotionSemanticDescriptor readMotion(Reader &r) {
  MotionSemanticDescriptor m;
  m.semantic = enumeration<MotionSemantic>(r);
  count(r, [&] {
    auto key = r.string();
    auto value = r.pod<double>();
    m.parameters.emplace(std::move(key), value);
  });
  return m;
}
void writeParameter(Writer &w, const RelationParameter &p) {
  w.pod(static_cast<std::uint8_t>(p.index()));
  std::visit(
      [&](const auto &v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>)
          w.string(v);
        else
          w.pod(v);
      },
      p);
}
RelationParameter readParameter(Reader &r) {
  switch (r.pod<std::uint8_t>()) {
  case 0:
    return r.pod<bool>();
  case 1:
    return r.pod<std::int64_t>();
  case 2:
    return r.pod<double>();
  case 3:
    return r.string();
  default:
    throw std::runtime_error("invalid parameter type");
  }
}
} // namespace

LightweightReferenceMetadata LightweightReferenceMetadata::create() {
  LightweightReferenceMetadata result;
  for (std::size_t i = 0; i < result.references.size(); ++i)
    result.references[i] = {static_cast<AbsoluteReferenceKind>(i),
                            cad::TopologyReferenceId::generate()};
  return result;
}
const AbsoluteReference &
LightweightReferenceMetadata::at(AbsoluteReferenceKind kind) const {
  const auto found =
      std::find_if(references.begin(), references.end(),
                   [&](const auto &r) { return r.kind == kind; });
  if (found == references.end())
    throw std::out_of_range("absolute reference missing");
  return *found;
}
DofState DofState::floating() { return {}; }
DofState DofState::fixed() {
  DofState d;
  d.free.fill(false);
  d.state = ConstraintState::Fixed;
  return d;
}
std::size_t DofState::independentDofCount() const {
  return static_cast<std::size_t>(std::count(free.begin(), free.end(), true));
}
bool DofState::isFree(Dof dof) const {
  return free.at(static_cast<std::size_t>(dof));
}
bool DofState::isConstrained(Dof dof) const { return !isFree(dof); }
core::Result<bool> LocalKinematicFrame::validate(double tolerance) const {
  const bool unit = std::abs(norm(xAxis) - 1) < tolerance &&
                    std::abs(norm(yAxis) - 1) < tolerance &&
                    std::abs(norm(zAxis) - 1) < tolerance;
  const bool perpendicular = std::abs(dot(xAxis, yAxis)) < tolerance &&
                             std::abs(dot(yAxis, zAxis)) < tolerance &&
                             std::abs(dot(zAxis, xAxis)) < tolerance;
  if (!unit || !perpendicular)
    return core::Result<bool>::failure({core::ErrorCode::invalid_argument,
                                        "kinematic frame must be orthonormal",
                                        "LocalKinematicFrame"});
  return core::Result<bool>::success(true);
}
OccurrenceState OccurrenceState::defaultOriginAligned() {
  OccurrenceState o;
  o.mobility = PlacementMobility::Fixed;
  o.subassemblyMode = SubassemblySolveMode::Rigid;
  o.insertionMethod = InsertionMethod::OriginAligned;
  o.dofState = DofState::fixed();
  return o;
}
OccurrenceState OccurrenceState::interactive(Transform transform) {
  OccurrenceState o;
  o.localTransform = transform;
  o.mobility = PlacementMobility::Floating;
  o.insertionMethod = InsertionMethod::Interactive;
  o.dofState = DofState::floating();
  return o;
}

std::string serialize(const AssemblyRelationsSnapshot &s) {
  Writer w;
  w.string("DUOMEC_ASSEMBLY_RELATIONS");
  w.pod(s.schemaVersion);
  sequence(w, s.parts, [&](const auto &p) {
    w.id(p.id);
    w.string(p.revisionHash);
    writeRefs(w, p.absoluteReferences);
  });
  sequence(w, s.assemblies, [&](const auto &a) {
    w.id(a.id);
    w.string(a.revisionHash);
    writeRefs(w, a.absoluteReferences);
  });
  sequence(w, s.occurrences, [&](const auto &o) {
    w.id(o.id);
    for (double v : o.localTransform.matrix)
      w.pod(v);
    enumeration(w, o.mobility);
    enumeration(w, o.subassemblyMode);
    enumeration(w, o.insertionMethod);
    writeDof(w, o.dofState);
  });
  sequence(w, s.relations, [&](const auto &rel) {
    w.id(rel.id);
    enumeration(w, rel.type);
    sequence(w, rel.endpoints, [&](const auto &e) {
      w.id(e.id);
      w.id(e.occurrenceId);
      w.id(e.topologyReferenceId);
      writeGeometry(w, e.geometry);
      writeFrame(w, e.localFrame);
    });
    sequence(w, rel.parameters, [&](const auto &p) {
      w.string(p.first);
      writeParameter(w, p.second);
    });
    enumeration(w, rel.state);
    writeDof(w, rel.dofState);
    w.pod(static_cast<std::uint8_t>(rel.solveIsland.has_value()));
    if (rel.solveIsland)
      w.id(*rel.solveIsland);
    writeMotion(w, rel.motion);
    w.pod(static_cast<std::uint8_t>(rel.analysisHint.has_value()));
    if (rel.analysisHint) {
      enumeration(w, rel.analysisHint->kind);
      w.string(rel.analysisHint->userNote);
      w.pod(rel.analysisHint->explicitlyAccepted);
    }
  });
  return w.take();
}
core::Result<AssemblyRelationsSnapshot> deserialize(std::string_view bytes) {
  try {
    Reader r(bytes);
    if (r.string() != "DUOMEC_ASSEMBLY_RELATIONS")
      throw std::runtime_error("invalid assembly relation header");
    AssemblyRelationsSnapshot s;
    s.schemaVersion = r.pod<std::uint32_t>();
    if (s.schemaVersion != AssemblyRelationsSnapshot::currentSchemaVersion)
      throw std::runtime_error("unsupported assembly relation schema");
    count(r, [&] {
      PartDefinitionMetadata p;
      p.id = r.id<cad::PartDefinitionId>();
      p.revisionHash = r.string();
      p.absoluteReferences = readRefs(r);
      s.parts.push_back(std::move(p));
    });
    count(r, [&] {
      AssemblyDefinitionMetadata a;
      a.id = r.id<cad::AssemblyDefinitionId>();
      a.revisionHash = r.string();
      a.absoluteReferences = readRefs(r);
      s.assemblies.push_back(std::move(a));
    });
    count(r, [&] {
      OccurrenceState o;
      o.id = r.id<cad::OccurrenceId>();
      for (auto &v : o.localTransform.matrix)
        v = r.pod<double>();
      o.mobility = enumeration<PlacementMobility>(r);
      o.subassemblyMode = enumeration<SubassemblySolveMode>(r);
      o.insertionMethod = enumeration<InsertionMethod>(r);
      o.dofState = readDof(r);
      s.occurrences.push_back(std::move(o));
    });
    count(r, [&] {
      AssemblyRelation rel;
      rel.id = r.id<cad::AssemblyRelationId>();
      rel.type = enumeration<RelationType>(r);
      count(r, [&] {
        RelationEndpoint e;
        e.id = r.id<cad::RelationEndpointId>();
        e.occurrenceId = r.id<cad::OccurrenceId>();
        e.topologyReferenceId = r.id<cad::TopologyReferenceId>();
        e.geometry = readGeometry(r);
        e.localFrame = readFrame(r);
        rel.endpoints.push_back(std::move(e));
      });
      count(r, [&] {
        auto key = r.string();
        auto value = readParameter(r);
        rel.parameters.emplace(std::move(key), std::move(value));
      });
      rel.state = enumeration<RelationState>(r);
      rel.dofState = readDof(r);
      if (r.pod<std::uint8_t>())
        rel.solveIsland = r.id<cad::SolveIslandId>();
      rel.motion = readMotion(r);
      if (r.pod<std::uint8_t>()) {
        AnalysisRelationHint h;
        h.kind = enumeration<AnalysisHintKind>(r);
        h.userNote = r.string();
        h.explicitlyAccepted = r.pod<bool>();
        rel.analysisHint = std::move(h);
      }
      s.relations.push_back(std::move(rel));
    });
    if (!r.empty())
      throw std::runtime_error("trailing assembly relation data");
    return core::Result<AssemblyRelationsSnapshot>::success(std::move(s));
  } catch (const std::exception &e) {
    return core::Result<AssemblyRelationsSnapshot>::failure(
        {core::ErrorCode::io_failure, e.what(), "AssemblyRelationsSnapshot"});
  }
}
} // namespace duomec::assembly
