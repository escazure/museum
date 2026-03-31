## Rendering demos

This project is a real-time graphics showcase built in C++/OpenGL. It demonstrates multiple techniques including:

- Phong and Blinn-Phong shading models
- Directional, point, and spot light shadows
- Particle systems
- Instancing with 10K objects
- Procedural texturing
- Post-processing effects: gamma correction, inverse, grayscale, Gaussian blur

---

### Phong / Blinn-Phong Shading Models Comparison

#### Point Light
| Blinn-Phong | Phong |
|-------------|-------|
| <div align="center"><img src="assets/light_comparison/blinn_phong_point.png" alt="blinn-phong point" width="400"/></div> | <div align="center"><img src="assets/light_comparison/phong_point.png" alt="phong point" width="400"/></div> |

---

#### Spot Light
| Blinn-Phong | Phong |
|-------------|-------|
| <div align="center"><img src="assets/light_comparison/blinn_phong_spot.png" alt="blinn-phong spot" width="400"/></div> | <div align="center"><img src="assets/light_comparison/phong_spot.png" alt="phong spot" width="400"/></div> |

---

#### Directional Light
| Blinn-Phong | Phong |
|-------------|-------|
| <div align="center"><img src="assets/light_comparison/blinn_phong_dir.png" alt="blinn-phong dir" width="400"/></div> | <div align="center"><img src="assets/light_comparison/phong_dir.png" alt="phong dir" width="400"/></div> |

---

#### Instancing Demo
*10K instances ~45fps*
<div align="left"><img src="assets/instancing.gif" alt="instancing" width="300"/></div>


---

#### Particle Demo
<div align="left"><img src="assets/particles.gif" alt="particles" width="300"/></div>

---

#### Procedural Texturing Demo
*in shaders*

| Cube | Sphere | Torus |
|------|--------|-------|
| <div align="center"><img src="assets/procedural/cube.gif" alt="cube" width="280"/></div> | <div align="center"><img src="assets/procedural/sphere.gif" alt="sphere" width="280"/></div> | <div align="center"><img src="assets/procedural/torus.gif" alt="torus" width="280"/></div> |

---

#### Directional Shadow Mapping
*Extended to point and spot lights using a perspective matrix*

| Point Light | Spot Light | Directional Light |
|------------|------------|-------------------|
| <div align="center"><img src="assets/dir_shadow_mapping/point.png" alt="point" width="300"/></div>  | <div align="center"><img src="assets/dir_shadow_mapping/spot.png" alt="spot" width="300"/></div>  | <div align="center"><img src="assets/dir_shadow_mapping/dir.png" alt="dir" width="300"/></div> |

> **Note:**  
> Since point and spot lights use a perspective matrix instead of orthographic, you need to adjust bias parameters to avoid shadow acne.

**Parameters used:**
- **Point light:** max = 0.003, min = 0.0001  
- **Spot light:** max = 0.001, min = 0.0001  
- **Directional light:** max = 0.05, min = 0.0001 

---

#### Post-Processing
| Gamma correction | Without gamma correction | Inverse | Grayscale | Gaussian blur |
|------------|------------|-------------------|-------------------|-------------------|
| <div align="center"><img src="assets/post_process/gamma.png" alt="gamma" width="200"/></div>  | <div align="center"><img src="assets/post_process/no_gamma.png" alt="no gamma" width="200"/></div>  | <div align="center"><img src="assets/post_process/inverse.png" alt="inverse" width="200"/></div> | <div align="center"><img src="assets/post_process/grayscale.png" alt="grayscale" width="200"/></div> | <div align="center"><img src="assets/post_process/gaussian_blur.png" alt="gaussian_blur" width="200"/></div> |

---
### Dependencies:


### Build:
```cmake --build build```

### Run:
```./build/museum```

---

### Features Planned
The project roadmap includes implementing additional graphics techniques such as:
- **MSAA (Multi-sample anti-aliasing)**
- **Omnidirectional shadow mapping**
- **Normal mapping**
- **HDR (High dynamic range)**
- **SSAO (Screen-space ambient occlusion)**

---

This project is in the public domain. See [UNLICENSE.txt](UNLICENSE.txt)

---
