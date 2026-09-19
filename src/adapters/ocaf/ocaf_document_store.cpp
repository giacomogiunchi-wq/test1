#include "ocaf_document_store.hpp"

#include <BinDrivers.hxx>
#include <PCDM_ReaderStatus.hxx>
#include <PCDM_StoreStatus.hxx>
#include <Standard_Failure.hxx>
#include <TCollection_AsciiString.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDF_ChildIterator.hxx>
#include <TDataStd_AsciiString.hxx>
#include <TDataStd_Integer.hxx>
#include <TDataStd_Name.hxx>
#include <TDataStd_Real.hxx>
#include <TDocStd_Application.hxx>
#include <TDocStd_Document.hxx>

#include <cstdio>
#include <filesystem>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif

namespace duomec::adapters::ocaf {
namespace {
using cad::DocumentSnapshot;
constexpr int kMetadata = 1;
constexpr int kBodies = 3;

core::Error persistence_error(std::string message, std::string context = {}) {
  return {core::ErrorCode::io_failure, std::move(message), std::move(context)};
}
void set_ascii(const TDF_Label &label, std::string_view value) {
  TDataStd_AsciiString::Set(label, TCollection_AsciiString(value.data()));
}
void set_name(const TDF_Label &label, std::string_view value) {
  TDataStd_Name::Set(label,
                     TCollection_ExtendedString(value.data(), Standard_True));
}
std::string get_ascii(const TDF_Label &label) {
  Handle(TDataStd_AsciiString) attribute;
  if (!label.FindAttribute(TDataStd_AsciiString::GetID(), attribute))
    return {};
  return attribute->Get().ToCString();
}
std::string get_name(const TDF_Label &label) {
  Handle(TDataStd_Name) attribute;
  if (!label.FindAttribute(TDataStd_Name::GetID(), attribute))
    return {};
  return TCollection_AsciiString(attribute->Get()).ToCString();
}
template <class Id> core::Result<Id> read_id(const TDF_Label &label) {
  return Id::parse(get_ascii(label));
}

core::Result<bool> replace_file(const std::filesystem::path &temporary,
                                const std::filesystem::path &target) {
#ifdef _WIN32
  if (!MoveFileExW(temporary.c_str(), target.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    return core::Result<bool>::failure(persistence_error(
        "atomic document replacement failed", target.string()));
  }
#else
  if (std::rename(temporary.c_str(), target.c_str()) != 0) {
    return core::Result<bool>::failure(persistence_error(
        "atomic document replacement failed", target.string()));
  }
#endif
  return core::Result<bool>::success(true);
}
} // namespace

class OcafDocumentStore::Impl {
public:
  Impl() : application(new TDocStd_Application) {
    BinDrivers::DefineFormat(application);
  }

  core::Result<bool> write(const DocumentSnapshot &snapshot) {
    if (document.IsNull())
      return core::Result<bool>::failure(
          persistence_error("OCAF document is not open"));
    const TDF_Label main = document->Main();
    main.FindChild(kMetadata, Standard_True).ForgetAllAttributes(Standard_True);
    main.FindChild(kBodies, Standard_True).ForgetAllAttributes(Standard_True);
    const TDF_Label metadata = main.FindChild(kMetadata, Standard_True);
    set_ascii(metadata.FindChild(1, Standard_True), "DuomecDocument");
    TDataStd_Integer::Set(metadata.FindChild(2, Standard_True),
                          snapshot.format_version);
    set_ascii(metadata.FindChild(3, Standard_True), snapshot.id.value());
    const TDF_Label bodies = main.FindChild(kBodies, Standard_True);
    int body_tag = 1;
    for (const auto &body : snapshot.bodies) {
      const TDF_Label body_label = bodies.FindChild(body_tag++, Standard_True);
      set_ascii(body_label.FindChild(1, Standard_True), body.id.value());
      set_name(body_label.FindChild(2, Standard_True), body.name);
      const TDF_Label features = body_label.FindChild(3, Standard_True);
      int feature_tag = 1;
      for (const auto &feature : body.features) {
        const TDF_Label feature_label =
            features.FindChild(feature_tag++, Standard_True);
        set_ascii(feature_label.FindChild(1, Standard_True),
                  feature.id.value());
        set_ascii(feature_label.FindChild(2, Standard_True), "generic");
        set_name(feature_label.FindChild(3, Standard_True), feature.name);
        TDataStd_Integer::Set(feature_label.FindChild(4, Standard_True),
                              feature.enabled ? 1 : 0);
        TDataStd_Integer::Set(feature_label.FindChild(5, Standard_True),
                              static_cast<int>(feature.recompute_state));
        const TDF_Label parameters = feature_label.FindChild(6, Standard_True);
        int parameter_tag = 1;
        for (const auto &parameter : feature.parameters) {
          const TDF_Label parameter_label =
              parameters.FindChild(parameter_tag++, Standard_True);
          set_ascii(parameter_label.FindChild(1, Standard_True),
                    parameter.id.value());
          set_name(parameter_label.FindChild(2, Standard_True), parameter.name);
          TDataStd_Real::Set(parameter_label.FindChild(3, Standard_True),
                             parameter.value_si);
        }
        TDataStd_Integer::Set(feature_label.FindChild(7, Standard_True),
                              static_cast<int>(feature.execution_status));
      }
    }
    return core::Result<bool>::success(true);
  }

