#include "display.h"
#include "transform.h"
#include "lines.h"
#include "options.h"
#include "render.h"

inline double eye_distance(double x, double y, double z, double *eye) {
  double dx = x - eye[0];
  double dy = y - eye[1];
  double dz = z - eye[2];
  return (dx*dx) + (dy*dy) + (dz*dz);
}

static inline void extract_vectors(double coors[12],
				   double a[3], double b[3]) {
  double *p1 = coors, *p2 = coors+4, *p3 = coors+8;
  a[0] = p2[0]-p1[0]; //p2-p1
  a[1] = p2[1]-p1[1];
  a[2] = p2[2]-p1[2];
  b[0] = p3[0]-p2[0]; //p3-p2
  b[1] = p3[1]-p2[1];
  b[2] = p3[2]-p2[2];
}

static inline void cross_vectors(double a[3], double b[3],
				 double cross[3]) {
  cross[0] = (a[1]*b[2]) - (a[2]*b[1]);
  cross[1] = (a[2]*b[0]) - (a[0]*b[2]);
  cross[2] = (a[0]*b[1]) - (a[1]*b[0]);
}

static inline double dot_vectors(double a[3], double b[3]) {
  return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
}

// assumes x y z 1 pattern
static inline char culltri(double coors[12], double *eye,
			   double cross[3]) {
  //backface culling
  double *p1 = coors, *p2 = coors+4, *p3 = coors+8;
  double ep1[3];
  ep1[0] = p1[0] - eye[0]; //p1 - eye
  ep1[1] = p1[1] - eye[1];
  ep1[2] = p1[2] - eye[2];
  double dot = (cross[0]*ep1[0]) + (cross[1]*ep1[1]) + (cross[2]*ep1[2]);
  return dot >= 0;
}

// modifies the pointed to x and y with a perspective transform
inline void perspectify(double *x, double *y, double pz, double *eye) {
  *x = eye[0] - (eye[2] * (*x-eye[0]) / (pz - eye[2]));
  *y = eye[1] - (eye[2] * (*y-eye[1]) / (pz - eye[2]));
}

void renderperspective(Matrix *faces, double *eye, Matrix *colors) {
  double coors[6];
  int c;
  double pz;
  KZ_Point p1 = (KZ_Point){0,0,0,0,0,0}, p2 = (KZ_Point){0,0,0,0,0,0}, p3 = (KZ_Point){0,0,0,0,0,0};
  double tri[12];
  for (c = 0; c < faces->cols; c += 3) {
#if ENABLE_CULLING
      mat_get_column(faces, c, tri);
      mat_get_column(faces, c+1, tri+4);
      mat_get_column(faces, c+2, tri+8);
      double a[3], b[3], cross[3];
      extract_vectors(tri, a, b);
      if (culltri(tri, eye, cross))
	continue;
#endif
    pz = mat_get_cell(faces, c, 2);
    coors[0] = mat_get_cell(faces, c, 0);
    coors[1] = mat_get_cell(faces, c, 1);
    p1.r = eye_distance(coors[0], coors[1], pz, eye);
    p1.kr = mat_get_cell(colors, c, 0);
    p1.kg = mat_get_cell(colors, c, 1);
    p1.kb = mat_get_cell(colors, c, 2);
    p1.ared = p1.kr * ambient_red;
    p1.agreen = p1.kg * ambient_green;
    p1.ablue = p1.kb * ambient_blue;
    p1.norm_x = cross[0];
    p1.norm_y = cross[1];
    p1.norm_z = cross[2];
    perspectify(coors, coors+1, pz, eye);

    pz = mat_get_cell(faces, c+1, 2);
    coors[2] = mat_get_cell(faces, c+1, 0);
    coors[3] = mat_get_cell(faces, c+1, 1);
    p2.r = eye_distance(coors[2], coors[3], pz, eye);
    p2.kr = mat_get_cell(colors, c+1, 0);
    p2.kg = mat_get_cell(colors, c+1, 1);
    p2.kb = mat_get_cell(colors, c+1, 2);
    p2.ared = p2.kr * ambient_red;
    p2.agreen = p2.kg * ambient_green;
    p2.ablue = p2.kb * ambient_blue;
    p2.norm_x = cross[0];
    p2.norm_y = cross[1];
    p2.norm_z = cross[2];
    perspectify(coors+2, coors+3, pz, eye);

    pz = mat_get_cell(faces, c+2, 2);
    coors[4] = mat_get_cell(faces, c+2, 0);
    coors[5] = mat_get_cell(faces, c+2, 1);
    p3.r = eye_distance(coors[4], coors[5], pz, eye);
    p3.kr = mat_get_cell(colors, c+2, 0);
    p3.kg = mat_get_cell(colors, c+2, 1);
    p3.kb = mat_get_cell(colors, c+2, 2);
    p3.ared = p3.kr * ambient_red;
    p3.agreen = p3.kg * ambient_green;
    p3.ablue = p3.kb * ambient_blue;
    p3.norm_x = cross[0];
    p3.norm_y = cross[1];
    p3.norm_z = cross[2];
    perspectify(coors+4, coors+5, pz, eye);

    map_coors(coors, coors+1);
    map_coors(coors+2, coors+3);
    map_coors(coors+4, coors+5);

    p1.x = coors[0];
    p1.y = coors[1];
    p2.x = coors[2];
    p2.y = coors[3];
    p3.x = coors[4];
    p3.y = coors[5];
    draw_triangle(p1, p2, p3);
  }
  flip_KZ_buffer();
}

