# M5.2 Project Chrono mapping prototype

Project Chrono **9.0.1** remains the pinned future integration baseline. No Chrono headers, libraries, objects, or time integration are introduced by Milestone 5.2.

| Duomec semantic | Neutral compiled entity | Future Chrono prototype mapping |
|---|---|---|
| Ground / Fixed occurrence | FixedBody | fixed `ChBody` |
| Lock | FixedJoint | `ChLinkMateFix` or equivalent fixed link |
| Hinge / Revolute semantic | Revolute | revolute link |
| Prismatic semantic | Prismatic | prismatic link |
| Universal Joint | Universal | universal link |
| Screw | Helical | screw/helical link |
| Gear | Gear | gear relation/link |
| Rack and Pinion | RackPinion | rack-pinion relation |
| Belt / Chain | BeltPulley | pulley/belt relation where supported, otherwise custom relation |
| Path | Trajectory | trajectory or generic link |
| Other mates | GenericConstraint | backend constraint equations |

Compilation is explicit and scope-limited. Local kinematic frames become future joint/reaction frames. A confirmed high-level Hinge suppresses redundant Coincident/Concentric exports for the same occurrence pair. The adapter must validate exact Chrono APIs and license/build packaging before production integration.