  core::Result<DocumentSnapshot> read() const {
    if (document.IsNull())
      return core::Result<DocumentSnapshot>::failure(
          persistence_error("OCAF document is not open"));
    const TDF_Label metadata =
        document->Main().FindChild(kMetadata, Standard_False);
    Handle(TDataStd_Integer) version;
    if (metadata.IsNull() ||
        get_ascii(metadata.FindChild(1, Standard_False)) != "DuomecDocument" ||
        !metadata.FindChild(2, Standard_False)
             .FindAttribute(TDataStd_Integer::GetID(), version)) {
      return core::Result<DocumentSnapshot>::failure(
          persistence_error("not a Duomec document"));
    }
    if (version->Get() != DocumentSnapshot::current_format_version) {
      return core::Result<DocumentSnapshot>::failure(
          persistence_error("unsupported Duomec document version",
                            std::to_string(version->Get())));
    }
    auto document_id =
        read_id<cad::DocumentId>(metadata.FindChild(3, Standard_False));
    if (!document_id)
      return core::Result<DocumentSnapshot>::failure(document_id.error());
    DocumentSnapshot snapshot{version->Get(), document_id.value(), {}};
    const TDF_Label bodies =
        document->Main().FindChild(kBodies, Standard_False);
    for (TDF_ChildIterator body_iterator(bodies); body_iterator.More();
         body_iterator.Next()) {
      const TDF_Label body_label = body_iterator.Value();
      if (get_ascii(body_label.FindChild(1, Standard_False)).empty())
        continue;
      auto body_id =
          read_id<cad::BodyId>(body_label.FindChild(1, Standard_False));
      if (!body_id)
        return core::Result<DocumentSnapshot>::failure(body_id.error());
      cad::Body body{body_id.value(),
                     get_name(body_label.FindChild(2, Standard_False)),
                     {}};
      for (TDF_ChildIterator feature_iterator(
               body_label.FindChild(3, Standard_False));
           feature_iterator.More(); feature_iterator.Next()) {
        const TDF_Label feature_label = feature_iterator.Value();
        if (get_ascii(feature_label.FindChild(1, Standard_False)).empty())
          continue;
        auto feature_id =
            read_id<cad::FeatureId>(feature_label.FindChild(1, Standard_False));
        if (!feature_id)
          return core::Result<DocumentSnapshot>::failure(feature_id.error());
        cad::Feature feature{
            feature_id.value(),
            get_name(feature_label.FindChild(3, Standard_False)),
            cad::FeatureType::generic,
            true,
            {},
            {},
            cad::FeatureExecutionStatus::not_executed,
            cad::RecomputeState::clean,
            {}};
        Handle(TDataStd_Integer) enabled;
        Handle(TDataStd_Integer) state;
        Handle(TDataStd_Integer) execution;
        if (feature_label.FindChild(4, Standard_False)
                .FindAttribute(TDataStd_Integer::GetID(), enabled))
          feature.enabled = enabled->Get() != 0;
        if (feature_label.FindChild(5, Standard_False)
                .FindAttribute(TDataStd_Integer::GetID(), state))
          feature.recompute_state =
              static_cast<cad::RecomputeState>(state->Get());
        if (feature_label.FindChild(7, Standard_False)
                .FindAttribute(TDataStd_Integer::GetID(), execution))
          feature.execution_status =
              static_cast<cad::FeatureExecutionStatus>(execution->Get());
        for (TDF_ChildIterator parameter_iterator(
                 feature_label.FindChild(6, Standard_False));
             parameter_iterator.More(); parameter_iterator.Next()) {
          const TDF_Label parameter_label = parameter_iterator.Value();
          if (get_ascii(parameter_label.FindChild(1, Standard_False)).empty())
            continue;
          auto parameter_id = read_id<cad::ParameterId>(
              parameter_label.FindChild(1, Standard_False));
          Handle(TDataStd_Real) value;
          if (!parameter_id ||
              !parameter_label.FindChild(3, Standard_False)
                   .FindAttribute(TDataStd_Real::GetID(), value)) {
            return core::Result<DocumentSnapshot>::failure(
                persistence_error("invalid parameter record"));
          }
          feature.parameters.push_back(
              {parameter_id.value(),
               get_name(parameter_label.FindChild(2, Standard_False)),
               value->Get()});
        }
        body.features.push_back(std::move(feature));
      }
      snapshot.bodies.push_back(std::move(body));
    }
    return core::Result<DocumentSnapshot>::success(std::move(snapshot));
  }

