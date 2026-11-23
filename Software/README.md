# RFTIR Contact-Plane Reconstruction (Single Device, Post-processing)

A post-processing program that reconstructs contact positions on the contact plane from differential binary images captured by an RFTIR camera.

## Processing Flow

1. Read differential binary RFTIR images (768x432) from the input folder
2. Reconstruct contact / non-contact on the contact plane (1 mm grid) with the LUT (`LUT/fast_lut.bin`)
3. Cluster the contact grid points with DBSCAN to remove noise and compute the centroid of each contact
4. Save the results as PNG (reconstruction images, `denoised/`: denoised images, `clusters/`: cluster visualizations)

## Requirements

- CMake 3.16 or later
- A C++17 compiler
- OpenCV 4 (core, imgproc, imgcodecs / Apache License 2.0; tested with 4.10.0)

## Build and Run

```bash
cmake -S . -B build
cmake --build build
./build/Singledevice_postprocess --input img/debug_img
```

The results are saved to `img/debug_img_recon/`. Run with `--help` to see the options.

## Folder Structure

| Path | Description |
| --- | --- |
| `main.cpp`, `include/`, `src/` | Source code |
| `LUT/` | LUT used for reconstruction (`fast_lut.bin`) |
| `img/debug_img/` | Sample input (differential binary images) |
| `img/raw_img/` | RFTIR images before differencing |
| `img/FTIR_img/` | Reference images of contact positions taken with an FTIR camera |
