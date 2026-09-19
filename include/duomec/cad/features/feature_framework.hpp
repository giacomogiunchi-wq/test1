#pragma once

#include "duomec/cad/document/ids.hpp"
#include "duomec/cad/feature_result.hpp"
#include "duomec/core/error.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <stop_token>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace duomec::cad::features {

struct Direction3d {
  double x{};
  double y{};
  double z{1.0};
  auto operator<=>(const Direction3d &) const = default;
};

using ParameterValue =
    std::variant<bool, std::int64_t, double, std::string, Direction3d>;

enum class ParameterType { boolean, integer, scalar, text, direction };
enum class ParameterDimension { none, length, angle, ratio };

struct ParameterDefinition {
  std::string key;
  std::string label;
  ParameterType type{ParameterType::scalar};
  ParameterDimension dimension{ParameterDimension::none};
  ParameterValue default_value{0.0};
  std::optional<double> minimum;
  std::optional<double> maximum;
  bool required{true};
  bool advanced{};
};

struct FeatureParameterSchema {
  std::vector<ParameterDefinition> parameters;
  [[nodiscard]] const ParameterDefinition *find(std::string_view key) const;
};

enum class SelectionKind {
  vertex,
  edge,
  face,
  sketch,
  profile,
  datum,
  solid_body,
  surface_body,
  mesh_body
};

struct SelectionReference {
  std::string persistent_reference;
  SelectionKind kind{SelectionKind::face};
  std::string display_name;
  bool resolved{true};
  auto operator<=>(const SelectionReference &) const = default;
};

struct SelectionRule {
  std::set<SelectionKind> allowed_kinds;
  std::size_t minimum_count{1};
  std::optional<std::size_t> maximum_count;
  bool allow_duplicates{};
};

struct SelectionCollectorDefinition {
  std::string key;
  std::string label;
  SelectionRule rule;
  bool consume_preselection{true};
};

class SelectionCollector {
public:
  explicit SelectionCollector(SelectionCollectorDefinition definition);
  [[nodiscard]] const SelectionCollectorDefinition &definition() const noexcept;
  [[nodiscard]] const std::vector<SelectionReference> &
  selections() const noexcept;
  core::Result<bool> add(SelectionReference selection);
  bool remove(std::string_view persistent_reference);
  std::size_t
  consume_preselection(const std::vector<SelectionReference> &preselection);
  [[nodiscard]] std::vector<FeatureDiagnostic> validate() const;

private:
  SelectionCollectorDefinition definition_;
  std::vector<SelectionReference> selections_;
};

enum class BodyOperationMode { new_body, add, cut, intersect };
enum class ExtentKind {
  blind,
  symmetric,
  through_all,
  up_to_reference,
  up_to_next
};

struct ExtentDefinition {
  ExtentKind kind{ExtentKind::blind};
  double distance_si{};
  std::optional<SelectionReference> limit_reference;
  bool reversed{};
};

struct DirectionDefinition {
  Direction3d vector;
  bool reversed{};
};

enum class ManipulatorKind { distance, direction, angle };
struct FeatureManipulator {
  std::string key;
  ManipulatorKind kind{ManipulatorKind::distance};
  std::string parameter_key;
};

struct FeaturePanelGroup {
  std::string key;
  std::string label;
  bool advanced{};
  std::vector<std::string> parameter_keys;
  std::vector<std::string> collector_keys;
};

struct FeatureDefinition {
  std::string type_key;
  std::string display_name;
  FeatureParameterSchema parameter_schema;
  std::vector<SelectionCollectorDefinition> collectors;
  std::vector<FeaturePanelGroup> panel_groups;
  std::set<BodyOperationMode> supported_operations;
  std::vector<FeatureManipulator> manipulators;
  bool supports_live_preview{true};
};

enum class ExecutionQuality { preview, final };
struct FeatureExecutionContext {
  std::optional<FeatureId> edited_feature;
  std::map<std::string, ParameterValue, std::less<>> parameters;
  std::map<std::string, std::vector<SelectionReference>, std::less<>>
      selections;
  BodyOperationMode body_operation{BodyOperationMode::new_body};
  std::optional<ExtentDefinition> extent;
  std::optional<DirectionDefinition> direction;
  ExecutionQuality quality{ExecutionQuality::preview};
};

class IFeatureExecutor {
public:
  virtual ~IFeatureExecutor() = default;
  virtual FeatureResult execute(const FeatureDefinition &definition,
                                const FeatureExecutionContext &context,
                                std::stop_token cancellation) = 0;
};

class IFeatureTransaction {
public:
  virtual ~IFeatureTransaction() = default;
  virtual core::Result<FeatureResult>
  commit(const FeatureDefinition &definition,
         const FeatureExecutionContext &final_context,
         IFeatureExecutor &executor, std::stop_token cancellation) = 0;
};

enum class PreviewSessionState {
  editing,
  preview_ready,
  invalid,
  committed,
  cancelled
};

class FeaturePreviewSession {
public:
  FeaturePreviewSession(FeatureDefinition definition,
                        IFeatureExecutor &executor,
                        IFeatureTransaction &transaction,
                        std::optional<FeatureId> edited_feature = std::nullopt);

  [[nodiscard]] const FeatureDefinition &definition() const noexcept;
  [[nodiscard]] const FeatureExecutionContext &context() const noexcept;
  [[nodiscard]] PreviewSessionState state() const noexcept;
  [[nodiscard]] const std::optional<FeatureResult> &
  latest_preview() const noexcept;

  core::Result<bool> set_parameter(std::string_view key, ParameterValue value);
  core::Result<bool> set_body_operation(BodyOperationMode operation);
  void set_extent(ExtentDefinition extent);
  void set_direction(DirectionDefinition direction);
  core::Result<bool> add_selection(std::string_view collector_key,
                                   SelectionReference selection);
  core::Result<bool> remove_selection(std::string_view collector_key,
                                      std::string_view persistent_reference);
  std::size_t
  consume_preselection(const std::vector<SelectionReference> &preselection);
  [[nodiscard]] std::vector<FeatureDiagnostic> validate() const;
  core::Result<FeatureResult> preview();
  core::Result<FeatureResult> commit();
  void request_preview_cancel() noexcept;
  void cancel() noexcept;

private:
  [[nodiscard]] bool terminal() const noexcept;
  void synchronize_selections();

  FeatureDefinition definition_;
  FeatureExecutionContext context_;
  std::vector<SelectionCollector> collectors_;
  IFeatureExecutor &executor_;
  IFeatureTransaction &transaction_;
  std::stop_source cancellation_;
  PreviewSessionState state_{PreviewSessionState::editing};
  std::optional<FeatureResult> latest_preview_;
};

} // namespace duomec::cad::features