void rendercyclops(Matrix *faces, double *eye, Matrix *colors) {
  clear_pixel_buffer();
  renderperspective(faces, eye, colors);
  update_display();
}

/*
void renderstereo(Matrix *faces, double *eyes) {
  clear_pixel_buffer();

  // left -- red
  renderperspective(faces, eyes, rgb(127, 0, 0));

  // right -- cyan
  mixcolors(1);
  renderperspective(faces, eyes+3, rgb(0, 127, 127));
  mixcolors(0);

  update_display();
  }*/

char endspin() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if ( event.type == SDL_QUIT ) {
      SDL_Quit();
      return 1;
    } else if (event.type == SDL_KEYDOWN) {
      
      return 1;
    }
  }
  return 0;
}

Matrix *spinmat(int x, int y, int z) {
  Matrix *xr = rotate_x_mat(x * M_PI / 180);
  Matrix *yr = rotate_y_mat(y * M_PI / 180);
  Matrix *zr = rotate_z_mat(z * M_PI / 180);
  Matrix *temp = mat_multiply(xr, yr);
  Matrix *xyz = mat_multiply(temp, zr);
  mat_destruct(temp);
  mat_destruct(xr);
  mat_destruct(yr);
  mat_destruct(zr);
  return xyz;
}

void spincyclops(Matrix *faces, double *eye, Matrix *colors, int del) {
  Matrix *xyz = spinmat(1, 1, 1);
  Matrix *rot;
  Matrix *unspun = faces;
  faces = mat_multiply(xyz, faces);
  clear_screen();
  mixcolors(0);
  while(!endspin()) {
    rot = mat_multiply(xyz, faces);
    mat_destruct(faces);
    faces = rot;
    renderperspective(faces, eye, colors);
    SDL_Delay(del);
    update_display();
    clear_pixel_buffer();
  }
  clear_screen();
  printf("Spin finished... Resetting display.\n");
  renderperspective(unspun, eye, colors);
  mat_destruct(xyz);
  mat_destruct(rot);
}

/*
void spinstereo(Matrix *faces, double *eyes, int del) {
  Matrix *xyz = spinmat(1, 1, 1);
  double *el = eyes;
  double *er = eyes + 3;
  Matrix *rot;
  Matrix *unspun = faces;
  faces = mat_multiply(xyz, faces);
  uint32_t red = rgb(127, 0, 0);
  uint32_t cyan = rgb(0, 127, 127);
  clear_screen();
  while(!endspin()) {
    rot = mat_multiply(xyz, faces);
    mat_destruct(faces);
    faces = rot;
    renderperspective(faces, el, red);
    mixcolors(1);
    renderperspective(faces, er, cyan);
    mixcolors(0);
    SDL_Delay(del);
    update_display();
    clear_pixel_buffer();
  }
  clear_screen();
  printf("Spin finished... Resetting display.\n");
  renderperspective(unspun, el, red);
  mixcolors(1);
  renderperspective(unspun, el, cyan);
  mixcolors(0);
  mat_destruct(xyz);
  mat_destruct(rot);
}
*/
