# Cg_assignment
Choi Jimin_202315326_컴퓨터공학부


**Environment:**  
- Visual Studio 2022 (x64) / Windows 11  
- freeglut 3.6.0, glew 2.2.0, glfw3 3.4 (via vcpkg)  

---

## ✅ Features

- **Q1 – Phong Shading & Shadows**  
  Implements ambient, diffuse, and specular shading per object.  
  Shadow rays determine whether a point is lit.
![Q1](https://github.com/user-attachments/assets/9a120819-1fd5-4acf-b76a-85bc335643c8)


- **Q2 – Gamma Correction**  
  Applies γ = 2.2 correction to improve brightness perception.

![Q2](https://github.com/user-attachments/assets/1d033b03-51e0-4e47-b22d-1d4c2c561470)


- **Q3 – Antialiasing**  
  Renders 64 randomized rays per pixel and averages them with a box filter.

![Q3](https://github.com/user-attachments/assets/73fb55f0-2364-4764-ab90-62a1c375ccb1)


---

## 🧩 Notes

- Ray tracer with `Ray`, `Camera`, `Scene`, `Surface`, `Plane`, `Sphere` classes.
- Resolution: 512×512  
- Y-axis flipped fix by reversing render loop.  
- Plane at `y = -2` was commented during testing to reduce overexposure.
- LNK4272 issue resolved by switching to 64-bit libraries.

---

**Author:** choijimin_202315326
📅 March 2025




   
