# GPU Pothos Blocks

This toolkit adds 100+ Pothos blocks that provide optimized implementations
of various mathematical operations by offloading intensive processing onto
a GPU, resulting in significant speed improvements.

Under the hood, the [ArrayFire](https://github.com/arrayfire/arrayfire) library
uses a heavily optimized CUDA or OpenCL implementation, depending on what
is available. If neither are available, it falls back onto a default CPU
implementation.

## Documentation

* https://github.com/pothosware/PothosGPU/wiki (TODO)

## Dependencies

* Pothos library (0.7+)
* ArrayFire (3.6+)
* Mako Python module (build-time only)
* YAML Python module (build-time only)

## Licensing information

This module is licensed under the BSD 3-Clause license. To view the full license, view LICENSE.txt.
