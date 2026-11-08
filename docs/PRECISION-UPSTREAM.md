# Precision upstream API provenance

Checked 2026-11-08 against the upstream MSL and both Precision source trees. This note explains why upstream MSL can use special-stagger annotations while standard Precision 2.0.6 cannot supply their cached payload.

## The two repositories expose different APIs

Upstream ModernStaggerLock requests `InterfaceVersion::V5` and, when the request succeeds, registers its plugin name with `AddExtraParameterName` ([`LoadGame.cpp`](https://github.com/max-su-2019/ModernStaggerLock/blob/master/src/LoadGame.cpp)). Its special-stagger handler retrieves the annotation string with `GetCachedExtraData` from the target actor's cached hit data ([`SpecialStaggerHandler.cpp`](https://github.com/max-su-2019/ModernStaggerLock/blob/master/src/SpecialStaggerHandler.cpp)). Those methods are present in the Precision API maintained in the MSL author's separate [`max-su-2019/Precision` source](https://github.com/max-su-2019/Precision/blob/main/src/PrecisionAPI.h).

The standard [`ersh1/Precision` API](https://github.com/ersh1/Precision/blob/main/src/PrecisionAPI.h) defines only V1–V4 and defaults requests to V4. It does not define the V5 annotation registration or cached extra-data methods. Therefore MSL and “Precision” version numbers alone do not establish compatibility: upstream MSL's special-payload path depends on the max-su V5 API lineage, while standard Precision 2.0.6 is the V1–V4 provider examined in this project.

The MSL commit titled “Updated to v1.1.7 / Updated for new Precision API” changed `LoadGame.cpp`, `PrecisionAPI.h`, and `SpecialStaggerHandler.cpp` together ([commit `3ce822f`](https://github.com/max-su-2019/ModernStaggerLock/commit/3ce822f)). This is direct evidence of an MSL-to-Precision API update; no release-date comparison is needed to explain the difference.

## What happens without V5

Upstream MSL's load code only calls `AddExtraParameterName` when the V5 request returns a pointer, and its special-stagger handler returns false when the pointer is absent. In this repository, [`src/LoadGame.cpp`](../src/LoadGame.cpp) logs that special-stagger payloads are disabled, while [`src/SpecialStaggerHandler.cpp`](../src/SpecialStaggerHandler.cpp) returns without processing those payloads if Precision V5 is unavailable. The ordinary MSL hooks are installed independently before the optional Precision request, so missing V5 disables that integration feature rather than the whole plugin.

## Sources

- [Upstream MSL Precision API update commit](https://github.com/max-su-2019/ModernStaggerLock/commit/3ce822f)
- [Upstream MSL API request and parameter registration](https://github.com/max-su-2019/ModernStaggerLock/blob/master/src/LoadGame.cpp)
- [Upstream MSL cached payload consumer](https://github.com/max-su-2019/ModernStaggerLock/blob/master/src/SpecialStaggerHandler.cpp)
- [max-su-2019 Precision API](https://github.com/max-su-2019/Precision/blob/main/src/PrecisionAPI.h)
- [standard ersh1 Precision API](https://github.com/ersh1/Precision/blob/main/src/PrecisionAPI.h)
