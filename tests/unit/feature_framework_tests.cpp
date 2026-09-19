#include "duomec/cad/features/feature_framework.hpp"

#include <iostream>
#include <memory>
#include <stop_token>

namespace {
using namespace duomec::cad;
using namespace duomec::cad::features;

bool check(bool condition, const char *message) {
  if (!condition)
    std::cerr << "FAIL: " << message << '\n';
  return condition;
}

FeatureDefinition extrude_definition() {
  FeatureDefinition definition;
  definition.type_key = "duomec.extrude";
  definition.display_name = "Extrude";
  definition.parameter_schema.parameters = {
      {"distance", "Distance", ParameterType::scalar,
       ParameterDimension::length, 0.01, 1.0e-9, 10.0, true, false},
      {"draft", "Draft angle", ParameterType::scalar, ParameterDimension::angle,
       0.0, -1.4, 1.4, false, true}};
  definition.collectors = {{"profiles",
                            "Profiles",
                            {{SelectionKind::profile}, 1, std::nullopt, false},
                            true}};
  definition.panel_groups = {
      {"primary", "Primary", false, {"distance"}, {"profiles"}},
      {"advanced", "Advanced", true, {"draft"}, {}}};
  definition.supported_operations = {
      BodyOperationMode::new_body, BodyOperationMode::add,
      BodyOperationMode::cut, BodyOperationMode::intersect};
  definition.manipulators = {
      {"distance_handle", ManipulatorKind::distance, "distance"}};
  return definition;
}

class RecordingExecutor final : public IFeatureExecutor {
public:
  FeatureResult execute(const FeatureDefinition &,
                        const FeatureExecutionContext &context,
                        std::stop_token cancellation) override {
    ++calls;
    qualities.push_back(context.quality);
    last_token = cancellation;
    FeatureResult result;
    result.status = cancellation.stop_requested() ? FeatureStatus::cancelled
                                                  : FeatureStatus::success;
    result.shapes.push_back({"transient-shape"});
    return result;
  }
  int calls{};
  std::vector<ExecutionQuality> qualities;
  std::stop_token last_token;
};

class RecordingTransaction final : public IFeatureTransaction {
public:
  duomec::core::Result<FeatureResult>
  commit(const FeatureDefinition &definition,
         const FeatureExecutionContext &final_context,
         IFeatureExecutor &executor, std::stop_token cancellation) override {
    ++attempts;
    if (final_context.quality != ExecutionQuality::final)
      return duomec::core::Result<FeatureResult>::failure(
          {duomec::core::ErrorCode::internal,
           "commit did not receive final quality",
           {}});
    FeatureResult result =
        executor.execute(definition, final_context, cancellation);
    if (result.status == FeatureStatus::success ||
        result.status == FeatureStatus::warning)
      ++commits;
    return duomec::core::Result<FeatureResult>::success(std::move(result));
  }
  int attempts{};
  int commits{};
};
} // namespace

int main() {
  using namespace duomec::cad;
  using namespace duomec::cad::features;
  bool ok = true;
  RecordingExecutor executor;
  RecordingTransaction transaction;
  FeaturePreviewSession session(extrude_definition(), executor, transaction);

  ok &= check(session.consume_preselection(
                  {{"profile:sketch-a:wire-1", SelectionKind::profile,
                    "Rectangle", true}}) == 1,
              "valid preselection is consumed");
  ok &= check(session.set_parameter("distance", 0.025).has_value(),
              "keyboard scalar entry updates typed parameter");
  ok &= check(session.set_body_operation(BodyOperationMode::add).has_value(),
              "body operation is selected explicitly");
  session.set_extent({ExtentKind::blind, 0.025, std::nullopt, false});
  auto preview = session.preview();
  ok &= check(preview && preview.value().status == FeatureStatus::success,
              "valid session previews");
  ok &= check(session.state() == PreviewSessionState::preview_ready,
              "preview state is visible");
  ok &= check(executor.qualities.size() == 1 &&
                  executor.qualities[0] == ExecutionQuality::preview,
              "preview quality is explicit");
  session.request_preview_cancel();
  ok &= check(executor.last_token.stop_requested(),
              "preview cancellation reaches executor token");

  auto committed = session.commit();
  ok &= check(committed && committed.value().status == FeatureStatus::success,
              "final execution commits");
  ok &= check(transaction.attempts == 1 && transaction.commits == 1,
              "commit creates one transaction unit");
  ok &= check(executor.qualities.back() == ExecutionQuality::final,
              "commit executes final quality");
  ok &= check(session.state() == PreviewSessionState::committed,
              "session closes after commit");
  ok &= check(!session.set_parameter("distance", 0.030),
              "committed session rejects further edits");

  RecordingExecutor invalid_executor;
  RecordingTransaction invalid_transaction;
  FeaturePreviewSession invalid(extrude_definition(), invalid_executor,
                                invalid_transaction);
  ok &= check(invalid.set_parameter("distance", -1.0).has_value(),
              "typed input is retained for diagnostics");
  auto invalid_preview = invalid.preview();
  ok &= check(invalid_preview &&
                  invalid_preview.value().status == FeatureStatus::failure &&
                  invalid_executor.calls == 0,
              "invalid input never reaches geometry executor");
  ok &= check(invalid_transaction.attempts == 0,
              "invalid preview creates no transaction");

  FeaturePreviewSession cancelled(extrude_definition(), invalid_executor,
                                  invalid_transaction);
  cancelled.consume_preselection(
      {{"profile:one", SelectionKind::profile, "Profile", true}});
  cancelled.cancel();
  ok &= check(cancelled.state() == PreviewSessionState::cancelled &&
                  invalid_transaction.attempts == 0,
              "cancel leaves no persistent change");

  const FeatureId edited = FeatureId::generate();
  FeaturePreviewSession edit_session(extrude_definition(), invalid_executor,
                                     invalid_transaction, edited);
  ok &= check(edit_session.context().edited_feature == edited &&
                  edit_session.definition().type_key == "duomec.extrude",
              "edit reuses creation definition and identifies target feature");

  SelectionCollector edge_collector(
      {"edges", "Edges", {{SelectionKind::edge}, 1, 1, false}, true});
  ok &= check(
      edge_collector.add({"edge:stable-1", SelectionKind::edge, "Edge", true})
          .has_value(),
      "collector accepts allowed reference");
  ok &= check(
      !edge_collector.add({"face:stable-1", SelectionKind::face, "Face", true}),
      "collector rejects wrong topology kind");
  ok &= check(!edge_collector.add(
                  {"edge:stable-2", SelectionKind::edge, "Edge 2", true}),
              "collector enforces maximum count");
  ok &= check(edge_collector.remove("edge:stable-1") &&
                  edge_collector.selections().empty(),
              "collector chip can remove its persistent reference");

  return ok ? 0 : 1;
}