  Handle(TDocStd_Application) application;
  Handle(TDocStd_Document) document;
};

OcafDocumentStore::OcafDocumentStore() : impl_(std::make_unique<Impl>()) {}
OcafDocumentStore::~OcafDocumentStore() = default;
core::Result<bool>
OcafDocumentStore::initialize(const DocumentSnapshot &snapshot) {
  try {
    impl_->application->NewDocument("BinOcaf", impl_->document);
    impl_->document->SetUndoLimit(100);
    impl_->document->OpenCommand();
    auto result = impl_->write(snapshot);
    if (!result) {
      impl_->document->AbortCommand();
      return result;
    }
    impl_->document->CommitCommand();
    impl_->document->ClearUndos();
    return core::Result<bool>::success(true);
  } catch (const Standard_Failure &failure) {
    return core::Result<bool>::failure(
        persistence_error(failure.GetMessageString()));
  }
}
core::Result<bool> OcafDocumentStore::begin_transaction() {
  if (impl_->document.IsNull() || impl_->document->HasOpenCommand())
    return core::Result<bool>::failure(
        persistence_error("cannot open OCAF transaction"));
  impl_->document->OpenCommand();
  return core::Result<bool>::success(true);
}
core::Result<bool> OcafDocumentStore::write(const DocumentSnapshot &snapshot) {
  return impl_->write(snapshot);
}
core::Result<bool> OcafDocumentStore::commit_transaction() {
  if (impl_->document.IsNull() || !impl_->document->HasOpenCommand())
    return core::Result<bool>::failure(
        persistence_error("no OCAF transaction to commit"));
  impl_->document->CommitCommand();
  return core::Result<bool>::success(true);
}
void OcafDocumentStore::abort_transaction() noexcept {
  if (!impl_->document.IsNull() && impl_->document->HasOpenCommand())
    impl_->document->AbortCommand();
}
core::Result<bool> OcafDocumentStore::can_undo() const {
  return core::Result<bool>::success(!impl_->document.IsNull() &&
                                     impl_->document->GetAvailableUndos() > 0);
}
core::Result<bool> OcafDocumentStore::can_redo() const {
  return core::Result<bool>::success(!impl_->document.IsNull() &&
                                     impl_->document->GetAvailableRedos() > 0);
}
core::Result<DocumentSnapshot> OcafDocumentStore::undo() {
  if (impl_->document.IsNull() || !impl_->document->Undo())
    return core::Result<DocumentSnapshot>::failure(
        persistence_error("nothing to undo"));
  return impl_->read();
}
core::Result<DocumentSnapshot> OcafDocumentStore::redo() {
  if (impl_->document.IsNull() || !impl_->document->Redo())
    return core::Result<DocumentSnapshot>::failure(
        persistence_error("nothing to redo"));
  return impl_->read();
}
core::Result<bool>
OcafDocumentStore::save_atomic(const std::filesystem::path &path) {
  if (impl_->document.IsNull())
    return core::Result<bool>::failure(
        persistence_error("OCAF document is not open"));
  const auto temporary = std::filesystem::path(path.string() + ".tmp");
  std::error_code error;
  std::filesystem::remove(temporary, error);
  try {
    if (impl_->application->SaveAs(
            impl_->document, TCollection_ExtendedString(
                                 temporary.string().c_str())) != PCDM_SS_OK)
      return core::Result<bool>::failure(
          persistence_error("OCAF temporary save failed", temporary.string()));
    Handle(TDocStd_Application) validation_application =
        new TDocStd_Application;
    BinDrivers::DefineFormat(validation_application);
    Handle(TDocStd_Document) validation;
    if (validation_application->Open(
            TCollection_ExtendedString(temporary.string().c_str()),
            validation) != PCDM_RS_OK)
      return core::Result<bool>::failure(persistence_error(
          "saved OCAF document could not be reopened", temporary.string()));
    Handle(TDocStd_Document) original = impl_->document;
    impl_->document = validation;
    auto checked = impl_->read();
    impl_->document = original;
    validation_application->Close(validation);
    if (!checked)
      return core::Result<bool>::failure(checked.error());
    return replace_file(temporary, path);
  } catch (const Standard_Failure &failure) {
    return core::Result<bool>::failure(
        persistence_error(failure.GetMessageString(), path.string()));
  }
}
core::Result<DocumentSnapshot>
OcafDocumentStore::load(const std::filesystem::path &path) {
  try {
    Handle(TDocStd_Document) loaded;
    if (impl_->application->Open(
            TCollection_ExtendedString(path.string().c_str()), loaded) !=
        PCDM_RS_OK)
      return core::Result<DocumentSnapshot>::failure(
          persistence_error("OCAF document open failed", path.string()));
    if (!impl_->document.IsNull())
      impl_->application->Close(impl_->document);
    impl_->document = loaded;
    impl_->document->SetUndoLimit(100);
    return impl_->read();
  } catch (const Standard_Failure &failure) {
    return core::Result<DocumentSnapshot>::failure(
        persistence_error(failure.GetMessageString(), path.string()));
  }
}

} // namespace duomec::adapters::ocaf
