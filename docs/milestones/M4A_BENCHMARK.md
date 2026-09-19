# M4A discrete import baseline

No pre-M4 mesh import/display implementation existed, so there is no valid before-change baseline. M4A establishes the first reproducible import baseline. Display frame time remains unavailable because Qt/VTK are not installed; it is not reported as zero.

## Method

The benchmark deterministically writes temporary datasets, then times import only:

* a 708 × 708 planar vertex grid triangulated into 999,698 OBJ triangles;
* an XYZ cloud containing 1,000,000 points;
* explicit millimetre-to-SI conversion;
* metadata/history serialization for one body of each type.

Peak RSS uses Linux `getrusage`; domain payload bytes are deterministic vector element payloads and exclude allocator/hash-table overhead.

## Observation — 2026-09-19

| Dataset | Elements | Asset size | Domain payload | Import | Peak process RSS |
|---|---:|---:|---:|---:|---:|
| OBJ mesh | 999,698 triangles | 28,193,466 B | 24,026,712 B | 5,627–5,820 ms | 145,670,144–145,788,928 B |
| XYZ cloud | 1,000,000 points | 10,191,761 B | 24,000,000 B | 888–909 ms | 145,670,144–145,788,928 B |

Combined metadata for both external assets and their two import-history records was 753 bytes and encoded in 455–463 µs. Dataset generation is excluded from import timing. This unoptimized native baseline is not a marketing target or an Open3D comparison.

## Build-size observation

| Artifact | Size |
|---|---:|
| `libduomec_discrete_geometry.a` | 1,908,142 B |
| `libduomec_native_discrete_adapter.a` | 935,968 B |
| Discrete unit-test executable | 1,153,968 B |
| Discrete benchmark executable | 1,076,552 B |

These are default-build static artifacts with compiler/debug choices inherited from the runner; installed release impact must be measured from a release/package configuration.
