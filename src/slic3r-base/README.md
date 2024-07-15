# slic3r-base

This library provides general base foundation for all `slic3r-*` libs. Only bare minimum with system-level deps, intended to be shared across all libs, 
should be placed here. 

If you have something heavier (either implementaiton wise or dependency wise---like OpenGL), consider placing it into `slic3r-shared` or similar lib.
