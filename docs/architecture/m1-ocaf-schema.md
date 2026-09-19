# Milestone 1A OCAF schema

Milestone 1A uses only standard OCAF attributes. Domain UUIDs—not label entries—are public identities. Numeric tags below are private persistence details and may only be interpreted by `OcafDocumentStore`.

```text
Document.Main()
├── 1 Metadata
│   ├── 1 format name       TDataStd_AsciiString = "DuomecDocument"
│   ├── 2 format version    TDataStd_Integer = 1
│   └── 3 document UUID     TDataStd_AsciiString
└── 3 Bodies
    └── N Body
        ├── 1 UUID          TDataStd_AsciiString
        ├── 2 name          TDataStd_Name
        └── 3 Features
            └── N Feature
                ├── 1 UUID          TDataStd_AsciiString
                ├── 2 type          TDataStd_AsciiString
                ├── 3 name          TDataStd_Name
                ├── 4 enabled       TDataStd_Integer
                ├── 5 recompute     TDataStd_Integer
                ├── 6 Parameters
                │   └── N Parameter
                │       ├── 1 UUID   TDataStd_AsciiString
                │       ├── 2 name   TDataStd_Name
                │       └── 3 value  TDataStd_Real (SI)
                └── 7 execution     TDataStd_Integer
```

Tags are positional storage keys, never domain identifiers. Dependencies, references, `TFunction_Function`, and `TNaming_NamedShape` receive reserved feature children in later sub-milestones when they have real semantics; 1A does not persist invented empty data.

## Persistence and transactions

`BinDrivers::DefineFormat` registers `BinOcaf`; files use `.duomec`. Each application mutation opens one OCAF command, rewrites the standard-attribute projection, and commits once. Failure aborts the command. Undo/redo executes OCAF history and rehydrates the authoritative domain snapshot.

Atomic save writes a sibling temporary `.duomec` file, opens it into a second document to validate readability and schema/version, and only then renames it over the destination. No custom attributes or persistence drivers are introduced in 1A.
