#include "duomec/cad/document/ids.hpp"

#include <array>
#include <cctype>
#include <iomanip>
#include <random>
#include <sstream>

namespace duomec::cad {
namespace {
bool valid_uuid(std::string_view value) {
  if (value.size() != 36)
    return false;
  for (std::size_t index = 0; index < value.size(); ++index) {
    const bool hyphen = index == 8 || index == 13 || index == 18 || index == 23;
    if (hyphen ? value[index] != '-'
               : std::isxdigit(static_cast<unsigned char>(value[index])) == 0) {
      return false;
    }
  }
  return true;
}

std::string make_uuid_v4() {
  std::array<unsigned char, 16> bytes{};
  thread_local std::mt19937_64 source = [] {
    std::random_device entropy;
    std::seed_seq seed{entropy(), entropy(), entropy(), entropy(),
                       entropy(), entropy(), entropy(), entropy()};
    return std::mt19937_64(seed);
  }();
  for (std::size_t offset = 0; offset < bytes.size(); offset += 8) {
    const auto random = source();
    for (std::size_t index = 0; index < 8; ++index)
      bytes[offset + index] = static_cast<unsigned char>(random >> (index * 8));
  }
  bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0fU) | 0x40U);
  bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3fU) | 0x80U);
  std::ostringstream stream;
  stream << std::hex << std::setfill('0');
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    if (index == 4 || index == 6 || index == 8 || index == 10)
      stream << '-';
    stream << std::setw(2) << static_cast<unsigned>(bytes[index]);
  }
  return stream.str();
}
} // namespace

template <class Tag> PersistentId<Tag> PersistentId<Tag>::generate() {
  return PersistentId(make_uuid_v4());
}

template <class Tag>
core::Result<PersistentId<Tag>>
PersistentId<Tag>::parse(std::string_view text) {
  if (!valid_uuid(text)) {
    return core::Result<PersistentId>::failure(
        {core::ErrorCode::invalid_argument, "invalid persistent UUID",
         std::string(text)});
  }
  std::string canonical(text);
  for (char &value : canonical)
    value = static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
  return core::Result<PersistentId>::success(
      PersistentId(std::move(canonical)));
}

template class PersistentId<DocumentIdTag>;
template class PersistentId<BodyIdTag>;
template class PersistentId<FeatureIdTag>;
template class PersistentId<SketchIdTag>;
template class PersistentId<SketchEntityIdTag>;
template class PersistentId<ConstraintIdTag>;
template class PersistentId<ParameterIdTag>;
template class PersistentId<TopologyReferenceIdTag>;
template class PersistentId<MeshBodyIdTag>;
template class PersistentId<PointCloudBodyIdTag>;
template class PersistentId<DiscreteFeatureIdTag>;
template class PersistentId<DiscreteRegionIdTag>;
template class PersistentId<AssemblyRelationIdTag>;
template class PersistentId<RelationEndpointIdTag>;
template class PersistentId<KinematicFrameIdTag>;
template class PersistentId<SolveIslandIdTag>;
template class PersistentId<PartDefinitionIdTag>;
template class PersistentId<PartRevisionIdTag>;
template class PersistentId<BodyRevisionIdTag>;
template class PersistentId<AssemblyDefinitionIdTag>;
template class PersistentId<AssemblyRevisionIdTag>;
template class PersistentId<OccurrenceIdTag>;

} // namespace duomec::cad
